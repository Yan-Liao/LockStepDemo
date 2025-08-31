// Fill out your copyright notice in the Description page of Project Settings.


#include "ClientLockStepComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameState.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "PhysicsPublic.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Containers/StringConv.h"
#include "LockStepDemo/Interface/LockStepActorInterface.h"
#include "LockStepDemo/Core/PlayerPawn/RTSPawn.h"

DEFINE_LOG_CATEGORY(StepDebug);

// Sets default values for this component's properties
UClientLockStepComponent::UClientLockStepComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UClientLockStepComponent::BeginPlay()
{
	Super::BeginPlay();

	// Close the world's default physics simulation.
	check(GetWorld());
	GetWorld()->bShouldSimulatePhysics = false;
	
}

void UClientLockStepComponent::EndPlay(EEndPlayReason::Type InEndPlayReason)
{
	Super::EndPlay(InEndPlayReason);
}

void UClientLockStepComponent::OnRegister()
{
	Super::OnRegister();

	// // Attach tick delegate.
	// if (!PostActorTickHandle.IsValid()) {
	// 	//绑定多播委托，当world所有actor的tick完成后执行
	// 	PostActorTickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UClientLockStepComponent::OnWorldPostActorTick);
	// }
}

void UClientLockStepComponent::OnUnregister()
{
	Super::OnUnregister();

	// // Detach tick delegate.
	// if (PostActorTickHandle.IsValid()) {
	// 	FWorldDelegates::OnWorldPostActorTick.Remove(PostActorTickHandle);
	// 	PostActorTickHandle.Reset();
	// }
}


// Called every frame
void UClientLockStepComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UClientLockStepComponent::AddCommand(const FString& InCmd)
{
	ClientAction = InCmd;  //简化 每帧只记录最后一次操作
	// C2S_RequestStep(InCmd);
}

void UClientLockStepComponent::C2S_RequestStep_Implementation(const FClientActionInfo& InRequestActions)const
{
	// Add client's request to game state's action list.
	APlayerController* PlayerController = CastChecked<APlayerController>(GetOwner());
	check(GetWorld());
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	checkf(GameMode, TEXT("C2S_RequestStep must be called with authority."));

	UServerLockStepComponent* GameLockStepComp = GameMode->FindComponentByClass<UServerLockStepComponent>();
	if (ensureMsgf(GameLockStepComp, TEXT("UClientLockStepComponent must pair with UServerLockStepComponent."))) {
		// Add actions to current step's action list.
		GameLockStepComp->ServerCurrentStepInfo.StepActions.Add(InRequestActions);
	}
}

void UClientLockStepComponent::S2C_InitClient_Implementation()
{
	check(GetWorld());
	// Do nothing for dedicated server, only run on clients.
	if (GetWorld()->GetNetMode() == NM_DedicatedServer) {
		return;     
	}
	
	APlayerController* PlayerController = CastChecked<APlayerController>(GetOwner());
	Pawn = CastChecked<ARTSPawn>(PlayerController->GetPawn());
	Pawn->RegisterClientComponent(this);

	GetWorld()->GetTimerManager().SetTimer(
		LogicTimerHandle,                    
		this,                            // 回调函数所属的对象
		&UClientLockStepComponent::ClientDoStepAdvance,
		UServerLockStepComponent::StaticGetStepAdvanceTime(),
		true 
	);

	ClientPredictStepIndex = 0;
	ClientSyncStepIndex = 0;
	FStepActionInfo Tmp0;
	Tmp0.StepIndex = 0;
	StoreClientActions.Add(Tmp0);  //0帧默认无操作
	
	//初始位置快照
	StoreSnapshots.Add(Pawn->GetSnapshot());
}

void UClientLockStepComponent::ExecMoveToLocation(const FString& UnitsIdsStr, float LocationX, float LocationY)
{
	const float DeltaTime = UServerLockStepComponent::StaticGetStepAdvanceTime();
	
	if (IsValid(Pawn))
	{
		Pawn->ExeMoveToLocationCommand(UnitsIdsStr, LocationX, LocationY);
	}
	
}




void UClientLockStepComponent::OnWorldPostActorTick(UWorld* InWorld, ELevelTick InTickType, float InDeltaSeconds)
{
	if (InWorld != this->GetWorld()) {
		return;
	}

	// Do nothing for dedicated server, only run on clients.
	if (InWorld->GetNetMode() == NM_DedicatedServer) {
		return;     
	}

	// Try do client steps.
	ProcessQueuedSteps();
}

