// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ServerLockStepComponent.h"
#include "ClientLockStepComponent.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(StepDebug, Display, All);

struct Snapshot
{
public:
	TArray<uint32> UnitIds;
	TArray<FVector> UnitLocations;
};


UCLASS(ClassGroup=(Custom), Within=PlayerController)
class LOCKSTEPDEMO_API UClientLockStepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UClientLockStepComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	
	// /** Push client command.
	// */
	void AddCommand(const FString& InCmd);
	
	/** Run in client only, handling 'step advance' message from server. */
	UFUNCTION(Client, Reliable)  //RPC调用：Client上执行；可靠传输；带Validation实现，先执行Validation，成功后继续执行
	void S2C_StepAdvance(const TArray<FStepActionInfo>& InStepActionInfo);

	UFUNCTION(Client, Reliable)  //RPC调用：Client上执行；可靠传输；带Validation实现，先执行Validation，成功后继续执行
	void S2C_InitClient();

	/** Run in server only, handling client's step request. */
	UFUNCTION(Server, Reliable)
	void C2S_SetServerPktLag(int32 PktLag) const;
	
	//
	// # Common game exec functions
	//
	UFUNCTION(exec)
	void ExecMoveToLocation(const FString& UnitsIdsStr, float LocationX, float LocationY);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type InEndPlayReason) override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

private:

	/** world steps tick.
	*/
	void OnWorldPostActorTick(UWorld* InWorld, ELevelTick InTickType, float InDeltaSeconds);
	
	/** Process queued steps.
	*/
	void ProcessQueuedSteps();

	/** Update client physics.
	*/
	static void StaticUpdateClientPhysics(UWorld* InWorldContext, float InFrameTickTime);

	/** Run in client only, execute player action code.
	*/
	void ExecutePlayerAction(int32 InPlayerId, const FString& InExecCode);

	/** Run in server only, handling client's step request. */
	UFUNCTION(Server, Reliable)
	void C2S_RequestStep(const FClientActionInfo& InRequestActions) const;



private:
	/** PostActorTick delegate handle. */
	FDelegateHandle PostActorTickHandle;

	/** Client pawn. */
	class ARTSPawn* Pawn = nullptr;

private:
	
	//客户端logic frame Timer Handle
	FTimerHandle LogicTimerHandle;

	/** Client predict step index */
	UPROPERTY()
	uint32 ClientPredictStepIndex;

	/** Client has been Sync step index */
	UPROPERTY()
	uint32 ClientSyncStepIndex;
	
	/** Client actions for this step, will be sent to server, and do local predict. */
	UPROPERTY()
	FString ClientAction;

	/** Client queued steps by server*/
	UPROPERTY()
	TSet<FStepActionInfo> ClientQueuedStepsSet;

	UPROPERTY()
	TArray<FStepActionInfo> StoreClientActions;

	void ClientDoStepAdvance();

private:
	/** stored snapshots*/
	TArray<Snapshot> StoreSnapshots;

	bool RollBackToSyncStep(const TArray<uint32>& RollbackUnits);

	TArray<uint32> GetRollBackedUnitIds(const TArray<FClientActionInfo>& StoredStepActions, const TArray<FClientActionInfo>& NewStepActions);
};
