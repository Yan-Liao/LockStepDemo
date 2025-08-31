// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RTSUnitsInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class URTSUnitsInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LOCKSTEPDEMO_API IRTSUnitsInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Units_Selected(bool bSelected);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Units_SetTargetLocation(const FVector& InTargetLocation);

	/*
	 * InUnitState 0: stop  1: move
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Units_SetUnitState(int32 InUnitState);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Units_SetUnitLocation(const FVector& InLocation);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FVector Units_GetUnitLocation();
};
