// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "LockStepDemo/Core/PlayerController/RTSPlayerController.h"
#include "EnhancedInputComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "LockStepDemo/LockStepComponent/ClientLockStepComponent.h"
#include "LockStepDemo/Utils/RTSFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "EngineUtils.h"
#include "LockStepDemo/Pawn/LockStepPawnManager/LockStepPawnManager.h"
#include "LockStepDemo/Interface/RTSUnitsInterface.h"

// Sets default values
ARTSPawn::ARTSPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//初始化组件
	SceneComponent = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(SceneComponent);
	
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>("CameraArm");
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->SetRelativeRotation(FRotator(-60, 0, 0));
	SpringArmComponent->TargetArmLength = 1000;
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("Camera");
	CameraComponent->SetupAttachment(SpringArmComponent);

	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>("FloatingPawnMovement");
}

// Called when the game starts or when spawned
void ARTSPawn::BeginPlay()
{
	Super::BeginPlay();
	
	RegisterPawnManager();
}

void ARTSPawn::MoveForward(const FInputActionValue& Value)
{
	float MoveForwardVal = Value.Get<float>();
	
	AddMovementInput(GetActorForwardVector(), MoveForwardVal);
	
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, -1, FColor::Red, FString::Printf(TEXT("forward %f %f %f"), GetActorForwardVector()[0], GetActorForwardVector()[1], GetActorForwardVector()[2]));
	// 	GEngine->AddOnScreenDebugMessage(-1, -1, FColor::Blue, FString::Printf(TEXT("forward %f"), MoveForwardVal));
	// }
}

void ARTSPawn::MoveRight(const FInputActionValue& Value)
{
	float MoveRightVal = Value.Get<float>();
	AddMovementInput(GetActorRightVector(), MoveRightVal);
	
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, -1, FColor::Red, FString::Printf(TEXT("right %f %f %f"), GetActorRightVector()[0], GetActorRightVector()[1], GetActorRightVector()[2]));
	// 	GEngine->AddOnScreenDebugMessage(-1, -1, FColor::Blue, FString::Printf(TEXT("right %f"), MoveRightVal));
	// }
}

void ARTSPawn::Zoom(const FInputActionValue& Value)
{
	// float ZoomVal = Value.Get<float>();
	// ZoomVal = SpringArmComponent->TargetArmLength + ZoomVal*ZoomSpeed;
	// if (ZoomVal > ZoomMax)
	// {
	// 	ZoomVal = ZoomMax;
	// }else if (ZoomVal < ZoomMin)
	// {
	// 	ZoomVal = ZoomMin;
	// }
	// SpringArmComponent->TargetArmLength = ZoomVal;

	float ZoomVal = Value.Get<float>();
	if (LockStepComponent)
	{
		static int32 Val = 0;
		Val += ZoomVal*100;
		Val = Val<0?0:Val;
		LockStepComponent->C2S_SetServerPktLag(Val);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(0, 5, FColor::Red, FString::Format(TEXT("SetServerPktLag {0}"), {Val}));
		}
	}
	
}

void ARTSPawn::KeyMouseRightStarted()
{
	MoveToLocationCommand();
}

void ARTSPawn::MoveToLocationCommand()
{
	TArray<AActor*> CurrentSelectedUnits = URTSFunctionLibrary::GetCurrentSelectedUnits(this);
	if (CurrentSelectedUnits.Num() > 0)
	{
		FVector TargetLocation;
		AActor* TargetActor;
		if (GetUnderCursorTarget(TargetLocation, TargetActor))
		{
			//鼠标点击特效
			UNiagaraSystem* NiagaraTemplate = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Resource/Cursor/FX_Cursor.FX_Cursor"));
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, NiagaraTemplate, TargetLocation);
			
			if (IsValid(LockStepPawnManager))
			{
				//获取pawn编号
				TArray<uint32> UnitsId = LockStepPawnManager->GetIdsByLockStepPawns(CurrentSelectedUnits);
				
				FString CMD = SerializeCMD("ExecMoveToLocation", UnitsId, TargetLocation.X, TargetLocation.Y);

				//缓存命令
				if (LockStepComponent)
				{
					LockStepComponent->AddCommand(CMD);
				}
			}
		}
		
	}
}

