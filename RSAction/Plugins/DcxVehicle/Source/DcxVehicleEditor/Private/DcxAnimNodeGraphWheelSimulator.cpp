// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehicleEditorPrivatePCH.h"
#include "CompilerResultsLog.h"
#include "DcxVehicleAnimInstance.h"
#include "DcxAnimNodeGraphWheelSimulator.h"

UDcxAnimGraphNodeWheelSimulator::UDcxAnimGraphNodeWheelSimulator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UDcxAnimGraphNodeWheelSimulator::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Wheel Simulator for Vehicles"));
}

FText UDcxAnimGraphNodeWheelSimulator::GetTooltipText() const
{
	return FText::FromString(TEXT("Updates the wheel transform based on setup in Vehicles."));
}

void UDcxAnimGraphNodeWheelSimulator::ValidateAnimNodePostCompile(FCompilerResultsLog& MessageLog, UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex)
{
	if (!CompiledClass->IsChildOf(UDcxVehicleAnimInstance::StaticClass()))
		MessageLog.Error(TEXT("@@ is only allowed for DcxVehicleAnimInstance."), this);
}

bool UDcxAnimGraphNodeWheelSimulator::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return Blueprint && Blueprint->ParentClass->IsChildOf<UDcxVehicleAnimInstance>() && Super::IsCompatibleWithGraph(TargetGraph);
}

const FAnimNode_SkeletalControlBase* UDcxAnimGraphNodeWheelSimulator::GetNode() const
{
	return &Node;
}