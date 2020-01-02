// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

/**
*	
*	===================== SoldierReplicationGraph Replication =====================
*
*	Overview
*	
*		This changes the way actor relevancy works. AActor::IsNetRelevantFor is NOT used in this system!
*		
*		Instead, The USoldierReplicationGraph contains UReplicationGraphNodes. These nodes are responsible for generating lists of actors to replicate for each connection.
*		Most of these lists are persistent across frames. This enables most of the gathering work ("which actors should be considered for replication) to be shared/reused.
*		Nodes may be global (used by all connections), connection specific (each connection gets its own node), or shared (e.g, teams: all connections on the same team share).
*		Actors can be in multiple nodes! For example a pawn may be in the spatialization node but also in the always-relevant-for-team node. It will be returned twice for 
*		teammates. This is ok though should be minimized when possible.
*		
*		USoldierReplicationGraph is intended to not be directly used by the game code. That is, you should not have to include SoldierReplicationGraph.h anywhere else.
*		Rather, USoldierReplicationGraph depends on the game code and registers for events that the game code broadcasts (e.g., events for players joining/leaving teams).
*		This choice was made because it gives USoldierReplicationGraph a complete holistic view of actor replication. Rather than exposing generic public functions that any
*		place in game code can invoke, all notifications are explicitly registered in USoldierReplicationGraph::InitGlobalActorClassSettings.
*		
*	SoldierGame Nodes
*	
*		These are the top level nodes currently used:
*		
*		UReplicationGraphNode_GridSpatialization2D: 
*		This is the spatialization node. All "distance based relevant" actors will be routed here. This node divides the map into a 2D grid. Each cell in the grid contains 
*		children nodes that hold lists of actors based on how they update/go dormant. Actors are put in multiple cells. Connections pull from the single cell they are in.
*		
*		UReplicationGraphNode_ActorList
*		This is an actor list node that contains the always relevant actors. These actors are always relevant to every connection.
*		
*		USoldierReplicationGraphNode_AlwaysRelevant_ForConnection
*		This is the node for connection specific always relevant actors. This node does not maintain a persistent list but builds it each frame. This is possible because (currently)
*		these actors are all easily accessed from the PlayerController. A persistent list would require notifications to be broadcast when these actors change, which would be possible
*		but currently not necessary.
*		
*		USoldierReplicationGraphNode_PlayerStateFrequencyLimiter
*		A custom node for handling player state replication. This replicates a small rolling set of player states (currently 2/frame). This is so player states replicate
*		to simulated connections at a low, steady frequency, and to take advantage of serialization sharing. Auto proxy player states are replicated at higher frequency (to the
*		owning connection only) via USoldierReplicationGraphNode_AlwaysRelevant_ForConnection.
*		
*		UReplicationGraphNode_TearOff_ForConnection
*		Connection specific node for handling tear off actors. This is created and managed in the base implementation of Replication Graph.
*		
*	Dependent Actors (ASoldierWeapon)
*		
*		Replication Graph introduces a concept of dependent actor replication. This is an actor (ASoldierWeapon) that only replicates when another actor replicates (Pawn). I.e, the weapon
*		actor itself never goes into the Replication Graph. It is never gathered on its own and never prioritized. It just has a chance to replicate when the Pawn replicates. This keeps
*		the graph leaner since no extra work has to be done for the weapon actors.
*		
*		See USoldierReplicationGraph::OnCharacterWeaponChange: this is how actors are added/removed from the dependent actor list. 
*	
*	How To Use
*	
*		Making something always relevant: Please avoid if you can :) If you must, just setting AActor::bAlwaysRelevant = true in the class defaults will do it.
*		
*		Making something always relevant to connection: You will need to modify USoldierReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection. You will also want 
*		to make sure the actor does not get put in one of the other nodes. The safest way to do this is by setting its EClassRepNodeMapping to NotRouted in USoldierReplicationGraph::InitGlobalActorClassSettings.
*
*	How To Debug
*	
*		Its a good idea to just disable rep graph to see if your problem is specific to this system or just general replication/game play problem.
*		
*		If it is replication graph related, there are several useful commands that can be used: see ReplicationGraph_Debugging.cpp. The most useful are below. Use the 'cheat' command to run these on the server from a client.
*	
*		"Net.RepGraph.PrintGraph" - this will print the graph to the log: each node and actor. 
*		"Net.RepGraph.PrintGraph class" - same as above but will group by class.
*		"Net.RepGraph.PrintGraph nclass" - same as above but will group by native classes (hides blueprint noise)
*		
*		Net.RepGraph.PrintAll <Frames> <ConnectionIdx> <"Class"/"Nclass"> -  will print the entire graph, the gathered actors, and how they were prioritized for a given connection for X amount of frames.
*		
*		Net.RepGraph.PrintAllActorInfo <ActorMatchString> - will print the class, global, and connection replication info associated with an actor/class. If MatchString is empty will print everything. Call directly from client.
*		
*		SoldierRepGraph.PrintRouting - will print the EClassRepNodeMapping for each class. That is, how a given actor class is routed (or not) in the Replication Graph.
*	
*/

#include "RSAction.h"
#include "SoldierReplicationGraph.h"

