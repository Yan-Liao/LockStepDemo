// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerLockStepComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameState.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "ClientLockStepComponent.h"
#include "IpNetDriver.h"

// Sets default values for this component's properties
UServerLockStepComponent::UServerLockStepComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	bLockStepAutoAdvanced = 1;
	bLockStepStarted = 0;
}


// Called when the game starts
void UServerLockStepComponent::BeginPlay()
{
	Super::BeginPlay();

	bool bWorldSupport = false;
	UWorld* World = GetWorldChecked(bWorldSupport);
	
	// In server mode, Close all in-map actor's replication and tick,
	// as it may be set in default but cannot be handled in lock-step state.
	if (World->GetNetMode() == NM_DedicatedServer || World->GetNetMode() == NM_ListenServer) {
		for (TActorIterator<AActor> It(World); It; ++It) {
			AActor* Actor = *It;
	
			// All GameState, GameMode, PlayerController, and PlayerState will stay in default sync route.
			if (!Actor || Actor->IsPendingKillPending()
				|| Actor->IsA(AGameStateBase::StaticClass())
				|| Actor->IsA(AGameModeBase::StaticClass())
				|| Actor->IsA(APlayerController::StaticClass())
				|| Actor->IsA(APlayerState::StaticClass())
				) {
				continue;
				}
	
			Actor->SetReplicates(false);
			Actor->SetActorTickEnabled(false);
		}
	}
	
	// Close world's default physics simulation route.
	// We do physics simulate in client's step advance.
	World->bShouldSimulatePhysics = false;
	
}


// Called every frame
void UServerLockStepComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// // If in server-mode, 
	// if (GetOwner() && GetOwner()->HasAuthority() && bLockStepStarted) {
	// 	if (bLockStepAutoAdvanced) {
	// 		// Do server step advance and tick.
	// 		const float StepAdvTime = StaticGetStepAdvanceTime();
	//
	// 		// Accumulate delta time and check how much frames need to step.
	// 		FrameTimeAccum += DeltaTime;
	// 		while (FrameTimeAccum > StepAdvTime) {
	// 			// Go steps by time.
	// 			FrameTimeAccum -= StepAdvTime;
	//
	// 			// Do step advance.
	// 			AuthDoStepAdvance();
	// 		}
	// 	}
	// }
}

float UServerLockStepComponent::StaticGetStepAdvanceTime()
{
	return 5e-2f;
}

float UServerLockStepComponent::StaticGetActionsSyncTime()
{
	return 2e-1f;
}

void UServerLockStepComponent::AuthStartLockStepGame()
{
	// This method should only be called once during a game.
	if (bLockStepStarted) {
		return;
	}
	bool bWorldSupported = false;
	UWorld* World = GetWorldChecked(bWorldSupported);

	// Check if world is authority.
	checkf(World->GetAuthGameMode(), TEXT("AuthStartLockStepGame must be called with authority."));

	for (TPlayerControllerIterator<APlayerController>::ServerAll It(World); It; ++It) {
		APlayerController* PlayerCtrl = *It;

		// Ensure the player has a ClientLockStepComponent.
		UClientLockStepComponent* LockStepComp = PlayerCtrl->FindComponentByClass<UClientLockStepComponent>();
		if (!LockStepComp) {
			// New Component
			static FName Name_LockStepComp(TEXT("LockStepComp"));
			LockStepComp = ::NewObject<UClientLockStepComponent>(PlayerCtrl, UClientLockStepComponent::StaticClass(), Name_LockStepComp);

			// Tell Actor it has a new component.
			PlayerCtrl->AddOwnedComponent(LockStepComp);

			// Register the new component to the actor.
			LockStepComp->RegisterComponent();
		}
		// Let it replicable.
		if (ensure(LockStepComp)) {
			LockStepComp->SetIsReplicated(true);
			LockStepComp->S2C_InitClient();
		}
	}

	// Set LockStepStarted to true
	bLockStepStarted = 1;
	StoreStepActions.Add(ServerCurrentStepInfo);  //0帧默认空
	
	GetWorld()->GetTimerManager().SetTimer(
		LogicTimerHandle,                    
		this,                            // 回调函数所属的对象
		&UServerLockStepComponent::AuthDoStepAdvance,
		StaticGetStepAdvanceTime(),
		true 
	);
}

void UServerLockStepComponent::SetPktLag(int32 PktLag)
{
	if (GetWorld())
	{
		// 获取NetDriver
		UNetDriver* NetDriver = GetWorld()->GetNetDriver();
		if (NetDriver)
		{
			// 创建网络模拟配置
			FPacketSimulationSettings EmulationSettings;
                
			// 设置你想要的延迟（单位：毫秒）
			EmulationSettings.PktLag = PktLag;  // 最小延迟
                
			// 应用设置
			NetDriver->SetPacketSimulationSettings(EmulationSettings);
			
		}
	}
}

void UServerLockStepComponent::AuthDoStepAdvance()
{
	bool bWorldSupported = false;
	UWorld* World = GetWorldChecked(bWorldSupported);

	// Check if world is authority.
	if (!World->GetAuthGameMode()) {
		UE_LOG(LogScript, Error, TEXT("AuthDoStepAdvance must be called in authority mode."));
		return;
	}
	
	// Mark the new step info.
	ServerCurrentStepInfo.StepIndex++;
	
	//Save old step info to the store.
	//TODO 验证下收到的操作是否合法，比如延迟收到的操作
	StoreStepActions.Add(ServerCurrentStepInfo);

	for (const auto& sa: ServerCurrentStepInfo.StepActions)
	{
		if (!sa.ExecCode.IsEmpty())
		{
			UE_LOG(StepDebug, Display, TEXT("%s %p step:%d Move :%s"),
				*this->GetName(), this, ServerCurrentStepInfo.StepIndex, *sa.ExecCode);
		}
	}
	
	// Tell all player to step the advance.
	for (TPlayerControllerIterator<APlayerController>::ServerAll It(World); It; ++It) {
		APlayerController* PlayerCtrl = *It;

		UClientLockStepComponent* LockStepComp = PlayerCtrl->FindComponentByClass<UClientLockStepComponent>();
		if (!LockStepComp) {
			UE_LOG(LogScript, Error, TEXT("AuthDoStepAdvance : The game has started with some invalid players."));
			continue;
		}

		int32 PlayerStepIndex = -1;
		for (const auto& StepAction: ServerCurrentStepInfo.StepActions)
		{
			check(StepAction.ClientSyncStepIndex<ServerCurrentStepInfo.StepIndex);  //奇异数据客户端step的比服务器还快
			//找到当前player的client传来的帧数最大值（为客户端已同步帧数）（最大值是防止接收到之前的包），发送客户端已同步帧数->当前帧数
			if (StepAction.PlayerId==PlayerCtrl->PlayerState->GetPlayerId())
			{
				PlayerStepIndex = static_cast<int32>(StepAction.ClientSyncStepIndex)>PlayerStepIndex?static_cast<int32>(StepAction.ClientSyncStepIndex):PlayerStepIndex;
			}
		}
		
		TArray<FStepActionInfo> CurrentPlayerAsnycActions;
		if (PlayerStepIndex>-1)
		{
			for (int32 i= PlayerStepIndex+1; i<=static_cast<int32>(ServerCurrentStepInfo.StepIndex);++i)
			{
				CurrentPlayerAsnycActions.Add(StoreStepActions[i]);
			}
			LockStepComp->S2C_StepAdvance(CurrentPlayerAsnycActions);
		}
	}

	// Cleanup current step datas.
	ServerCurrentStepInfo.StepActions.Empty(256);
}

