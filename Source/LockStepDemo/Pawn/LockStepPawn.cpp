// Fill out your copyright notice in the Description page of Project Settings.


#include "LockStepPawn.h"

#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"

// Sets default values
ALockStepPawn::ALockStepPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = false;
	
	// 遍历并关闭所有组件的复制
	TArray<UActorComponent*> Components;
	GetComponents(Components);
	for (UActorComponent* Comp : Components)
	{
		if (Comp)
		{
			Comp->SetIsReplicated(false);
		}
	}
}

// Called when the game starts or when spawned
void ALockStepPawn::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) return;

	// 创建AIController
	AAIController* AIController = World->SpawnActor<AAIController>(AAIController::StaticClass(), GetActorTransform());
	if (AIController)
	{
		//  possession
		AIController->Possess(this);
        
		UE_LOG(LogTemp, Log, TEXT("Spawned and possessed AIController on client"));
	}
	
}

void ALockStepPawn::StateMachineTick()
{
	if (UnitState==0)
	{
		StopMove();
	}else if (UnitState==1)
	{
		StateMoveToLocation();
	}
}

void ALockStepPawn::StopMove()
{
	//停止移动
	AAIController* AaiController = UAIBlueprintHelperLibrary::GetAIController(this);
	if (AaiController)
	{
		AaiController->StopMovement();
	}
}

void ALockStepPawn::StateMoveToLocation()
{
	AAIController* AaiController = UAIBlueprintHelperLibrary::GetAIController(this);
	if (AaiController)
	{
		EPathFollowingRequestResult::Type MoveToLocationResult = AaiController->MoveToLocation(TargetLocation);
		if (MoveToLocationResult==EPathFollowingRequestResult::AlreadyAtGoal)
		{
			UnitState = 0;
		}
	}
}

// Called every frame
void ALockStepPawn::Tick(float DeltaTime)
{
	// ENetRole Rool = GetLocalRole();
	// ENetRole NetMode = GetRemoteRole();
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Blue, FString::Format(TEXT("{0} {1}"), {Rool, NetMode}));
	// }
	
	Super::Tick(DeltaTime);
	StateMachineTick();
}

// Called to bind functionality to input
void ALockStepPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ALockStepPawn::Units_Selected_Implementation(bool bSelected){}

void ALockStepPawn::Units_SetTargetLocation_Implementation(const FVector& InTargetLocation)
{
	TargetLocation = InTargetLocation;
}

void ALockStepPawn::Units_SetUnitState_Implementation(int32 InUnitState)
{
	UnitState = InUnitState;
}

void ALockStepPawn::Units_SetUnitLocation_Implementation(const FVector& InLocation)
{
	if (InLocation!=GetActorLocation())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Format(TEXT("CurrentLocation: {0} {1} {2}  RollBackLocation: {3} {4} {5}"),
                         {GetActorLocation().X, GetActorLocation().Y,GetActorLocation().Z, InLocation.X, InLocation.Y, InLocation.Z}));
		}
		SetActorLocation(InLocation);
		UnitState = 0;
	}

	
	
}

FVector ALockStepPawn::Units_GetUnitLocation_Implementation()
{
	return GetActorLocation();
}