#include "Net/UnrealNetwork.h"
#include "Engine/LevelStreaming.h"
#include "EngineUtils.h"
#include "CoreGlobals.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategoryReplicator.h"
#endif

#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/LevelScriptActor.h"
#include "Player/SoldierCharacter.h"
#include "Online/SoldierPlayerState.h"
#include "Weapons/SoldierWeapon.h"
#include "Pickups/SoldierPickup.h"

DEFINE_LOG_CATEGORY( LogSoldierReplicationGraph );

float CVar_SoldierRepGraph_DestructionInfoMaxDist = 30000.f;
static FAutoConsoleVariableRef CVarSoldierRepGraphDestructMaxDist(TEXT("SoldierRepGraph.DestructInfo.MaxDist"), CVar_SoldierRepGraph_DestructionInfoMaxDist, TEXT("Max distance (not squared) to rep destruct infos at"), ECVF_Default );

int32 CVar_SoldierRepGraph_DisplayClientLevelStreaming = 0;
static FAutoConsoleVariableRef CVarSoldierRepGraphDisplayClientLevelStreaming(TEXT("SoldierRepGraph.DisplayClientLevelStreaming"), CVar_SoldierRepGraph_DisplayClientLevelStreaming, TEXT(""), ECVF_Default );

float CVar_SoldierRepGraph_CellSize = 10000.f;
static FAutoConsoleVariableRef CVarSoldierRepGraphCellSize(TEXT("SoldierRepGraph.CellSize"), CVar_SoldierRepGraph_CellSize, TEXT(""), ECVF_Default );

// Essentially "Min X" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
float CVar_SoldierRepGraph_SpatialBiasX = -150000.f;
static FAutoConsoleVariableRef CVarSoldierRepGraphSpatialBiasX(TEXT("SoldierRepGraph.SpatialBiasX"), CVar_SoldierRepGraph_SpatialBiasX, TEXT(""), ECVF_Default );

// Essentially "Min Y" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
float CVar_SoldierRepGraph_SpatialBiasY = -200000.f;
static FAutoConsoleVariableRef CVarSoldierRepSpatialBiasY(TEXT("SoldierRepGraph.SpatialBiasY"), CVar_SoldierRepGraph_SpatialBiasY, TEXT(""), ECVF_Default );

// How many buckets to spread dynamic, spatialized actors across. High number = more buckets = smaller effective replication frequency. This happens before individual actors do their own NetUpdateFrequency check.
int32 CVar_SoldierRepGraph_DynamicActorFrequencyBuckets = 3;
static FAutoConsoleVariableRef CVarSoldierRepDynamicActorFrequencyBuckets(TEXT("SoldierRepGraph.DynamicActorFrequencyBuckets"), CVar_SoldierRepGraph_DynamicActorFrequencyBuckets, TEXT(""), ECVF_Default );

int32 CVar_SoldierRepGraph_DisableSpatialRebuilds = 1;
static FAutoConsoleVariableRef CVarSoldierRepDisableSpatialRebuilds(TEXT("SoldierRepGraph.DisableSpatialRebuilds"), CVar_SoldierRepGraph_DisableSpatialRebuilds, TEXT(""), ECVF_Default );

// ----------------------------------------------------------------------------------------------------------


USoldierReplicationGraph::USoldierReplicationGraph()
{
}

void InitClassReplicationInfo(FClassReplicationInfo& Info, UClass* Class, bool bSpatialize, float ServerMaxTickRate)
{
	AActor* CDO = Class->GetDefaultObject<AActor>();
	if (bSpatialize)
	{
		Info.SetCullDistanceSquared(CDO->NetCullDistanceSquared);
		UE_LOG(LogSoldierReplicationGraph, Log, TEXT("Setting cull distance for %s to %f (%f)"), *Class->GetName(), Info.GetCullDistanceSquared(), Info.GetCullDistance());
	}

	Info.ReplicationPeriodFrame = FMath::Max<uint32>( (uint32)FMath::RoundToFloat(ServerMaxTickRate / CDO->NetUpdateFrequency), 1);

	UClass* NativeClass = Class;
	while(!NativeClass->IsNative() && NativeClass->GetSuperClass() && NativeClass->GetSuperClass() != AActor::StaticClass())
	{
		NativeClass = NativeClass->GetSuperClass();
	}

	UE_LOG(LogSoldierReplicationGraph, Log, TEXT("Setting replication period for %s (%s) to %d frames (%.2f)"), *Class->GetName(), *NativeClass->GetName(), Info.ReplicationPeriodFrame, CDO->NetUpdateFrequency);
}

void USoldierReplicationGraph::ResetGameWorldState()
{
	Super::ResetGameWorldState();

	AlwaysRelevantStreamingLevelActors.Empty();

	for (UNetReplicationGraphConnection* ConnManager : Connections)
	{
		for (UReplicationGraphNode* ConnectionNode : ConnManager->GetConnectionGraphNodes())
		{
			if (USoldierReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = Cast<USoldierReplicationGraphNode_AlwaysRelevant_ForConnection>(ConnectionNode))
			{
				AlwaysRelevantConnectionNode->ResetGameWorldState();
			}
		}
	}

	for (UNetReplicationGraphConnection* ConnManager : PendingConnections)
	{
		for (UReplicationGraphNode* ConnectionNode : ConnManager->GetConnectionGraphNodes())
		{
			if (USoldierReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = Cast<USoldierReplicationGraphNode_AlwaysRelevant_ForConnection>(ConnectionNode))
			{
				AlwaysRelevantConnectionNode->ResetGameWorldState();
			}
		}
	}
}

