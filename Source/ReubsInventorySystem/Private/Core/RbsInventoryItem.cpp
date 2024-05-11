// Copyright Vinipi Studios 2024. All Rights Reserved.

#include "Core/RbsInventoryItem.h"

#include "Core/RbsInventoryComponent.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "Item"

URbsInventoryItem::URbsInventoryItem()
{
	DisplayName = LOCTEXT("Placeholder Name", "Item");
	Description = LOCTEXT("Placeholder Description", "Item");
}

#if WITH_EDITOR

void URbsInventoryItem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	FName ChangedPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// UPROPERTY clamping doesn't support using a variable to clamp so we do in here instead
	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(URbsInventoryItem, Quantity))
	{
		Quantity = FMath::Clamp(Quantity, 1, bStackable ? MaxStackSize : 1);
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(URbsInventoryItem, bStackable))
	{
		if (!bStackable)
		{
			Quantity = 1;
		}
	}
}

#endif

void URbsInventoryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URbsInventoryItem, Quantity);
}

void URbsInventoryItem::MarkDirtyForReplication()
{
	++RepKey;

	if (IsValid(OwningInventory))
	{
		OwningInventory->ReplicatedItemsKey++;
	}
}

void URbsInventoryItem::OnRep_Quantity()
{
	OnItemModified.Broadcast();
}

void URbsInventoryItem::Use_Implementation(URbsInventoryComponent* Inventory)
{
}

void URbsInventoryItem::AddedToInventory_Implementation(URbsInventoryComponent* Inventory)
{
}

void URbsInventoryItem::SetQuantity(const int32 NewQuantity)
{
	if (NewQuantity != Quantity)
	{
		Quantity = NewQuantity;
			//FMath::Clamp(NewQuantity, 0, bStackable ? MaxStackSize : 1);
		OnRep_Quantity();
		MarkDirtyForReplication();
	}
}

#undef LOCTEXT_NAMESPACE
