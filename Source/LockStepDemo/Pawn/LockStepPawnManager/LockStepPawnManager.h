// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LockStepPawnManager.generated.h"

class ALockStepPawn;

UCLASS()
class LOCKSTEPDEMO_API ALockStepPawnManager : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<ALockStepPawn> BPLockStepPawn; // 可拖入蓝图类

	UPROPERTY(EditAnywhere)
	uint32 PawnNum; // 生成pawn数量
	
	UPROPERTY(EditAnywhere)
	float BetweenSpace; // 间隔大小

	UPROPERTY(EditAnywhere)
	FVector PawnLocation; // 生成pawn位置
	
public:
	// Sets default values for this actor's properties
	ALockStepPawnManager();
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void CreateLockStepPawn(const FVector& SpawnLocation, float Space, uint32 SpawnNum);

	void RemoveLockStepPawn(uint32 LockStepPawnId);

	void RemoveLockStepPawn(AActor* LockStepPawn);
	
	bool GetIdByLockStepPawn(const AActor* const LockStepPawn, uint32& LockStepPawnId);

	TArray<uint32> GetIdsByLockStepPawns(const TArray<AActor*>& LockStepPawns);
	
	AActor* GetLockStepPawnById(uint32 LockStepPawnId);

	TArray<AActor*> GetLockStepPawnsByIds(const TArray<uint32>& LockStepPawnId);

	TMap<uint32, AActor*> GetAllUnits() const;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	TMap<uint32, AActor*> Id2LockStepPawn;
	TMap<AActor*, uint32> LockStepPawn2Id;
	uint32 PawnIndexCount = 0;

};