void USoldierReplicationGraph::InitGlobalActorClassSettings()
{
	Super::InitGlobalActorClassSettings();

	// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Programatically build the rules.
	// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
	auto AddInfo = [&]( UClass* Class, EClassRepNodeMapping Mapping) { ClassRepNodePolicies.Set(Class, Mapping); };

	AddInfo( ASoldierWeapon::StaticClass(),							EClassRepNodeMapping::NotRouted);				// Handled via DependantActor replication (Pawn)
	AddInfo( ALevelScriptActor::StaticClass(),						EClassRepNodeMapping::NotRouted);				// Not needed
	AddInfo( APlayerState::StaticClass(),							EClassRepNodeMapping::NotRouted);				// Special cased via USoldierReplicationGraphNode_PlayerStateFrequencyLimiter
	AddInfo( AReplicationGraphDebugActor::StaticClass(),			EClassRepNodeMapping::NotRouted);				// Not needed. Replicated special case inside RepGraph
	AddInfo( AInfo::StaticClass(),									EClassRepNodeMapping::RelevantAllConnections);	// Non spatialized, relevant to all
	AddInfo( ASoldierPickup::StaticClass(),							EClassRepNodeMapping::Spatialize_Static);		// Spatialized and never moves. Routes to GridNode.

#if WITH_GAMEPLAY_DEBUGGER
	AddInfo( AGameplayDebuggerCategoryReplicator::StaticClass(),	EClassRepNodeMapping::NotRouted);				// Replicated via USoldierReplicationGraphNode_AlwaysRelevant_ForConnection
#endif

	TArray<UClass*> AllReplicatedClasses;

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		AActor* ActorCDO = Cast<AActor>(Class->GetDefaultObject());
		if (!ActorCDO || !ActorCDO->GetIsReplicated())
		{
			continue;
		}

		// Skip SKEL and REINST classes.
		if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
		{
			continue;
		}

		// --------------------------------------------------------------------
		// This is a replicated class. Save this off for the second pass below
		// --------------------------------------------------------------------
		
		AllReplicatedClasses.Add(Class);

		// Skip if already in the map (added explicitly)
		if (ClassRepNodePolicies.Contains(Class, false))
		{
			continue;
		}
		
		auto ShouldSpatialize = [](const AActor* CDO)
		{
			return CDO->GetIsReplicated() && (!(CDO->bAlwaysRelevant || CDO->bOnlyRelevantToOwner || CDO->bNetUseOwnerRelevancy));
		};

		auto GetLegacyDebugStr = [](const AActor* CDO)
		{
			return FString::Printf(TEXT("%s [%d/%d/%d]"), *CDO->GetClass()->GetName(), CDO->bAlwaysRelevant, CDO->bOnlyRelevantToOwner, CDO->bNetUseOwnerRelevancy);
		};

		// Only handle this class if it differs from its super. There is no need to put every child class explicitly in the graph class mapping
		UClass* SuperClass = Class->GetSuperClass();
		if (AActor* SuperCDO = Cast<AActor>(SuperClass->GetDefaultObject()))
		{
			if (	SuperCDO->GetIsReplicated() == ActorCDO->GetIsReplicated() 
				&&	SuperCDO->bAlwaysRelevant == ActorCDO->bAlwaysRelevant
				&&	SuperCDO->bOnlyRelevantToOwner == ActorCDO->bOnlyRelevantToOwner
				&&	SuperCDO->bNetUseOwnerRelevancy == ActorCDO->bNetUseOwnerRelevancy
				)
			{
				continue;
			}

			if (ShouldSpatialize(ActorCDO) == false && ShouldSpatialize(SuperCDO) == true)
			{
				UE_LOG(LogSoldierReplicationGraph, Log, TEXT("Adding %s to NonSpatializedChildClasses. (Parent: %s)"), *GetLegacyDebugStr(ActorCDO), *GetLegacyDebugStr(SuperCDO));
				NonSpatializedChildClasses.Add(Class);
			}
		}
			
		if (ShouldSpatialize(ActorCDO))
		{
			AddInfo(Class, EClassRepNodeMapping::Spatialize_Dynamic);
		}
		else if (ActorCDO->bAlwaysRelevant && !ActorCDO->bOnlyRelevantToOwner)
		{
			AddInfo(Class, EClassRepNodeMapping::RelevantAllConnections);
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Setup FClassReplicationInfo. This is essentially the per class replication settings. Some we set explicitly, the rest we are setting via looking at the legacy settings on AActor.
	// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
	TArray<UClass*> ExplicitlySetClasses;
	auto SetClassInfo = [&](UClass* Class, const FClassReplicationInfo& Info) { GlobalActorReplicationInfoMap.SetClassInfo(Class, Info); ExplicitlySetClasses.Add(Class); };

	FClassReplicationInfo PawnClassRepInfo;
	PawnClassRepInfo.DistancePriorityScale = 1.f;
	PawnClassRepInfo.StarvationPriorityScale = 1.f;
	PawnClassRepInfo.ActorChannelFrameTimeout = 4;
	PawnClassRepInfo.SetCullDistanceSquared(15000.f * 15000.f); // Yuck
	SetClassInfo( APawn::StaticClass(), PawnClassRepInfo );

	FClassReplicationInfo PlayerStateRepInfo;
	PlayerStateRepInfo.DistancePriorityScale = 0.f;
	PlayerStateRepInfo.ActorChannelFrameTimeout = 0;
	SetClassInfo( APlayerState::StaticClass(), PlayerStateRepInfo );
	
	UReplicationGraphNode_ActorListFrequencyBuckets::DefaultSettings.ListSize = 12;

	// Set FClassReplicationInfo based on legacy settings from all replicated classes
	for (UClass* ReplicatedClass : AllReplicatedClasses)
	{
		if (ExplicitlySetClasses.FindByPredicate([&](const UClass* SetClass) { return ReplicatedClass->IsChildOf(SetClass); }) != nullptr)
		{
			continue;
		}

		const bool bClassIsSpatialized = IsSpatialized(ClassRepNodePolicies.GetChecked(ReplicatedClass));

		FClassReplicationInfo ClassInfo;
		InitClassReplicationInfo(ClassInfo, ReplicatedClass, bClassIsSpatialized, NetDriver->NetServerMaxTickRate);
		GlobalActorReplicationInfoMap.SetClassInfo( ReplicatedClass, ClassInfo );
	}


	// Print out what we came up with
	UE_LOG(LogSoldierReplicationGraph, Log, TEXT(""));
	UE_LOG(LogSoldierReplicationGraph, Log, TEXT("Class Routing Map: "));
	UEnum* Enum = StaticEnum<EClassRepNodeMapping>();
	for (auto ClassMapIt = ClassRepNodePolicies.CreateIterator(); ClassMapIt; ++ClassMapIt)
	{		
		UClass* Class = CastChecked<UClass>(ClassMapIt.Key().ResolveObjectPtr());
		const EClassRepNodeMapping Mapping = ClassMapIt.Value();

		// Only print if different than native class
		UClass* ParentNativeClass = GetParentNativeClass(Class);
		const EClassRepNodeMapping* ParentMapping = ClassRepNodePolicies.Get(ParentNativeClass);
		if (ParentMapping && Class != ParentNativeClass && Mapping == *ParentMapping)
		{
			continue;
		}

		UE_LOG(LogSoldierReplicationGraph, Log, TEXT("  %s (%s) -> %s"), *Class->GetName(), *GetNameSafe(ParentNativeClass), *Enum->GetNameStringByValue(static_cast<uint32>(Mapping)));
	}

	UE_LOG(LogSoldierReplicationGraph, Log, TEXT(""));
	UE_LOG(LogSoldierReplicationGraph, Log, TEXT("Class Settings Map: "));
	FClassReplicationInfo DefaultValues;
	for (auto ClassRepInfoIt = GlobalActorReplicationInfoMap.CreateClassMapIterator(); ClassRepInfoIt; ++ClassRepInfoIt)
	{
		UClass* Class = CastChecked<UClass>(ClassRepInfoIt.Key().ResolveObjectPtr());
		const FClassReplicationInfo& ClassInfo = ClassRepInfoIt.Value();
		UE_LOG(LogSoldierReplicationGraph, Log, TEXT("  %s (%s) -> %s"), *Class->GetName(), *GetNameSafe(GetParentNativeClass(Class)), *ClassInfo.BuildDebugStringDelta());
	}


	// Rep destruct infos based on CVar value
	DestructInfoMaxDistanceSquared = CVar_SoldierRepGraph_DestructionInfoMaxDist * CVar_SoldierRepGraph_DestructionInfoMaxDist;

	// -------------------------------------------------------
	//	Register for game code callbacks.
	//	This could have been done the other way: E.g, AMyGameActor could do GetNetDriver()->GetReplicationDriver<USoldierReplicationGraph>()->OnMyGameEvent etc.
	//	This way at least keeps the rep graph out of game code directly and allows rep graph to exist in its own module
	//	So for now, erring on the side of a cleaning dependencies between classes.
	// -------------------------------------------------------
	
	ASoldierCharacter::NotifyEquipWeapon.AddUObject(this, &USoldierReplicationGraph::OnCharacterEquipWeapon);
	ASoldierCharacter::NotifyUnEquipWeapon.AddUObject(this, &USoldierReplicationGraph::OnCharacterUnEquipWeapon);

#if WITH_GAMEPLAY_DEBUGGER
	AGameplayDebuggerCategoryReplicator::NotifyDebuggerOwnerChange.AddUObject(this, &USoldierReplicationGraph::OnGameplayDebuggerOwnerChange);
#endif
}

void USoldierReplicationGraph::InitGlobalGraphNodes()
{
	// Preallocate some replication lists.
	PreAllocateRepList(3, 12);
	PreAllocateRepList(6, 12);
	PreAllocateRepList(128, 64);
	PreAllocateRepList(512, 16);

	// -----------------------------------------------
	//	Spatial Actors
	// -----------------------------------------------

	GridNode = CreateNewNode<UReplicationGraphNode_GridSpatialization2D>();
	GridNode->CellSize = CVar_SoldierRepGraph_CellSize;
	GridNode->SpatialBias = FVector2D(CVar_SoldierRepGraph_SpatialBiasX, CVar_SoldierRepGraph_SpatialBiasY);

	if (CVar_SoldierRepGraph_DisableSpatialRebuilds)
	{
		GridNode->AddSpatialRebuildBlacklistClass(AActor::StaticClass()); // Disable All spatial rebuilding
	}
	
	AddGlobalGraphNode(GridNode);

	// -----------------------------------------------
	//	Always Relevant (to everyone) Actors
	// -----------------------------------------------
	AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_ActorList>();
	AddGlobalGraphNode(AlwaysRelevantNode);

	// -----------------------------------------------
	//	Player State specialization. This will return a rolling subset of the player states to replicate
	// -----------------------------------------------
	USoldierReplicationGraphNode_PlayerStateFrequencyLimiter* PlayerStateNode = CreateNewNode<USoldierReplicationGraphNode_PlayerStateFrequencyLimiter>();
	AddGlobalGraphNode(PlayerStateNode);
}

void USoldierReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection)
{
	Super::InitConnectionGraphNodes(RepGraphConnection);

	USoldierReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = CreateNewNode<USoldierReplicationGraphNode_AlwaysRelevant_ForConnection>();

	// This node needs to know when client levels go in and out of visibility
	RepGraphConnection->OnClientVisibleLevelNameAdd.AddUObject(AlwaysRelevantConnectionNode, &USoldierReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd);
	RepGraphConnection->OnClientVisibleLevelNameRemove.AddUObject(AlwaysRelevantConnectionNode, &USoldierReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove);

	AddConnectionGraphNode(AlwaysRelevantConnectionNode, RepGraphConnection);
}

