// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RTSPawnInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class URTSPawnInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LOCKSTEPDEMO_API IRTSPawnInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void RTSPawn_AttackCommand();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void RTSPawn_MoveCommand();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void RTSPawn_StopCommand();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void RTSPawn_HoldCommand();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void RTSPawn_CancelCommand();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void RTSPawn_SkillCommand(int SkillIndex);
};
