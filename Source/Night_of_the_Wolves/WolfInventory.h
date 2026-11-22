#pragma once

#include "CoreMinimal.h"

// Each unique object travels with its owning checkpoint. NAME_None means empty hands.
struct FWolfInventory
{
	TArray<FName> Items;
	FName Equipped;
	bool bBurning = false;

	bool Has(FName Item) const { return !Item.IsNone() && Items.Contains(Item); }

	void Add(FName Item)
	{
		if (!Item.IsNone()) Items.AddUnique(Item);
	}

	bool Equip(FName Item)
	{
		if (!Item.IsNone() && !Has(Item)) return false;
		if (Item != Equipped && Equipped == TEXT("Firebrand")) Extinguish();
		Equipped = Item;
		return true;
	}

	bool Consume(FName Item)
	{
		if (!Has(Item)) return false;
		if (Equipped == Item) Equip(NAME_None);
		Items.RemoveSingle(Item);
		return true;
	}

	void Ignite()
	{
		Add(TEXT("Firebrand"));
		Equip(TEXT("Firebrand"));
		bBurning = true;
	}

	void Extinguish() { bBurning = false; }

	FString DisplayName(FName Item) const
	{
		if (Item.IsNone()) return TEXT("Empty hands");
		if (Item == TEXT("Gold")) return TEXT("Sack of gold");
		if (Item == TEXT("Firebrand")) return bBurning ? TEXT("Burning firebrand") : TEXT("Extinguished firebrand");
		return Item.ToString();
	}
};
