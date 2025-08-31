// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTSPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class LOCKSTEPDEMO_API ARTSPlayerController : public APlayerController
{
	GENERATED_BODY()

	ARTSPlayerController();

protected:
	virtual void BeginPlay() override;

protected:
	//输入映射
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTSPawnInput")
	class UInputMappingContext* RTSPawnMappingContext;

public:
	//移动
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTSPawnInput")
	class UInputAction* MoveForwardAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTSPawnInput")
	class UInputAction* MoveRightAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTSPawnInput")
	class UInputAction* ZoomAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTSPawnInput")
	class UInputAction* KeyMouseRight;
};
