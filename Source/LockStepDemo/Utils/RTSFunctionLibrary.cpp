// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSFunctionLibrary.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
// #include "Core/Interface/RTSUnitsInterface.h"
#include "LockStepDemo/Interface/RTSHUDInterface.h"
#include "GameFramework/HUD.h"
// #include "Gameplay/SkillSystem/SkillSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
// #include "OJRTS/Gameplay/UnitManagement/RTSUnitManagerSubsystem.h"

void URTSFunctionLibrary::GetActorInPyramid(const UObject* WorldContextObject,
                                            TArray<TSubclassOf<class AActor>> ClassFilters, const FVector& CameraLocation, const TArray<FVector>& EndLocations,
                                            TArray<AActor*>& OutActors)
{
	if (!WorldContextObject)return;

	UWorld* World = WorldContextObject->GetWorld();

	if (World)
	{
		OutActors.Empty();  //清空
		for (int32 i = 0;i<ClassFilters.Num(); ++i)
		{
			for (TActorIterator<AActor> Itr(World, ClassFilters[i]); Itr; ++Itr)  //迭代器筛选世界内的对应Class
			{
				AActor* EachActor = *Itr;
				if (EachActor)
				{
					FVector Point = EachActor->GetActorLocation();
					bool bIsPointInPyramid = IsPointInPyramid(CameraLocation, EndLocations, Point);
					if (bIsPointInPyramid)
					{
						OutActors.Add(EachActor);
					}
				}
			}
		}
	}
}

TArray<AActor*> URTSFunctionLibrary::GetCurrentSelectedUnits(const UObject* WorldContextObject)
{
	if (!WorldContextObject)return{};

	UWorld* World = WorldContextObject->GetWorld();
	if (World)
	{
		APlayerController* FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController)
		{
			AHUD* Ahud = FirstPlayerController->GetHUD();
			IRTSHUDInterface* IrtshudInterface = Cast<IRTSHUDInterface>(Ahud);
			if (IrtshudInterface)
			{
				return IrtshudInterface->HUD_GetSelectedUnits();
			}
		}
	}
	return {};
}

void URTSFunctionLibrary::SetCurrentSelectedUnits(const UObject* WorldContextObject, const TArray<AActor*>& Units)
{
	if (!WorldContextObject)return;

	UWorld* World = WorldContextObject->GetWorld();
	if (World)
	{
		APlayerController* FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController)
		{
			AHUD* Ahud = FirstPlayerController->GetHUD();
			IRTSHUDInterface* IrtshudInterface = Cast<IRTSHUDInterface>(Ahud);
			if (IrtshudInterface)
			{
				IrtshudInterface->HUD_SetSelectedUnits(Units);
			}
		}
	}
	return;
}

TArray<FVector> URTSFunctionLibrary::GetUnitsGoalsInPhalanx(const TArray<AActor*>& InUnits, const FVector& TargetCenter,
	float InUnitSpace)
{
	TArray<FVector> Goals = TArray<FVector>();
	int32 UnitsCount = InUnits.Num();
	if (InUnits.Num()==0)
	{
		return Goals;
	}

	FVector StartCenter = UGameplayStatics::GetActorArrayAverageLocation(InUnits);
	FVector Direction = TargetCenter - StartCenter;
	Direction.Z = 0.0f;
	Direction.Normalize();

	FVector RightDirection = Direction.RotateAngleAxis(90.f, {0.0f, 0.0f, 1.0f});

	int32 ColCount = FMath::CeilToInt(UKismetMathLibrary::Sqrt(UnitsCount));
	int32 RowCount = FMath::CeilToInt(1.0*UnitsCount/ColCount);

	//并非实际间隔，这么写能统一公式
	float ColumnSpace = ColCount/2.f;
	float RowSpace = RowCount/2.f;

	for (float i = -RowSpace; i < RowSpace; ++i)
	{
		for (float j = -ColumnSpace; j < ColumnSpace; ++j)
		{
			FVector TargetLocation = TargetCenter - (i+0.5f)*Direction*InUnitSpace + (j+0.5f)*RightDirection*InUnitSpace;
			Goals.Add(TargetLocation);
		}
	}

	//对Goals排下序，可以避免一部分碰撞
	//按Goals到起点的距离排序(从大到小)
	Goals.Sort([StartCenter](const FVector& A, const FVector& B)
	{
		return FVector::DistXY(StartCenter, A)>FVector::DistXY(StartCenter, B);
	});

	//便利最远点，最近的actor是谁，先满足最远点的
	TMap<int32, int32> ActorIdx2GoalIdx;
	TSet<int32> FinishedActorIdx;
	for (int32 i = 0; i < Goals.Num(); ++i)
	{
		FVector Goal = Goals[i];
		int32 MinDis = MAX_int32;
		int32 MinIdx = -1;
		for (int32 j = 0; j < InUnits.Num(); ++j)
		{
			if (FinishedActorIdx.Contains(j))continue;
			AActor* unit = InUnits[j];
			if (unit)
			{
				FVector UnitLocation = unit->GetActorLocation();
				double DistXY = FVector::DistXY(UnitLocation, Goal);
				if (DistXY<MinDis)
				{
					MinDis = DistXY;
					MinIdx = j;
				}
			}
		}
		if (MinIdx!=-1)
		{
			ActorIdx2GoalIdx.Add(MinIdx, i);
			FinishedActorIdx.Add(MinIdx);
		}
	}

	TArray<FVector> SortedGoals;
	for (int i=0;i<ActorIdx2GoalIdx.Num();++i)
	{
		if (ActorIdx2GoalIdx.Contains(i))
		{
			SortedGoals.Add(Goals[ActorIdx2GoalIdx[i]]);
		}
	}
	
	return SortedGoals;
}

bool URTSFunctionLibrary::IsPointInPyramid(const FVector& CameraLocation, const TArray<FVector>& EndLocations,
                                           const FVector& PointToCheck)
{
	if (EndLocations.Num() != 4)
	{
		return false;
	}

	//不能存在相同的点，这块简写后续优化
	if (EndLocations[0]==EndLocations[1])
	{
		return false;
	}

	TArray<FPlane> Planes;
	Planes.Add(FPlane(CameraLocation, EndLocations[0], EndLocations[1]));  //这三个点是有顺序的，打印后法线这个顺序下法线向内
	Planes.Add(FPlane(CameraLocation, EndLocations[1], EndLocations[2]));
	Planes.Add(FPlane(CameraLocation, EndLocations[2], EndLocations[3]));
	Planes.Add(FPlane(CameraLocation, EndLocations[3], EndLocations[0]));
	
	for (const auto& Plane: Planes)
	{
		if (Plane.PlaneDot(PointToCheck)<0)  //Plane法向量朝内（print 法线normal后看到的），向内为正，PlaneDot是计算点和面距离，小于零说明在面外侧。
		{
			return false;
		}
	}
	
	return true;
}
