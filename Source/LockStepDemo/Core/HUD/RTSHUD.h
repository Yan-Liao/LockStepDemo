// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LockStepDemo/Interface/RTSHUDInterface.h"
#include "RTSHUD.generated.h"

/**
 * 
 */
UCLASS()
class LOCKSTEPDEMO_API ARTSHUD : public AHUD, public IRTSHUDInterface
{
	GENERATED_BODY()

public:
	virtual void HUD_MutipleSelection_Implementation(bool bLeftMousePressed, bool bShiftPressed) override;

	virtual TArray<AActor*> HUD_GetSelectedUnits() override;

	virtual void HUD_SetSelectedUnits(const TArray<AActor*>& Units) override;
	
	virtual void DrawHUD() override;

	UFUNCTION(BlueprintCallable)
	void SetMappingMode();
	
private:
	//计算选中目标，并调用选中接口
	void QueryUnitsInRange();

	void UpdateSelectedUnits(const TArray<AActor*>& NewSelectedActors, bool bAdded = false);
	
private:
	bool IsMousePressed = false;
	bool IsShiftPressed = false;
	FVector2d StartMousePoint;
	FVector2d CurrentMousePoint;
	const int32 SelectLength = 1000000;
	const FColor SelectBoxColor = {118, 238 , 198, 50};
	TSet<AActor*> SelectedActors;

//for homework
private:
	bool IsMappingMode = false;  //映射模式
	
};
