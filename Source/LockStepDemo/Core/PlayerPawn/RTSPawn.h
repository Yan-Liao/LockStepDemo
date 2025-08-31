// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "LockStepDemo/Interface/RTSPawnInterface.h"
#include "RTSPawn.generated.h"

UCLASS()
class LOCKSTEPDEMO_API ARTSPawn : public APawn, public IRTSPawnInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARTSPawn();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleDefaultsOnly)
	class USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleDefaultsOnly)
	class UCameraComponent* CameraComponent;

	UPROPERTY(VisibleDefaultsOnly)
	class USceneComponent* SceneComponent;

	UPROPERTY(VisibleDefaultsOnly)
	class UFloatingPawnMovement* FloatingPawnMovement;

	UPROPERTY(EditAnywhere)
	float ZoomSpeed = 50;

	UPROPERTY(EditAnywhere)
	float ZoomMax = 3000;

	UPROPERTY(EditAnywhere)
	float ZoomMin = 600;

	UPROPERTY(EditAnywhere)
	float UnitsBetweenSpace = 200;

private:
	void MoveForward(const FInputActionValue& Value);

	void MoveRight(const FInputActionValue& Value);

	void Zoom(const FInputActionValue& Value);

	//鼠标右键按下
	void KeyMouseRightStarted();

	void MoveToLocationCommand();
	
	void MoveOnCursorNearScreenEdge();

	UFUNCTION(BlueprintCallable, meta=(AllowPrivateAccess="true"))
	bool GetUnderCursorTarget(FVector& TargetLocation, class AActor*& TargetActor);

	FString SerializeCMD(const FString& EXE_FuncName, const TArray<uint32>& UnitIds, float Value1, float Value2);

	void ExeMoveToLocationCommand(const FString& UnitsIdsStr, float LocationX, float LocationY);

	void RegisterClientComponent(class UClientLockStepComponent* ClientComponent);

	void RegisterPawnManager();

	void SetUnitToLocation(uint32 UnitId, const FVector& Location);

	struct Snapshot GetSnapshot();
private:
	class UClientLockStepComponent* LockStepComponent = nullptr;
	class ALockStepPawnManager* LockStepPawnManager = nullptr;

	friend UClientLockStepComponent;
};
