// Copyright Vinipi Studios 2024. All Rights Reserved.


#include "Core/RbsInventoryComponent.h"

#include "Core/RbsInventoryItem.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "Inventory"

URbsInventoryComponent::URbsInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

/*
 * Replication
 */

void URbsInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URbsInventoryComponent, Items);
}

bool URbsInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	//Check if the array of items needs to replicate
	if (Channel->KeyNeedsToReplicate(0, ReplicatedItemsKey))
	{
		for (auto& Item : Items)
		{
			if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
			{
				bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
			}
		}
	}

	return bWroteSomething;
}

/*
 * Behaviour
 */

URbsInventoryItem* URbsInventoryComponent::AddItem(URbsInventoryItem* Item)
{
	if (GetOwner()->GetLocalRole() < ROLE_Authority)
		return nullptr;

	URbsInventoryItem* NewItem = NewObject<URbsInventoryItem>(GetOwner(), Item->GetClass());
	NewItem->SetQuantity(Item->GetQuantity());
	NewItem->OwningInventory = this;
	NewItem->AddedToInventory(this);
	Items.Add(NewItem);
	OnReplicated_Items();
	NewItem->MarkDirtyForReplication();

	NewItem->OnItemModified.AddDynamic(this, &ThisClass::OnItemModified_Internal);

	return NewItem;
}

FItemAddResult URbsInventoryComponent::TryAddItem(URbsInventoryItem* Item)
{
	return TryAddItem_Internal(Item);
}

FItemAddResult URbsInventoryComponent::TryAddItemFromClass(TSubclassOf<URbsInventoryItem> ItemClass, const int32 Quantity)
{
	URbsInventoryItem* Item = NewObject<URbsInventoryItem>(GetOwner(), ItemClass);
	Item->SetQuantity(Quantity);
	
	return TryAddItem_Internal(Item);
}

FItemAddResult URbsInventoryComponent::TryAddItem_Internal(URbsInventoryItem* Item)
{
	if (GetOwner()->GetLocalRole() < ROLE_Authority)
		return FItemAddResult::AddedNone(Item->GetQuantity(), LOCTEXT("InventoryCallingFunctionsFromClient", "ERROR | You're trying to add items from a client"));;

	const int32 AddAmount = Item->GetQuantity();
	if (Items.Num() + 1 > GetCapacity())
		return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryCapacityFullText", "Inventory Is Full"));

	if (GetCurrentWeight() + Item->Weight > GetWeightCapacity())
		return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryTooMuchWeightText", "Too Much Weight"));

	if (Item->bStackable)
	{
		ensure(Item->GetQuantity() <= Item->MaxStackSize);

		if (URbsInventoryItem* ExistingItem = FindItem(Item))
		{
			if (ExistingItem->GetQuantity() < ExistingItem->MaxStackSize)
			{
				const int32 CapacityMaxAddAmount = ExistingItem->MaxStackSize - ExistingItem->GetQuantity();
				int ActualAddAmount = FMath::Min(AddAmount, CapacityMaxAddAmount);

				FText ErrorText = LOCTEXT("InventoryAddedSomeText", "Couldn't add all items");
				
				const int32 WeightMaxAddAmount = FMath::FloorToInt((WeightCapacity - GetCurrentWeight()) / Item->Weight);
				ActualAddAmount = FMath::Min(ActualAddAmount, WeightMaxAddAmount);

				if (ActualAddAmount <= 0)
				{
					return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryErrorText", "Couldn't add any item"));
				}

				ExistingItem->SetQuantity(ExistingItem->GetQuantity() + ActualAddAmount);
				OnInventoryUpdated.Broadcast();

				ensure(Item->GetQuantity() <= Item->MaxStackSize);

				if (ActualAddAmount < AddAmount)
				{
					return FItemAddResult::AddedSome(Item, AddAmount, ActualAddAmount, ErrorText);
				}
				else
				{
					return FItemAddResult::AddedAll(Item, AddAmount);
				}
			}
			else
			{
				return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryFullStackText", "You Already Have a Full Stack of this Item"));
			}
		}
		else
		{
			AddItem(Item);
			return FItemAddResult::AddedAll(Item, Item->GetQuantity());	
		}
	}
	else
	{
		ensure(Item->GetQuantity() == 1);

		AddItem(Item);
		return FItemAddResult::AddedAll(Item, Item->GetQuantity());
	}
}