EClassRepNodeMapping USoldierReplicationGraph::GetMappingPolicy(UClass* Class)
{
	EClassRepNodeMapping* PolicyPtr = ClassRepNodePolicies.Get(Class);
	EClassRepNodeMapping Policy = PolicyPtr ? *PolicyPtr : EClassRepNodeMapping::NotRouted;
	return Policy;
}

void USoldierReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo)
{
	EClassRepNodeMapping Policy = GetMappingPolicy(ActorInfo.Class);
	switch(Policy)
	{
		case EClassRepNodeMapping::NotRouted:
		{
			break;
		}
		
		case EClassRepNodeMapping::RelevantAllConnections:
		{
			if (ActorInfo.StreamingLevelName == NAME_None)
			{
				AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
			}
			else
			{
				FActorRepListRefView& RepList = AlwaysRelevantStreamingLevelActors.FindOrAdd(ActorInfo.StreamingLevelName);
				RepList.PrepareForWrite();
				RepList.ConditionalAdd(ActorInfo.Actor);
			}
			break;
		}

		case EClassRepNodeMapping::Spatialize_Static:
		{
			GridNode->AddActor_Static(ActorInfo, GlobalInfo);
			break;
		}
		
		case EClassRepNodeMapping::Spatialize_Dynamic:
		{
			GridNode->AddActor_Dynamic(ActorInfo, GlobalInfo);
			break;
		}
		
		case EClassRepNodeMapping::Spatialize_Dormancy:
		{
			GridNode->AddActor_Dormancy(ActorInfo, GlobalInfo);
			break;
		}
	};
}

void USoldierReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
	EClassRepNodeMapping Policy = GetMappingPolicy(ActorInfo.Class);
	switch(Policy)
	{
		case EClassRepNodeMapping::NotRouted:
		{
			break;
		}
		
		case EClassRepNodeMapping::RelevantAllConnections:
		{
			if (ActorInfo.StreamingLevelName == NAME_None)
			{
				AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
			}
			else
			{
				FActorRepListRefView& RepList = AlwaysRelevantStreamingLevelActors.FindChecked(ActorInfo.StreamingLevelName);
				if (RepList.Remove(ActorInfo.Actor) == false)
				{
					UE_LOG(LogSoldierReplicationGraph, Warning, TEXT("Actor %s was not found in AlwaysRelevantStreamingLevelActors list. LevelName: %s"), *GetActorRepListTypeDebugString(ActorInfo.Actor), *ActorInfo.StreamingLevelName.ToString());
				}				
			}
			break;
		}

		case EClassRepNodeMapping::Spatialize_Static:
		{
			GridNode->RemoveActor_Static(ActorInfo);
			break;
		}
		
		case EClassRepNodeMapping::Spatialize_Dynamic:
		{
			GridNode->RemoveActor_Dynamic(ActorInfo);
			break;
		}
		
		case EClassRepNodeMapping::Spatialize_Dormancy:
		{
			GridNode->RemoveActor_Dormancy(ActorInfo);
			break;
		}
	};
}

