// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RTSHUDInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class URTSHUDInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LOCKSTEPDEMO_API IRTSHUDInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void HUD_MutipleSelection(bool bLeftMousePressed, bool bShiftPressed);

	virtual TArray<AActor*> HUD_GetSelectedUnits() = 0;
	
	virtual void HUD_SetSelectedUnits(const TArray<AActor*>& Units) = 0;
};