void ARTSPawn::MoveOnCursorNearScreenEdge()
{
	FVector2D MousePositionOnViewport = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
	FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const float EdgeThreshold = 10.f;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(6, 0, FColor::Green, FString::Printf(TEXT("ViewPort x: %f, y: %f"), ViewportSize.X, ViewportSize.Y));
	}
	
	ViewportSize/=ViewportScale;  //实际像素大小转换为逻辑像素大小

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0, FColor::Blue, FString::Printf(TEXT("MousePoint x: %f, y: %f"), MousePositionOnViewport.X, MousePositionOnViewport.Y));
		GEngine->AddOnScreenDebugMessage(2, 0, FColor::Red, FString::Printf(TEXT("LogicPort x: %f, y: %f"), ViewportSize.X, ViewportSize.Y));
		GEngine->AddOnScreenDebugMessage(3, 0, FColor::Yellow, FString::Printf(TEXT("scale: %f"), ViewportScale));
	}
	// 检查是否接近左边缘
	if (MousePositionOnViewport.X <= EdgeThreshold)
	{
		//只要确保相机和actor的前方向（x轴）相对位置不变，这么实现就没问题。
		AddMovementInput(GetActorRightVector(), -1);
		
	}else if (MousePositionOnViewport.X >= (ViewportSize.X - EdgeThreshold))
	{
		AddMovementInput(GetActorRightVector(), 1);
	}
    
	// 检查是否接近上边缘
	if (MousePositionOnViewport.Y <= EdgeThreshold)
	{
		AddMovementInput(GetActorForwardVector(), 1);
		
	}else if (MousePositionOnViewport.Y >= (ViewportSize.Y - EdgeThreshold))
	{
		AddMovementInput(GetActorForwardVector(), -1);
		
	}
}

bool ARTSPawn::GetUnderCursorTarget(FVector& TargetLocation, class AActor*& TargetActor)
{
	APlayerController* LocalViewingPlayerController = GetLocalViewingPlayerController();
	if (LocalViewingPlayerController)
	{
		//ECC_GameTraceChannel5为Landscape
		const TArray<TEnumAsByte<EObjectTypeQuery> > ObjectTypes = {UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel5), UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1)};
		FHitResult HitResult;
		bool bHit = LocalViewingPlayerController->GetHitResultUnderCursorForObjects(ObjectTypes, false, HitResult);
		if (bHit)
		{
			TargetLocation = HitResult.Location;
			TargetActor = HitResult.GetActor();
			return true;
		}else
		{
			return false;
		}
	}
	return false;
}

FString ARTSPawn::SerializeCMD(const FString& EXE_FuncName, const TArray<uint32>& UnitIds, float Value1, float Value2)
{
	FString ArrayStr;
	for (int32 i = 0; i < UnitIds.Num(); ++i)
	{
		ArrayStr += FString::FromInt(UnitIds[i]);
		if (i < UnitIds.Num() - 1)
		{
			ArrayStr += TEXT(",");
		}
	}

	// 序列化 float 部分（空格分隔）
	FString FloatStr = FString::Printf(TEXT("%.1f %.1f"), Value1, Value2);

	// 合并结果
	return EXE_FuncName + " " + ArrayStr + " " + FloatStr;
}

