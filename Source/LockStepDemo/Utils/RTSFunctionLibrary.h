// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTSFunctionLibrary.generated.h"

class ASkillActorBase;
/**
 * 
 */
UCLASS()
class LOCKSTEPDEMO_API URTSFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*
	 * 获取范围内的单位
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static void GetActorInPyramid(const UObject* WorldContextObject, TArray<TSubclassOf<class AActor>> ClassFilters, const FVector& CameraLocation, const TArray<FVector>& EndLocations, TArray<AActor*>& OutActors);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static TArray<AActor*> GetCurrentSelectedUnits(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static void SetCurrentSelectedUnits(const UObject* WorldContextObject, const TArray<AActor*>& Units);
	
	/*
	 * 求队伍每个成员方阵坐标
	 * InUnitSpace：成员间距
	 */
	UFUNCTION(BlueprintCallable)
	static TArray<FVector> GetUnitsGoalsInPhalanx(const TArray<AActor*>& InUnits, const FVector& TargetCenter, float InUnitSpace);
	
private:
	static bool IsPointInPyramid(const FVector& CameraLocation, const TArray<FVector>& EndLocations, const FVector& PointToCheck);
};
