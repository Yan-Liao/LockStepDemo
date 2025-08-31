// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LockStepDemo/Interface/LockStepActorInterface.h"
#include "LockStepDemo/Interface/RTSUnitsInterface.h"
#include "LockStepPawn.generated.h"

UCLASS()
class LOCKSTEPDEMO_API ALockStepPawn : public APawn, public IRTSUnitsInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ALockStepPawn();
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void Units_Selected_Implementation(bool bSelected) override;

	virtual void Units_SetTargetLocation_Implementation(const FVector& InTargetLocation) override;

	virtual void Units_SetUnitState_Implementation(int32 InUnitState);

	virtual void Units_SetUnitLocation_Implementation(const FVector& InLocation) override;

	virtual FVector Units_GetUnitLocation_Implementation() override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
private:
	void StateMachineTick();  //人物切换状态
	void StopMove();
	void StateMoveToLocation();

private:
	int32 UnitState = 0;  //0: stop  1: move
	FVector TargetLocation = FVector::ZeroVector;
};