void ARTSPawn::ExeMoveToLocationCommand(const FString& UnitsIdsStr, float LocationX, float LocationY)
{
	TArray<FString> StringArray;
	UnitsIdsStr.ParseIntoArray(StringArray, TEXT(","), true);

	TArray<uint32> UnitIds;
	for (const FString& NumStr : StringArray)
	{
		// 去除可能存在的空格（可选，取决于你的字符串干净程度）
		FString TrimmedStr = NumStr.TrimStartAndEnd();
    
		uint32 ParsedNumber;
		// LexTryParseString 是安全转换的最佳方式
		if (LexTryParseString(ParsedNumber, *TrimmedStr))
		{
			UnitIds.Add(ParsedNumber);
		}
		else
		{
			// 转换失败处理（记录错误等）
			UE_LOG(LogTemp, Warning, TEXT("Failed to parse number: %s"), *TrimmedStr);
		}
	}
	if (LockStepPawnManager)
	{
		TArray<AActor*> Units = LockStepPawnManager->GetLockStepPawnsByIds(UnitIds);
		TArray<FVector> TargetLocationArray = URTSFunctionLibrary::GetUnitsGoalsInPhalanx(Units, {LocationX, LocationY, 0.f}, UnitsBetweenSpace);
		int idx = 0;
		for (auto Unit: Units)
		{
			if (IsValid(Unit))
			{
				IRTSUnitsInterface::Execute_Units_SetUnitState(Unit, 1);
				IRTSUnitsInterface::Execute_Units_SetTargetLocation(Unit, TargetLocationArray[idx]);
			}
			++idx;
		}
	}
}

void ARTSPawn::RegisterClientComponent(class UClientLockStepComponent* ClientComponent)
{
	LockStepComponent = ClientComponent;
}

void ARTSPawn::RegisterPawnManager()
{
	//获取pawnManager
	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<ALockStepPawnManager> It(World); It; ++It)
		{
			LockStepPawnManager = *It;  //简易实现，确保只放置一个LockStepPawnManager
			break;
		}
	}
}

void ARTSPawn::SetUnitToLocation(uint32 UnitId, const FVector& Location)
{
	if (LockStepPawnManager)
	{
		//注意如果unit已经删除，当前实现无法回滚
		AActor* Unit = LockStepPawnManager->GetLockStepPawnById(UnitId);
		if (IsValid(Unit))
		{
			IRTSUnitsInterface::Execute_Units_SetUnitLocation(Unit, Location);
		}
	}
}

Snapshot ARTSPawn::GetSnapshot()
{
	Snapshot Result;
	if (LockStepPawnManager)
	{
		TMap<uint32, AActor*> Units = LockStepPawnManager->GetAllUnits();
		
		for (auto Unit: Units)
		{
			if (IsValid(Unit.Value))
			{
				Result.UnitIds.Add(Unit.Key);
				Result.UnitLocations.Add(IRTSUnitsInterface::Execute_Units_GetUnitLocation(Unit.Value));
			}
		}
	}
	return Result;
}


// Called every frame
void ARTSPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// MoveOnCursorNearScreenEdge();
}

// Called to bind functionality to input
void ARTSPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(GetController());
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInputComponent && PlayerController  && PlayerController->MoveForwardAction)
	{
		EnhancedInputComponent->BindAction(PlayerController->MoveForwardAction, ETriggerEvent::Triggered,this,&ARTSPawn::MoveForward);
	}
	
	if (EnhancedInputComponent && PlayerController  && PlayerController->MoveRightAction)
	{
		EnhancedInputComponent->BindAction(PlayerController->MoveRightAction, ETriggerEvent::Triggered,this,&ARTSPawn::MoveRight);
	}
	
	if (EnhancedInputComponent && PlayerController  && PlayerController->ZoomAction)
	{
		EnhancedInputComponent->BindAction(PlayerController->ZoomAction, ETriggerEvent::Triggered,this,&ARTSPawn::Zoom);
	}

	if (EnhancedInputComponent && PlayerController  && PlayerController->KeyMouseRight)
	{
		EnhancedInputComponent->BindAction(PlayerController->KeyMouseRight, ETriggerEvent::Started,this,&ARTSPawn::KeyMouseRightStarted);
	}

}