void UClientLockStepComponent::ProcessQueuedSteps()
{
	//1.同步
	//先对ClientQueuedSteps按帧号排序
	TArray<FStepActionInfo>ClientQueuedSteps = ClientQueuedStepsSet.Array();
	ClientQueuedSteps.Sort([](const FStepActionInfo& LHS, const FStepActionInfo& RHS)
	{
		return LHS.StepIndex < RHS.StepIndex;
	});

	
	for (const auto& StepActionInfo: ClientQueuedSteps)
	{
		// # Client do step advance immediately.
		
		// Check if Step index matched.
		if (StepActionInfo.StepIndex <= ClientSyncStepIndex) {
			continue;  //网络延迟导致 该帧丢弃
		}

		//带同步帧排序后，不应跳帧
		if (StepActionInfo.StepIndex != ClientSyncStepIndex+1)
		{
			UE_LOG(LogScript, Error, TEXT("Error : StepAdvance Message cannot match client state!"));
			return;
		}
		
		//判断是否需要回滚
		if (ClientPredictStepIndex >= StepActionInfo.StepIndex)
		{
			//预测执行与同步帧不一致
			FStepActionInfo tmp = StoreClientActions[StepActionInfo.StepIndex];
			if (tmp!=StepActionInfo)
			{
				//进行回滚
				TArray<uint32> RollbackUnits = GetRollBackedUnitIds(tmp.StepActions, StepActionInfo.StepActions);
				RollBackToSyncStep(RollbackUnits);
				
				UE_LOG(StepDebug, Display, TEXT("%s %p step:%d  rollback"),
							*this->GetName(), this, StepActionInfo.StepIndex);

				for (const auto& sa: StepActionInfo.StepActions)
				{
					if (!sa.ExecCode.IsEmpty())
					{
						UE_LOG(StepDebug, Display, TEXT("%s %p step:%d Move :%s New"),
							*this->GetName(), this, StepActionInfo.StepIndex, *sa.ExecCode);
					}
				}

				for (const auto& sa: tmp.StepActions)
				{
					if (!sa.ExecCode.IsEmpty())
					{
						UE_LOG(StepDebug, Display, TEXT("%s %p step:%d Move :%s Store"),
							*this->GetName(), this, StepActionInfo.StepIndex, *sa.ExecCode);
					}
				}
				
			}else  //预测与同步帧一致
			{
				for (const auto& sa: tmp.StepActions)
				{
					if (!sa.ExecCode.IsEmpty())
					{
						UE_LOG(StepDebug, Display, TEXT("%s %p step:%d Move :%s  step is same"),
							*this->GetName(), this, StepActionInfo.StepIndex, *sa.ExecCode);
					}
				}
				//快照
				StoreSnapshots.Add(Pawn->GetSnapshot());
				ClientSyncStepIndex = StepActionInfo.StepIndex;
				continue;
			}
		}
		
		// # Execute all actions got from server.
		for (int ActionIndex = 0; ActionIndex < StepActionInfo.StepActions.Num(); ActionIndex++) {
			const FClientActionInfo& ActionInfo = StepActionInfo.StepActions[ActionIndex];

			// Execute player action.
			ExecutePlayerAction(ActionInfo.PlayerId, ActionInfo.ExecCode);
		}

		// Client now has the same step index with the server.
		//快照
		StoreSnapshots.Add(Pawn->GetSnapshot());
		ClientSyncStepIndex = StepActionInfo.StepIndex;
		//更新历史action
		if (ClientPredictStepIndex >= StepActionInfo.StepIndex)
		{
			StoreClientActions[StepActionInfo.StepIndex] = StepActionInfo;
		}else
		{
			StoreClientActions.Add(StepActionInfo);
		}
	}
	ClientQueuedStepsSet.Empty();

	//2.收集的操作发送给server
	APlayerController* PlayerController = CastChecked<APlayerController>(GetOwner());
	FClientActionInfo ActionInfo;
	ActionInfo.PlayerId = PlayerController->PlayerState->GetPlayerId();
	ActionInfo.ClientSyncStepIndex = ClientSyncStepIndex;
	ActionInfo.ExecCode = ClientAction;
	C2S_RequestStep(ActionInfo);
	ClientAction = "";

	//3.预测 已当前操作作为预测，其余player简易实现默认无操作
	ExecutePlayerAction(ActionInfo.PlayerId, ActionInfo.ExecCode);
	ClientPredictStepIndex = ClientSyncStepIndex>ClientPredictStepIndex?ClientSyncStepIndex+1:ClientPredictStepIndex+1;
	FStepActionInfo PredictStepAction;
	PredictStepAction.StepIndex = ClientPredictStepIndex;
	PredictStepAction.StepActions.Add(ActionInfo);
	check(static_cast<int32>(ClientPredictStepIndex) >= StoreClientActions.Num())
	StoreClientActions.Add(PredictStepAction);
	
	
}

