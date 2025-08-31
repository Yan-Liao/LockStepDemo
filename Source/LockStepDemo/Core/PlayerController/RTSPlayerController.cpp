// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSPlayerController.h"
#include "EnhancedInputSubsystems.h"

ARTSPlayerController::ARTSPlayerController()
{
	bShowMouseCursor = true;  //显示鼠标
}

void ARTSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	//增强输入设置
	UEnhancedInputLocalPlayerSubsystem* EnhancedInputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer());
	if (EnhancedInputSystem && RTSPawnMappingContext)
	{
		EnhancedInputSystem->AddMappingContext(RTSPawnMappingContext,0);
	}
}
