// Copyright Vinipi Studios 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RbsInventoryItem.generated.h"

class URbsItemTooltip;
class URbsInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemModified);

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class REUBSINVENTORYSYSTEM_API URbsInventoryItem : public UObject
{
	GENERATED_BODY()
	
public:

///////////////////////////////////////////////////// Variables ////////////////////////////////////////////////////////

	URbsInventoryItem();

/*
 * UObject Replication
 */

	UPROPERTY()
	int32 RepKey = 0;

/*
 * Properties
 */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSubclassOf<AActor> PickupClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UTexture2D* Thumbnail{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 0.0))
	float Weight = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	bool bStackable = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 2, EditCondition = bStackable))
	int32 MaxStackSize = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	TSubclassOf<URbsItemTooltip> ItemTooltip;

	UPROPERTY(ReplicatedUsing = OnRep_Quantity, EditAnywhere, Category = "Item", meta = (UIMin = 1, EditCondition = bStackable))
	int32 Quantity = 1;
	
	UPROPERTY()
	TObjectPtr<URbsInventoryComponent> OwningInventory;

///////////////////////////////////////////////////// Functions ////////////////////////////////////////////////////////


#if WITH_EDITOR	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif
	
/*
 * UObject Replication
 */

protected:
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
		
	UFUNCTION()
	void OnRep_Quantity();
	
public:
	void MarkDirtyForReplication();

/*
 * Behaviour
 */


public:
	
	UPROPERTY(BlueprintAssignable)
	FOnItemModified OnItemModified;

	UFUNCTION(BlueprintNativeEvent)
	void Use(URbsInventoryComponent* Inventory);
	UFUNCTION(BlueprintNativeEvent)
	void AddedToInventory(URbsInventoryComponent* Inventory);

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetQuantity(const int32 NewQuantity);
	
/*	
 * Helpers
 */
	
	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE bool ShouldShowInInventory() const { return true; } 

	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE float GetStackWeight() const { return Quantity * Weight; }

	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE int GetQuantity() const { return Quantity; }

	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE bool IsStackFull() const { return Quantity >= MaxStackSize; }
};
