// Copyright Vinipi Studios 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RbsPickupInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class URbsPickupInterface : public UInterface
{
	GENERATED_BODY()
};

class REUBSINVENTORYSYSTEM_API IRbsPickupInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent)
	void SetPickupQuantity(const int32 NewQuantity);

	UFUNCTION(BlueprintNativeEvent)
	void OnDropItem();
};