// Since we listen to global (static) events, we need to watch out for cross world broadcasts (PIE)
#if WITH_EDITOR
#define CHECK_WORLDS(X) if(X->GetWorld() != GetWorld()) return;
#else
#define CHECK_WORLDS(X)
#endif

void USoldierReplicationGraph::OnCharacterEquipWeapon(ASoldierCharacter* Character, ASoldierWeapon* NewWeapon)
{
	if (Character && NewWeapon)
	{
		CHECK_WORLDS(Character);

		GlobalActorReplicationInfoMap.AddDependentActor(Character, NewWeapon);
	}
}

void USoldierReplicationGraph::OnCharacterUnEquipWeapon(ASoldierCharacter* Character, ASoldierWeapon* OldWeapon)
{
	if (Character && OldWeapon)
	{
		CHECK_WORLDS(Character);

		GlobalActorReplicationInfoMap.RemoveDependentActor(Character, OldWeapon);
	}
}

#if WITH_GAMEPLAY_DEBUGGER
void USoldierReplicationGraph::OnGameplayDebuggerOwnerChange(AGameplayDebuggerCategoryReplicator* Debugger, APlayerController* OldOwner)
{
	auto GetAlwaysRelevantForConnectionNode = [&](APlayerController* Controller) -> USoldierReplicationGraphNode_AlwaysRelevant_ForConnection*
	{
		if (OldOwner)
		{
			if (UNetConnection* NetConnection = OldOwner->GetNetConnection())
			{
				if (UNetReplicationGraphConnection* GraphConnection = FindOrAddConnectionManager(NetConnection))
				{
					for (UReplicationGraphNode* ConnectionNode : GraphConnection->GetConnectionGraphNodes())
					{
						if (USoldierReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = Cast<USoldierReplicationGraphNode_AlwaysRelevant_ForConnection>(ConnectionNode))
						{
							return AlwaysRelevantConnectionNode;
						}
					}

				}
			}
		}

		return nullptr;
	};

	if (USoldierReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = GetAlwaysRelevantForConnectionNode(OldOwner))
	{
		AlwaysRelevantConnectionNode->GameplayDebugger = nullptr;
	}

	if (USoldierReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = GetAlwaysRelevantForConnectionNode(Debugger->GetReplicationOwner()))
	{
		AlwaysRelevantConnectionNode->GameplayDebugger = Debugger;
	}
}
#endif

// ------------------------------------------------------------------------------

void USoldierReplicationGraphNode_AlwaysRelevant_ForConnection::ResetGameWorldState()
{
	AlwaysRelevantStreamingLevelsNeedingReplication.Empty();
}

void USoldierReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{
	QUICK_SCOPE_CYCLE_COUNTER( USoldierReplicationGraphNode_AlwaysRelevant_ForConnection_GatherActorListsForConnection );

	USoldierReplicationGraph* SoldierGraph = CastChecked<USoldierReplicationGraph>(GetOuter());

	ReplicationActorList.Reset();

	auto ResetActorCullDistance = [&](AActor* ActorToSet, AActor*& LastActor) {

		if (ActorToSet != LastActor)
		{
			LastActor = ActorToSet;

			UE_LOG(LogSoldierReplicationGraph, Verbose, TEXT("Setting pawn cull distance to 0. %s"), *ActorToSet->GetName());
			FConnectionReplicationActorInfo& ConnectionActorInfo = Params.ConnectionManager.ActorInfoMap.FindOrAdd(ActorToSet);
			ConnectionActorInfo.SetCullDistanceSquared(0.f);
		}
	};

	for (const FNetViewer& CurViewer : Params.Viewers)
	{
		ReplicationActorList.ConditionalAdd(CurViewer.InViewer);
		ReplicationActorList.ConditionalAdd(CurViewer.ViewTarget);

		if (ASoldierPlayerController* PC = Cast<ASoldierPlayerController>(CurViewer.InViewer))
		{
			// 50% throttling of PlayerStates.
			const bool bReplicatePS = (Params.ConnectionManager.ConnectionId % 2) == (Params.ReplicationFrameNum % 2);
			if (bReplicatePS)
			{
				// Always return the player state to the owning player. Simulated proxy player states are handled by USoldierReplicationGraphNode_PlayerStateFrequencyLimiter
				if (APlayerState* PS = PC->PlayerState)
				{
					if (!bInitializedPlayerState)
					{
						bInitializedPlayerState = true;
						FConnectionReplicationActorInfo& ConnectionActorInfo = Params.ConnectionManager.ActorInfoMap.FindOrAdd(PS);
						ConnectionActorInfo.ReplicationPeriodFrame = 1;
					}

					ReplicationActorList.ConditionalAdd(PS);
				}
			}

			FAlwaysRelevantActorInfo* LastData = PastRelevantActors.FindByKey<UNetConnection*>(CurViewer.Connection);

			// We've not seen this actor before, go ahead and add them.
			if (LastData == nullptr)
			{
				FAlwaysRelevantActorInfo NewActorInfo;
				NewActorInfo.Connection = CurViewer.Connection;
				LastData = &(PastRelevantActors[PastRelevantActors.Add(NewActorInfo)]);
			}

			check(LastData != nullptr);

			if (ASoldierCharacter* Pawn = Cast<ASoldierCharacter>(PC->GetPawn()))
			{
				ResetActorCullDistance(Pawn, LastData->LastViewer);

				if (Pawn != CurViewer.ViewTarget)
				{
					ReplicationActorList.ConditionalAdd(Pawn);
				}

				int32 InventoryCount = Pawn->GetInventoryCount();
				for (int32 i = 0; i < InventoryCount; ++i)
				{
					ASoldierWeapon* Weapon = Pawn->GetInventoryWeapon(i);
					if (Weapon)
					{
						ReplicationActorList.ConditionalAdd(Weapon);
					}
				}
			}

			if (ASoldierCharacter* ViewTargetPawn = Cast<ASoldierCharacter>(CurViewer.ViewTarget))
			{
				ResetActorCullDistance(ViewTargetPawn, LastData->LastViewTarget);
			}
		}
	}

	PastRelevantActors.RemoveAll([&](FAlwaysRelevantActorInfo& RelActorInfo) {
		return RelActorInfo.Connection == nullptr;
	});

	Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);

	// Always relevant streaming level actors.
	FPerConnectionActorInfoMap& ConnectionActorInfoMap = Params.ConnectionManager.ActorInfoMap;
	
	TMap<FName, FActorRepListRefView>& AlwaysRelevantStreamingLevelActors = SoldierGraph->AlwaysRelevantStreamingLevelActors;

	for (int32 Idx=AlwaysRelevantStreamingLevelsNeedingReplication.Num()-1; Idx >= 0; --Idx)
	{
		const FName& StreamingLevel = AlwaysRelevantStreamingLevelsNeedingReplication[Idx];

		FActorRepListRefView* Ptr = AlwaysRelevantStreamingLevelActors.Find(StreamingLevel);
		if (Ptr == nullptr)
		{
			// No always relevant lists for that level
			UE_CLOG(CVar_SoldierRepGraph_DisplayClientLevelStreaming > 0, LogSoldierReplicationGraph, Display, TEXT("CLIENTSTREAMING Removing %s from AlwaysRelevantStreamingLevelActors because FActorRepListRefView is null. %s "), *StreamingLevel.ToString(),  *Params.ConnectionManager.GetName());
			AlwaysRelevantStreamingLevelsNeedingReplication.RemoveAtSwap(Idx, 1, false);
			continue;
		}

		FActorRepListRefView& RepList = *Ptr;

		if (RepList.Num() > 0)
		{
			bool bAllDormant = true;
			for (FActorRepListType Actor : RepList)
			{
				FConnectionReplicationActorInfo& ConnectionActorInfo = ConnectionActorInfoMap.FindOrAdd(Actor);
				if (ConnectionActorInfo.bDormantOnConnection == false)
				{
					bAllDormant = false;
					break;
				}
			}

			if (bAllDormant)
			{
				UE_CLOG(CVar_SoldierRepGraph_DisplayClientLevelStreaming > 0, LogSoldierReplicationGraph, Display, TEXT("CLIENTSTREAMING All AlwaysRelevant Actors Dormant on StreamingLevel %s for %s. Removing list."), *StreamingLevel.ToString(), *Params.ConnectionManager.GetName());
				AlwaysRelevantStreamingLevelsNeedingReplication.RemoveAtSwap(Idx, 1, false);
			}
			else
			{
				UE_CLOG(CVar_SoldierRepGraph_DisplayClientLevelStreaming > 0, LogSoldierReplicationGraph, Display, TEXT("CLIENTSTREAMING Adding always Actors on StreamingLevel %s for %s because it has at least one non dormant actor"), *StreamingLevel.ToString(), *Params.ConnectionManager.GetName());
				Params.OutGatheredReplicationLists.AddReplicationActorList(RepList);
			}
		}
		else
		{
			UE_LOG(LogSoldierReplicationGraph, Warning, TEXT("USoldierReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection - empty RepList %s"), *Params.ConnectionManager.GetName());
		}

	}

#if WITH_GAMEPLAY_DEBUGGER
	if (GameplayDebugger)
	{
		ReplicationActorList.ConditionalAdd(GameplayDebugger);
	}
#endif
}

void USoldierReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd(FName LevelName, UWorld* StreamingWorld)
{
	UE_CLOG(CVar_SoldierRepGraph_DisplayClientLevelStreaming > 0, LogSoldierReplicationGraph, Display, TEXT("CLIENTSTREAMING ::OnClientLevelVisibilityAdd - %s"), *LevelName.ToString());
	AlwaysRelevantStreamingLevelsNeedingReplication.Add(LevelName);
}

void USoldierReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove(FName LevelName)
{
	UE_CLOG(CVar_SoldierRepGraph_DisplayClientLevelStreaming > 0, LogSoldierReplicationGraph, Display, TEXT("CLIENTSTREAMING ::OnClientLevelVisibilityRemove - %s"), *LevelName.ToString());
	AlwaysRelevantStreamingLevelsNeedingReplication.Remove(LevelName);
}