void UClientLockStepComponent::StaticUpdateClientPhysics(UWorld* InWorldContext, float InFrameTickTime)
{
	const float FrameTickTime = InFrameTickTime;


	// Update physics scene.
	FPhysScene* PhysScene = InWorldContext->GetPhysicsScene();

#if WITH_PHYSX
	// When ticking the main scene, clean up any physics engine resources (once a frame)
	DeferredPhysResourceCleanup();
#endif

	// Update gravity in case it changed
	const FVector Gravity(0, 0, UPhysicsSettings::Get()->DefaultGravityZ);
	// PhysScene->SetUpForFrame(&Gravity, FrameTickTime, FrameTickTime);
	//
	// // Scene start / wait / end routes.
	// PhysScene->StartFrame();
	// PhysScene->WaitPhysScenes();
	// PhysScene->EndFrame(nullptr);

	// TODO: Do some clamp to make it same with all clients.
}

void UClientLockStepComponent::ExecutePlayerAction(int32 InPlayerId, const FString& InExecCode)
{
	// Check and run console command in client.
	ProcessConsoleExec(*InExecCode, *GLog, nullptr);
}

void UClientLockStepComponent::C2S_SetServerPktLag_Implementation(int32 PktLag) const
{
	APlayerController* PlayerController = CastChecked<APlayerController>(GetOwner());
	check(GetWorld());
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	checkf(GameMode, TEXT("C2S_RequestStep must be called with authority."));

	UServerLockStepComponent* GameLockStepComp = GameMode->FindComponentByClass<UServerLockStepComponent>();
	if (ensureMsgf(GameLockStepComp, TEXT("UClientLockStepComponent must pair with UServerLockStepComponent."))) {
		// Add actions to current step's action list.
		GameLockStepComp->SetPktLag(PktLag);
	}
}

void UClientLockStepComponent::ClientDoStepAdvance()
{
	//处理服务器下发的帧操作
	ProcessQueuedSteps();
}

bool UClientLockStepComponent::RollBackToSyncStep(const TArray<uint32>& RollbackUnits)
{
	if (StoreSnapshots.Num()>static_cast<int32>(ClientSyncStepIndex))
	{
		TArray<uint32>& UnitIds = StoreSnapshots[ClientSyncStepIndex].UnitIds;
		TArray<FVector>& UnitLocations = StoreSnapshots[ClientSyncStepIndex].UnitLocations;
		for (int i=0;i<UnitIds.Num();++i)
		{
			if (RollbackUnits.Find(UnitIds[i])!=INDEX_NONE)
			{
				Pawn->SetUnitToLocation(UnitIds[i], UnitLocations[i]);
			}
			
		}
		return true;
	}
	return false;
}

TArray<uint32> UClientLockStepComponent::GetRollBackedUnitIds(const TArray<FClientActionInfo>& StoredStepActions,
	const TArray<FClientActionInfo>& NewStepActions)
{
	//通过map筛选出不同操作的unit
	TMap<FClientActionInfo, int32> ElementCount;
	for (const FClientActionInfo& Element : StoredStepActions)
	{
		if (Element.ExecCode.IsEmpty())continue;  //只对比有操作项是否都一致
		ElementCount.FindOrAdd(Element) += 1;
	}
	for (const FClientActionInfo& Element : NewStepActions)
	{
		if (Element.ExecCode.IsEmpty())continue;
		ElementCount.FindOrAdd(Element, 0)--;
	}
	TArray<FString> ExecCodeArray;
	for (const auto& EC: ElementCount)
	{
		if (EC.Value!=0)
		{
			ExecCodeArray.Add(EC.Key.ExecCode);
		}
	}

	TArray<uint32> RollBackedUnitIds;
	for (const FString& ExecCode : ExecCodeArray)
	{
		if (ExecCode.IsEmpty())continue;
		TArray<FString> Parts;
		ExecCode.ParseIntoArrayWS(Parts); // 按空白字符分割
    
		// 查找包含逗号分隔数字的部分
		if (Parts.Num()<4)continue;
		TArray<FString> NumberStrings;
		Parts[1].ParseIntoArray(NumberStrings, TEXT(","));
            
		for (const FString& NumStr : NumberStrings)
		{
			if (NumStr.IsNumeric())
			{
				RollBackedUnitIds.Add(FCString::Atoi(*NumStr));
			}
		}
	}

	return RollBackedUnitIds;
}

void UClientLockStepComponent::S2C_StepAdvance_Implementation(const TArray<FStepActionInfo>& InStepActionInfo)
{
	// Push step info to the queue.
	ClientQueuedStepsSet.Append(InStepActionInfo);
}




