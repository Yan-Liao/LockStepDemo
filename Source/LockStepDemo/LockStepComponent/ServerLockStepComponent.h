// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ServerLockStepComponent.generated.h"

/** Action sent from clients.
*/
USTRUCT()
struct FClientActionInfo
{
	GENERATED_BODY()
public:
	FClientActionInfo() {
		PlayerId = -1;
	}

	/** PlayerId, equal to some PlayerState's PlayerId.
	*/
	UPROPERTY()
	int32 PlayerId;

	/** Client current sync step index
	*/
	UPROPERTY()
	uint32 ClientSyncStepIndex;

	/** Exec code
	*	Should be passed to PlayerController::ConsoleCommand.
	*/
	UPROPERTY()
	FString ExecCode;

	bool operator==(const FClientActionInfo& Other) const
	{
		if (this->PlayerId!=Other.PlayerId)return false;
		if (this->ExecCode!=Other.ExecCode)return false;
		// if (this->ClientSyncStepIndex!=Other.ClientSyncStepIndex)return false;  //不作为是否一致判断条件
		return true;
	}
	bool operator!=(const FClientActionInfo& Other) const
	{
		return !operator==(Other);
	}

	friend uint32 GetTypeHash(const FClientActionInfo& InActionInfo)
	{
		uint32 Hash = InActionInfo.ClientSyncStepIndex;
    
		// 将 FString 转换为 UTF-8 并计算 CRC
		FTCHARToUTF8 Convert(*InActionInfo.ExecCode);
		Hash = FCrc::MemCrc32(Convert.Get(), Convert.Length(), Hash);
    
		return Hash;
	}

};

USTRUCT()
struct FStepActionInfo
{
	GENERATED_BODY()
public:
	FStepActionInfo() {
		StepIndex = 0;
	}

	/** Step index. */
	UPROPERTY()
	uint32 StepIndex;

	/** Step actions. */
	UPROPERTY()
	TArray<FClientActionInfo> StepActions;

	bool operator==(const FStepActionInfo& Other) const
	{
		if (this->StepIndex!=Other.StepIndex)return false;
		
		// 使用TMap统计StepActions是否一致
		TMap<FClientActionInfo, int32> ElementCount;
		for (const FClientActionInfo& Element : this->StepActions)
		{
			if (Element.ExecCode.IsEmpty())continue;  //只对比有操作项是否都一致
			ElementCount.FindOrAdd(Element) += 1;
		}
		for (const FClientActionInfo& Element : Other.StepActions)
		{
			if (Element.ExecCode.IsEmpty())continue;
			int32* Count = ElementCount.Find(Element);
			if (!Count || *Count <= 0)
			{
				return false;
			}
			*Count -= 1;
		}
		for (const auto& EC: ElementCount)
		{
			if (EC.Value>0)
			{
				return false;
			}
		}
		return true;
	}

	bool operator!=(const FStepActionInfo& Other) const
	{
		return !operator==(Other);
	}

	friend uint32 GetTypeHash(const FStepActionInfo& InActionInfo)
	{
		return InActionInfo.StepIndex;
	}

};

/** Game lockstep server component.
*/
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOCKSTEPDEMO_API UServerLockStepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UServerLockStepComponent();

	/** Actions gathered from all clients of this frame. */
	UPROPERTY()
	FStepActionInfo ServerCurrentStepInfo;

	/** Elapsed time. */
	UPROPERTY()
	float FrameTimeAccum = 0;

	/** Server step automatic advanced. */
	UPROPERTY()
	uint32 bLockStepAutoAdvanced : 1;

	/** Is server lock step started. */
	UPROPERTY()
	uint32 bLockStepStarted : 1;

	///** Server step archives. */
	//UPROPERTY()
	//FArchive* ServerStepArchives;


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	/** Get step tick time.
	*	How much time to send a step advance from server.
	*/
	static float StaticGetStepAdvanceTime();

	/** Get step sync time multiply.
	*	How much time the client actions will sync to server.
	*/
	static float StaticGetActionsSyncTime();

	/** Notify lockstep sync start. Always be called in GameMode's StartPlay route.
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "VehicleAdvLockStep")
	void AuthStartLockStepGame();

	///** Manually do a step in server. And automatically set the server step to manual mode.
	//*/
	//UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "VehicleAdvLockStep")
	//void AuthManualStep(bool InUseManualStep = true);

	void SetPktLag(int32 PktLag);
	
private:
	FTimerHandle LogicTimerHandle;

	/** store step actions. */
	TArray<FStepActionInfo> StoreStepActions;
	
	/** Do server step advance. */
	void AuthDoStepAdvance();
};
