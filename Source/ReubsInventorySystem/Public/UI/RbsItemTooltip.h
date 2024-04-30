// Copyright Vinipi Studios 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RbsItemTooltip.generated.h"

class URbsItemSlot;

UCLASS()
class REUBSINVENTORYSYSTEM_API URbsItemTooltip : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip Item", meta = (ExposeOnSpawn = true))
	TObjectPtr<URbsItemSlot> Item;
};