bool URbsInventoryComponent::RemoveItem(URbsInventoryItem* Item)
{
	if (GetOwner()->GetLocalRole() < ROLE_Authority)
		return false;

	if (!IsValid(Item))
		return false;
			
	Items.RemoveSingle(Item);

	Item->OnItemModified.RemoveDynamic(this, &ThisClass::OnItemModified_Internal);
	//OnItemRemoved.Broadcast(Item);

	ReplicatedItemsKey++;

	return true;
}

int32 URbsInventoryComponent::ConsumeItem(URbsInventoryItem* Item)
{
	if (IsValid(Item))
	{
		return ConsumeItem(Item, Item->Quantity);
	}
	return 0;
}

int32 URbsInventoryComponent::ConsumeItem(URbsInventoryItem* Item, const int32 Quantity)
{
	if (GetOwner()->GetLocalRole() < ROLE_Authority)
		return 0;

	if (!IsValid(Item))
		return 0;

	const int32 RemoveQuantity = FMath::Min(Quantity, Item->GetQuantity());

	ensure(!(Item->GetQuantity() - RemoveQuantity < 0));

	Item->SetQuantity(Item->GetQuantity() - RemoveQuantity);

	if (Item->GetQuantity() <= 0)
	{
		RemoveItem(Item);
	}
	else
	{
		ClientRefreshInventory();
	}

	return RemoveQuantity;
}

void URbsInventoryComponent::OnItemModified_Internal()
{
	OnInventoryUpdated.Broadcast();
}

/*
 * Helpers
 */

bool URbsInventoryComponent::HasItem(TSubclassOf<URbsInventoryItem> ItemClass, const int32 Quantity) const
{
	URbsInventoryItem* ItemToFind = FindItemByClass(ItemClass);
	if (IsValid(ItemToFind))
	{
		return ItemToFind->GetQuantity() >= Quantity;
	}
	return false;
}

URbsInventoryItem* URbsInventoryComponent::FindItem(URbsInventoryItem* Item) const
{
	return FindItemByClass(Item->GetClass());
}

TArray<URbsInventoryItem*> URbsInventoryComponent::FindItems(URbsInventoryItem* Item) const
{
	return FindItemsByClass(Item->GetClass());
}

URbsInventoryItem* URbsInventoryComponent::FindItemByClass(TSubclassOf<URbsInventoryItem> ItemClass) const
{
	for (auto& Item : Items)
	{
		if (Item->GetClass() == ItemClass)
		{
			return Item;
		}
	}

	return nullptr;
}

TArray<URbsInventoryItem*> URbsInventoryComponent::FindItemsByClass(TSubclassOf<URbsInventoryItem> ItemClass) const
{
	TArray<URbsInventoryItem*> ItemsOfClas{};
	for (auto& Item : Items)
	{
		if (Item->GetClass() == ItemClass)
		{
			ItemsOfClas.Add(Item);
		}
	}

	return ItemsOfClas;
}

float URbsInventoryComponent::GetCurrentWeight() const
{
	float Weight = 0.f;

	for (auto& Item : Items)
	{
		Weight += Item->GetStackWeight();
	}

	return Weight;
}

void URbsInventoryComponent::SetWeightCapacity(const float NewWeightCapacity)
{
	WeightCapacity = NewWeightCapacity;
	OnInventoryUpdated.Broadcast();
}

void URbsInventoryComponent::SetCapacity(const int32 NewCapacity)
{
	Capacity = NewCapacity;
	OnInventoryUpdated.Broadcast();
}

void URbsInventoryComponent::OnReplicated_Items()
{
	OnInventoryUpdated.Broadcast();
}

void URbsInventoryComponent::ClientRefreshInventory_Implementation()
{
	OnInventoryUpdated.Broadcast();
}

#undef LOCTEXT_NAMESPACE
