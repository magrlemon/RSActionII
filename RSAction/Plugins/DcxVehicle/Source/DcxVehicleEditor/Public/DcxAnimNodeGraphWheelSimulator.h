// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "AnimGraphNode_SkeletalControlBase.h"
#include "DcxAnimNodeWheelSimulator.h"
#include "DcxAnimNodeGraphWheelSimulator.generated.h"

class FCompilerResultsLog;
class UAnimBlueprintGeneratedClass;

UCLASS(Meta = (Keywords = "Simulate Vehicle Wheel"))
class DCXVEHICLEEDITOR_API UDcxAnimGraphNodeWheelSimulator : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Node")
	FDcxAnimNodeWheelSimulator Node;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void ValidateAnimNodePostCompile(FCompilerResultsLog& MessageLog, UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex) override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;

protected:

	virtual const FAnimNode_SkeletalControlBase* GetNode() const override;
};
