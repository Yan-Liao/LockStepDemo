// Fill out your copyright notice in the Description page of Project Settings.


#include "LockStepPawnManager.h"
#include "LockStepDemo/Pawn/LockStepPawn.h"

// Sets default values
ALockStepPawnManager::ALockStepPawnManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ALockStepPawnManager::CreateLockStepPawn(const FVector& CenterLocation, float Space, uint32 SpawnNum)
{
	if (!GetWorld() || SpawnNum == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid world or spawn count!"));
		return;
	}

	// 确保有有效的Pawn类
	if (!BPLockStepPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("No valid ALockStepPawn class assigned!"));
		return;
	}

	// 随机生成SpawnNum个Pawn
	for (uint32 i = 0; i < SpawnNum; ++i)
	{
		FVector SpawnLocation = CenterLocation + FVector(0.f, Space*i, 0.f);
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        
		AActor* NewPawn = GetWorld()->SpawnActor<ALockStepPawn>(
			BPLockStepPawn,
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);
		
		if (NewPawn)
		{
			Id2LockStepPawn.Add({PawnIndexCount, NewPawn});
			LockStepPawn2Id.Add({NewPawn, PawnIndexCount});
			++PawnIndexCount;
			UE_LOG(LogTemp, Log, TEXT("Spawned ALockStepPawn at %s"), *SpawnLocation.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn ALockStepPawn at %s"), *SpawnLocation.ToString());
		}
	}
}

void ALockStepPawnManager::RemoveLockStepPawn(uint32 LockStepPawnId)
{
	if (Id2LockStepPawn.Find(LockStepPawnId))
	{
		AActor* LockStepPawn = *Id2LockStepPawn.Find(LockStepPawnId);
		if (IsValid(LockStepPawn))
		{
			LockStepPawn->Destroy();
		}
		Id2LockStepPawn.Remove(LockStepPawnId);
	}
}

void ALockStepPawnManager::RemoveLockStepPawn(AActor* LockStepPawn)
{
	if (LockStepPawn2Id.Find(LockStepPawn))
	{
		LockStepPawn2Id.Remove(LockStepPawn);
	}
	if (IsValid(LockStepPawn))
	{
		LockStepPawn->Destroy();
	}
	
}

bool ALockStepPawnManager::GetIdByLockStepPawn(const AActor* const LockStepPawn, uint32& LockStepPawnId)
{
	if (LockStepPawn2Id.Find(LockStepPawn))
	{
		LockStepPawnId = LockStepPawn2Id[LockStepPawn];
		return true;
	}
	return false;
}

TArray<uint32> ALockStepPawnManager::GetIdsByLockStepPawns(const TArray<AActor*>& LockStepPawns)
{
	TArray<uint32> Res;
	for (auto Pawn: LockStepPawns)
	{
		uint32 LockStepPawnId = 0;
		if (GetIdByLockStepPawn(Pawn, LockStepPawnId))
		{
			Res.Add(LockStepPawnId);
		}else
		{
			UE_LOG(LogScript, Error, TEXT("GetIdsByLockStepPawns: some pawn lose!"));
		}
	}
	
	return Res;
}

AActor* ALockStepPawnManager::GetLockStepPawnById(uint32 LockStepPawnId)
{
	if (Id2LockStepPawn.Find(LockStepPawnId))
	{
		return Id2LockStepPawn[LockStepPawnId];
	}
	return nullptr;
}

TArray<AActor*> ALockStepPawnManager::GetLockStepPawnsByIds(const TArray<uint32>& LockStepPawnId)
{
	TArray<AActor*> Res;
	for (auto PawnId: LockStepPawnId)
	{
		AActor* Pawn = GetLockStepPawnById(PawnId);
		if (IsValid(Pawn))
		{
			Res.Add(Pawn);
		}else
		{
			UE_LOG(LogScript, Error, TEXT("GetLockStepPawnsByIds: some pawn lose!"));
		}
	}
	return Res;
}

TMap<uint32, AActor*> ALockStepPawnManager::GetAllUnits() const
{
	return Id2LockStepPawn;
}


// Called when the game starts or when spawned
void ALockStepPawnManager::BeginPlay()
{
	Super::BeginPlay();

	PawnIndexCount = 0;
	
	CreateLockStepPawn(PawnLocation, BetweenSpace, PawnNum);
	
}

// Called every frame
void ALockStepPawnManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

