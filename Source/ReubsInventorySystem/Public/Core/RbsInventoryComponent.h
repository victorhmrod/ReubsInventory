// Copyright Vinipi Studios 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RbsInventoryItem.h"
#include "RbsInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class REUBSINVENTORYSYSTEM_API URbsInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URbsInventoryComponent();

	friend URbsInventoryItem;

////////////////////////////////////////////// Variables ///////////////////////////////////////////////////////////////
	
/*
 * info
 */

protected:
	UPROPERTY(ReplicatedUsing = OnReplicated_Items, VisibleAnywhere, Category = "Inventory")
	TArray<TObjectPtr<URbsInventoryItem>> Items;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	float WeightCapacity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin=0, ClampMax=500))
	int32 Capacity;

/*
 * Behaviour
 */

	UFUNCTION(Client, Reliable)
	void ClientRefreshInventory();
	
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

/*
 * Replication
 */

private:
	UPROPERTY()
	int32 ReplicatedItemsKey = 0;	

////////////////////////////////////////////// Functions ///////////////////////////////////////////////////////////////	

/*
 * Replication
 */

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

private:
	UFUNCTION()
	void OnReplicated_Items();
	
/*
 * Behaviour
 */

private:
	
	URbsInventoryItem* AddItem(URbsInventoryItem* Item);

public:	
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult TryAddItem(class URbsInventoryItem* Item);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult TryAddItemFromClass(TSubclassOf<URbsInventoryItem> ItemClass, const int32 Quantity = 1);

	FItemAddResult TryAddItem_Internal(URbsInventoryItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(URbsInventoryItem* Item);
	
	int32 ConsumeItem(URbsInventoryItem* Item);
	int32 ConsumeItem(URbsInventoryItem* Item, const int32 Quantity);

protected:

	UFUNCTION()
	void OnItemModified_Internal();
	
/*
 * Helpers
 */

public:	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE float GetWeightCapacity() const { return WeightCapacity; };

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE int32 GetCapacity() const { return Capacity; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE TArray<URbsInventoryItem*> GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetCurrentWeight() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetWeightCapacity(const float NewWeightCapacity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetCapacity(const int32 NewCapacity);

	/**Return true if we have a given amount of an item*/
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(TSubclassOf <URbsInventoryItem> ItemClass, const int32 Quantity = 1) const;

	/**Return the first item with the same class as a given Item*/
	UFUNCTION(BlueprintPure, Category = "Inventory")
	URbsInventoryItem* FindItem(URbsInventoryItem* Item) const;

	/**Return all items with the same class as a given Item*/
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<URbsInventoryItem*> FindItems(URbsInventoryItem* Item) const;

	/**Return the first item with the same class as ItemClass.*/
	UFUNCTION(BlueprintPure, Category = "Inventory")
	URbsInventoryItem* FindItemByClass(TSubclassOf<URbsInventoryItem> ItemClass) const;

	/**Get all inventory items that are a child of ItemClass. Useful for grabbing all weapons, all food, etc*/
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<URbsInventoryItem*> FindItemsByClass(TSubclassOf<URbsInventoryItem> ItemClass) const;
	
};