void USoldierReplicationGraphNode_AlwaysRelevant_ForConnection::LogNode(FReplicationGraphDebugInfo& DebugInfo, const FString& NodeName) const
{
	DebugInfo.Log(NodeName);
	DebugInfo.PushIndent();
	LogActorRepList(DebugInfo, NodeName, ReplicationActorList);

	for (const FName& LevelName : AlwaysRelevantStreamingLevelsNeedingReplication)
	{
		USoldierReplicationGraph* SoldierGraph = CastChecked<USoldierReplicationGraph>(GetOuter());
		if (FActorRepListRefView* RepList = SoldierGraph->AlwaysRelevantStreamingLevelActors.Find(LevelName))
		{
			LogActorRepList(DebugInfo, FString::Printf(TEXT("AlwaysRelevant StreamingLevel List: %s"), *LevelName.ToString()), *RepList);
		}
	}

	DebugInfo.PopIndent();
}

// ------------------------------------------------------------------------------

USoldierReplicationGraphNode_PlayerStateFrequencyLimiter::USoldierReplicationGraphNode_PlayerStateFrequencyLimiter()
{
	bRequiresPrepareForReplicationCall = true;
}

void USoldierReplicationGraphNode_PlayerStateFrequencyLimiter::PrepareForReplication()
{
	QUICK_SCOPE_CYCLE_COUNTER( USoldierReplicationGraphNode_PlayerStateFrequencyLimiter_GlobalPrepareForReplication );

	ReplicationActorLists.Reset();
	ForceNetUpdateReplicationActorList.Reset();

	ReplicationActorLists.AddDefaulted();
	FActorRepListRefView* CurrentList = &ReplicationActorLists[0];
	CurrentList->PrepareForWrite();

	// We rebuild our lists of player states each frame. This is not as efficient as it could be but its the simplest way
	// to handle players disconnecting and keeping the lists compact. If the lists were persistent we would need to defrag them as players left.

	for (TActorIterator<APlayerState> It(GetWorld()); It; ++It)
	{
		APlayerState* PS = *It;
		if (IsActorValidForReplicationGather(PS) == false)
		{
			continue;
		}

		if (CurrentList->Num() >= TargetActorsPerFrame)
		{
			ReplicationActorLists.AddDefaulted();
			CurrentList = &ReplicationActorLists.Last(); 
			CurrentList->PrepareForWrite();
		}
		
		CurrentList->Add(PS);
	}	
}

void USoldierReplicationGraphNode_PlayerStateFrequencyLimiter::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{
	const int32 ListIdx = Params.ReplicationFrameNum % ReplicationActorLists.Num();
	Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorLists[ListIdx]);

	if (ForceNetUpdateReplicationActorList.Num() > 0)
	{
		Params.OutGatheredReplicationLists.AddReplicationActorList(ForceNetUpdateReplicationActorList);
	}	
}

void USoldierReplicationGraphNode_PlayerStateFrequencyLimiter::LogNode(FReplicationGraphDebugInfo& DebugInfo, const FString& NodeName) const
{
	DebugInfo.Log(NodeName);
	DebugInfo.PushIndent();	

	int32 i=0;
	for (const FActorRepListRefView& List : ReplicationActorLists)
	{
		LogActorRepList(DebugInfo, FString::Printf(TEXT("Bucket[%d]"), i++), List);
	}

	DebugInfo.PopIndent();
}

// ------------------------------------------------------------------------------

void USoldierReplicationGraph::PrintRepNodePolicies()
{
	UEnum* Enum = StaticEnum<EClassRepNodeMapping>();
	if (!Enum)
	{
		return;
	}

	GLog->Logf(TEXT("===================================="));
	GLog->Logf(TEXT("Soldier Replication Routing Policies"));
	GLog->Logf(TEXT("===================================="));

	for (auto It = ClassRepNodePolicies.CreateIterator(); It; ++It)
	{
		FObjectKey ObjKey = It.Key();
		
		EClassRepNodeMapping Mapping = It.Value();

		GLog->Logf(TEXT("%-40s --> %s"), *GetNameSafe(ObjKey.ResolveObjectPtr()), *Enum->GetNameStringByValue(static_cast<uint32>(Mapping)));
	}
}

FAutoConsoleCommandWithWorldAndArgs SoldierPrintRepNodePoliciesCmd(TEXT("SoldierRepGraph.PrintRouting"),TEXT("Prints how actor classes are routed to RepGraph nodes"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		for (TObjectIterator<USoldierReplicationGraph> It; It; ++It)
		{
			It->PrintRepNodePolicies();
		}
	})
);

// ------------------------------------------------------------------------------

FAutoConsoleCommandWithWorldAndArgs ChangeFrequencyBucketsCmd(TEXT("SoldierRepGraph.FrequencyBuckets"), TEXT("Resets frequency bucket count."), FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray< FString >& Args, UWorld* World) 
{
	int32 Buckets = 1;
	if (Args.Num() > 0)
	{
		LexTryParseString<int32>(Buckets, *Args[0]);
	}

	UE_LOG(LogSoldierReplicationGraph, Display, TEXT("Setting Frequency Buckets to %d"), Buckets);
	for (TObjectIterator<UReplicationGraphNode_ActorListFrequencyBuckets> It; It; ++It)
	{
		UReplicationGraphNode_ActorListFrequencyBuckets* Node = *It;
		Node->SetNonStreamingCollectionSize(Buckets);
	}
}));
