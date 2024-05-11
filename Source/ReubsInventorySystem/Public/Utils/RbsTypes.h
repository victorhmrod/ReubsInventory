// Copyright Vinipi Studios 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RbsTypes.generated.h"

class URbsInventoryItem;

UENUM(BlueprintType)
enum class EItemAddResult : uint8
{
	IAR_NoItemsAdded UMETA(DisplayName = "No items added"),
	IAR_SomeItemsAdded UMETA(DisplayName = "Some items added"),
	IAR_AllItemsAdded UMETA(DisplayName = "All items added")
};

USTRUCT(BlueprintType)
struct FItemAddResult
{
	GENERATED_BODY()
	
	FItemAddResult() {};
	FItemAddResult(int32 InItemQuantity) : AmountToGive(InItemQuantity), AmountGiven(0) {};
	FItemAddResult(int32 InItemQuantity, int32 InQuantityAdded) : AmountToGive(InItemQuantity), AmountGiven(InQuantityAdded) {};

	//The amount of the item that we tried to add
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 AmountToGive = 0;

	//The amount of the item that was actually added in the end. Maybe we tried adding 10 items, but only 8 could be added because of capacity/weight
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 AmountGiven = 0;

	//The result
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	EItemAddResult Result = EItemAddResult::IAR_NoItemsAdded;

	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	FText ErrorText = FText::GetEmpty();

	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	URbsInventoryItem* AddedItem = nullptr;

	static FItemAddResult AddedNone(const int32 InItemQuantity, const FText& ErrorText)
	{
		FItemAddResult AddedNoneResult(InItemQuantity);
		AddedNoneResult.Result = EItemAddResult::IAR_NoItemsAdded;
		AddedNoneResult.ErrorText = ErrorText;

		return AddedNoneResult;
	}

	static FItemAddResult AddedSome(URbsInventoryItem* Item, const int32 InItemQuantity, const int32 ActualAmountGiven, const FText& ErrorText)
	{
		FItemAddResult AddedSomeResult(InItemQuantity, ActualAmountGiven);

		AddedSomeResult.Result = EItemAddResult::IAR_SomeItemsAdded;
		AddedSomeResult.ErrorText = ErrorText;
		AddedSomeResult.AddedItem = Item;

		return AddedSomeResult;
	}

	static FItemAddResult AddedAll(URbsInventoryItem* Item, const int32 InItemQuantity)
	{
		FItemAddResult AddAllResult(InItemQuantity, InItemQuantity);

		AddAllResult.Result = EItemAddResult::IAR_AllItemsAdded;
		AddAllResult.AddedItem = Item;

		return AddAllResult;
	}
};