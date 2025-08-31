// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSHUD.h"
#include "LockStepDemo/Utils//RTSFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LockStepDemo/Pawn/LockStepPawn.h"

void ARTSHUD::HUD_MutipleSelection_Implementation(bool bLeftMousePressed, bool bShiftPressed)
{
	IRTSHUDInterface::HUD_MutipleSelection_Implementation(bLeftMousePressed, bShiftPressed);

	//如果按住mouse时加按shift，会先触发addselect，此时只改变IsShiftPressed
	if (bLeftMousePressed&&bShiftPressed&&IsMousePressed)
	{
		IsShiftPressed = bShiftPressed;
		return;
	}
	//如果按住mouse时加按shift，先触发addselect后会触发select compelect,此时不响应
	if (IsShiftPressed&&IsMousePressed&&!bLeftMousePressed&&!bShiftPressed)
	{
		return;
	}
	
	IsMousePressed = bLeftMousePressed;
	IsShiftPressed = bShiftPressed;
		
	APlayerController* OwningPlayerController = GetOwningPlayerController();
	if (OwningPlayerController)
	{
		//首次按下，记录起始点
		if (IsMousePressed)
		{
			OwningPlayerController->GetMousePosition(StartMousePoint.X, StartMousePoint.Y);
		}
		//释放按键，计算选中目标，并调用选中接口
		else
		{
			QueryUnitsInRange();
		}
		
	}
	
}

TArray<AActor*> ARTSHUD::HUD_GetSelectedUnits()
{
	return SelectedActors.Array();
}

void ARTSHUD::HUD_SetSelectedUnits(const TArray<AActor*>& Units)
{
	UpdateSelectedUnits(Units);
}

void ARTSHUD::DrawHUD()
{
	Super::DrawHUD();

	if (IsMousePressed)
	{
		APlayerController* OwningPlayerController = GetOwningPlayerController();
		if (OwningPlayerController)
		{
			OwningPlayerController->GetMousePosition(CurrentMousePoint.X, CurrentMousePoint.Y);
			DrawRect(SelectBoxColor, StartMousePoint.X, StartMousePoint.Y, CurrentMousePoint.X-StartMousePoint.X, CurrentMousePoint.Y-StartMousePoint.Y);
		}
	}
}

void ARTSHUD::SetMappingMode()
{
	if (IsMousePressed)return;
	IsMappingMode = !IsMappingMode;
}

void ARTSHUD::QueryUnitsInRange()
{
	APlayerController* OwningPlayerController = GetOwningPlayerController();
	if (OwningPlayerController)
	{
		TArray<AActor*> NewSelectedActors;
		if (StartMousePoint==CurrentMousePoint)
		{
			FVector WorldPoint;
			FVector WorldDirection;
			OwningPlayerController->DeprojectScreenPositionToWorld(StartMousePoint.X, StartMousePoint.Y, WorldPoint, WorldDirection);
			TArray<TEnumAsByte<EObjectTypeQuery> > ObjectTypes;
			ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
			FHitResult OnHit;
			bool bHited = UKismetSystemLibrary::LineTraceSingleForObjects(this, WorldPoint, WorldPoint+WorldDirection*SelectLength, ObjectTypes, false, {}, EDrawDebugTrace::None, OnHit, true);
			if (bHited)
			{
				AActor* Actor = OnHit.GetActor();
				if (Actor)
				{
					NewSelectedActors.Add(Actor);
				}
			}
		}else
		{
			FVector2D ViewPointCenter = (StartMousePoint+CurrentMousePoint)/2;
			FVector WorldPointCenter;
			FVector tmp;
			OwningPlayerController->DeprojectScreenPositionToWorld(ViewPointCenter.X, ViewPointCenter.Y, WorldPointCenter, tmp);

			FVector2D ViewPoint0 = {FMath::Min(StartMousePoint.X, CurrentMousePoint.X), FMath::Min(StartMousePoint.Y, CurrentMousePoint.Y)};
			FVector WorldPoint0;
			FVector WorldDirection0;
			OwningPlayerController->DeprojectScreenPositionToWorld(ViewPoint0.X, ViewPoint0.Y, WorldPoint0, WorldDirection0);
			WorldPoint0+=WorldDirection0*SelectLength;
		
			FVector2D ViewPoint1 = {FMath::Min(StartMousePoint.X, CurrentMousePoint.X), FMath::Max(StartMousePoint.Y, CurrentMousePoint.Y)};
			FVector WorldPoint1;
			FVector WorldDirection1;
			OwningPlayerController->DeprojectScreenPositionToWorld(ViewPoint1.X, ViewPoint1.Y, WorldPoint1, WorldDirection1);
			WorldPoint1+=WorldDirection1*SelectLength;
		
			FVector2D ViewPoint2 = {FMath::Max(StartMousePoint.X, CurrentMousePoint.X), FMath::Max(StartMousePoint.Y, CurrentMousePoint.Y)};
			FVector WorldPoint2;
			FVector WorldDirection2;
			OwningPlayerController->DeprojectScreenPositionToWorld(ViewPoint2.X, ViewPoint2.Y, WorldPoint2, WorldDirection2);
			WorldPoint2+=WorldDirection2*SelectLength;
		
			FVector2D ViewPoint3 = {FMath::Max(StartMousePoint.X, CurrentMousePoint.X), FMath::Min(StartMousePoint.Y, CurrentMousePoint.Y)};
			FVector WorldPoint3;
			FVector WorldDirection3;
			OwningPlayerController->DeprojectScreenPositionToWorld(ViewPoint3.X, ViewPoint3.Y, WorldPoint3, WorldDirection3);
			WorldPoint3+=WorldDirection3*SelectLength;
		
			//新选出的
			TArray<TSubclassOf<class AActor>> ClassFilters;
			ClassFilters.Add(ALockStepPawn::StaticClass());
			URTSFunctionLibrary::GetActorInPyramid(this, ClassFilters, WorldPointCenter, {WorldPoint0, WorldPoint1, WorldPoint2, WorldPoint3}, NewSelectedActors);
		}
		
		
		//更新选中项
		UpdateSelectedUnits(NewSelectedActors, IsShiftPressed);

	}
}

void ARTSHUD::UpdateSelectedUnits(const TArray<AActor*>& NewSelectedActors, bool bAdded)
{
	//之前的选择整体调用不选
	for (const auto& actor: SelectedActors)
	{
		if (IsValid(actor))
		{
			IRTSUnitsInterface::Execute_Units_Selected(actor, false);
		}
		
	}
	
	if (!bAdded)
	{
		SelectedActors.Empty();
	}
	//Add Select		
	SelectedActors.Append(NewSelectedActors);
				
	//选择新框中的actor
	for (const auto& actor: SelectedActors)
	{
		if (IsValid(actor))
		{
			IRTSUnitsInterface::Execute_Units_Selected(actor, true);
		}
		
	}

	// //选中单位
	// if (GetWorld())
	// {
	// 	UBattleEventSubsystem* BattleEventSubsystem = GetWorld()->GetSubsystem<UBattleEventSubsystem>();
	// 	if (BattleEventSubsystem)
	// 	{
	// 		BattleEventSubsystem->OnTriggerSelect.Broadcast();
	// 	}
	// }
}
