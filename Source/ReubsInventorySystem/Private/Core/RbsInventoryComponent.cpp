// Copyright Vinipi Studios 2024. All Rights Reserved.


#include "Core/RbsInventoryComponent.h"

#include "Components/CapsuleComponent.h"
#include "Core/RbsInventoryItem.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/Character.h"
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
		//ensure(Item->GetQuantity() <= Item->MaxStackSize);

		const int32 WeightMaxAddAmount = FMath::FloorToInt((WeightCapacity - GetCurrentWeight()) / Item->Weight);
		int32 ActualAddAmount = FMath::Min(AddAmount, WeightMaxAddAmount);
		if (ActualAddAmount <= 0)
			return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryErrorText", "Couldn't add any item"));
		
		auto ExistingItems = FindItems(Item);
		for (auto Temp : ExistingItems)
		{
			if (Temp->GetQuantity() >= Temp->MaxStackSize)
				continue;
			
			const int32 StackAddAmount = FMath::Min(ActualAddAmount, Temp->MaxStackSize - Temp->GetQuantity());
			if (StackAddAmount <= 0)
				continue;
			
			Temp->SetQuantity(Temp->GetQuantity() + StackAddAmount);
			Item->SetQuantity(Item->GetQuantity() - StackAddAmount);
			ActualAddAmount -= StackAddAmount;
		}

		if (ActualAddAmount > 0)
		{
			const int32 StacksToAdd = FMath::CeilToInt(float(ActualAddAmount) / float(Item->MaxStackSize));
			for (size_t i = 0; i < StacksToAdd; i++)
			{
				if (ActualAddAmount <= 0)
					break;
				
				int32 StackAddAmount = FMath::Min(ActualAddAmount, Item->MaxStackSize);
				if (StackAddAmount <= 0)
					continue;
				
				ActualAddAmount -= StackAddAmount;
				Item->SetQuantity(StackAddAmount);
				AddItem(Item);
			}
		}

		if (ActualAddAmount > 0)
			return FItemAddResult::AddedSome(Item, AddAmount, ActualAddAmount, LOCTEXT("InventoryAddedSomeText", "Couldn't add all items"));
		
		return FItemAddResult::AddedAll(Item, AddAmount);

		/*
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
		*/
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
			
	Items.Remove(Item);
	OnReplicated_Items();

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

	ClientRefreshInventory();

	return RemoveQuantity;
}

void URbsInventoryComponent::UseItem(URbsInventoryItem* Item)
{
	if (GetOwnerRole() < ROLE_Authority)
		ServerUseItem(Item);

	if (GetOwner()->GetLocalRole() >= ROLE_Authority)
	{
		if (!IsValid(FindItem(Item)))
			return;
	}

	if (IsValid(Item))
	{
		Item->Use(this);
	}
}

void URbsInventoryComponent::ServerUseItem_Implementation(URbsInventoryItem* Item)
{
	UseItem(Item);
}

void URbsInventoryComponent::DropItem(URbsInventoryItem* Item, const int32 Quantity)
{
	if (!IsValid(FindItem(Item)))
		return;

	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerDropItem(Item, Quantity);
		return;
	}
	
	const int32 DroppedQuantity = ConsumeItem(Item, Quantity);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.bNoFail = true;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FVector SpawnLocation = GetOwner()->GetActorLocation();
	SpawnLocation.Z -= Cast<ACharacter>(GetOwner())->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	
	FTransform SpawnTransform(GetOwner()->GetActorRotation(), SpawnLocation);

	ensure(Item->PickupClass);

	AActor* Pickup = GetWorld()->SpawnActor<AActor>(Item->PickupClass, SpawnTransform, SpawnParams);
	// TODO - Call interrface function to set the quantity
}

void URbsInventoryComponent::ServerDropItem_Implementation(URbsInventoryItem* Item, const int32 Quantity)
{
	DropItem(Item, Quantity);
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
