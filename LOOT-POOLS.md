# Editing Dynamic Pickpocket Loot item pools

The installed file is `Data/SKSE/Plugins/DynamicPickpocketLoot.ini`. Edit the copy deployed by your mod manager. Close Skyrim before editing and restart it afterward. There is no in-game menu or live reload.

## Removing an item

Find its line and delete it, or put a semicolon at the start. The included file names every vanilla item in a comment:

```ini
[Food]
; Skyrim.esm|0x065C97 = Destitute, true, true ; FoodBread01A
```

Commenting out this entry stops new distribution of that bread. It does not remove bread already carried by NPCs or prevent Skyrim and other mods from providing it. Existing recognized possessions can still count toward allowances.

## Adding vanilla or modded items

Put the entry under the matching section:

```ini
[Jewelry]
MyJewelry.esp|0x000812 = Comfortable, false, false

[Potion]
MyPotions.esl|0x000801 = Common, false, false
```

These are syntax examples: replace the filenames and IDs with your actual items. Plugin names can contain spaces. ESM, ESP, ESL, and ESL-flagged ESP files are supported. Use the filename of the plugin that originally defines the form, even if another plugin overrides it.

The ID is hexadecimal and local to that plugin, not the complete ID shown for your current load order. For an ordinary plugin form displayed as `3A000812`, use `000812`; for a light-plugin form displayed as `FE123801`, use `801`. Light-plugin IDs must fit within `000`–`FFF` and be nonzero. The `0x` prefix and leading zeros are optional. Obtain IDs from the item's defining record in xEdit; do not guess an ID from its display name. Compacted/renumbered forms require updated entries.

Each line uses this order:

```ini
Plugin.ext|LocalHexFormID = MinimumWealth, ChildSafe, PrisonerSafe
```

- **MinimumWealth:** `Destitute`, `Poor`, `Common`, `Comfortable`, `Wealthy`, or `Noble`. This is the earliest wealth tier eligible to select the item.
- **ChildSafe:** `true` permits selection by the existing child profile; `false` excludes it. Default to false for new custom items.
- **PrisonerSafe:** The equivalent permission for the existing prisoner profile. Default to false.

Safety permissions do not override the child/prisoner category chances, budgets, or their Destitute wealth tier. For an item to be possible for either profile, its minimum must also be Destitute and its category must be supported by that profile. These flags are not an automatic content-safety assessment.

Names and true/false values are case-insensitive. `1` and `0` also work as booleans. UTF-8 (with or without BOM) and ordinary ASCII files are supported. Lines beginning with `;` or `#` are comments; trailing comments are also allowed.

## Categories

- `[Food]`: Skyrim food/drink items (food-flagged alchemy records), including alcohol where permitted by the flags.
- `[Jewelry]`: Rings and necklaces occupying only the ring/amulet equipment slots. Enchanted jewelry is supported and its engine value counts against the strict valuables budget.
- `[Gem]`: Miscellaneous records explicitly treated as gemstones or small valuables. Their base value consumes the jewelry/gem budget and offsets gold. Only put items you intend to be valuables here.
- `[Potion]`: Non-food, non-poison alchemy records.
- `[Ingredient]`: Alchemy ingredients.
- `[Magic]`: Scrolls and soul gems.
- `[Lockpick]`: Lockpicks or explicitly categorized miscellaneous items that should use the lockpick allowance. These do not receive a valuables gold offset.

Keys, ammunition, weapons, books, body clothing, and ordinary armor are not supported pool types. Gold remains managed by the existing coin system and cannot be inserted into another category. Custom accessory slots outside ring/amulet slots are not supported. Avoid quest-critical or unique story items: category validation cannot determine whether an arbitrary custom item would be safe to distribute.

## How edits affect loot

This readable file replaces the complete built-in generation list. Its shipped contents reproduce all 87 original entries and restrictions. Commenting out one entry does not cause the built-in version to reappear. The last valid definition of the same resolved form wins; duplicate lines do not increase its selection weight. Entries for unavailable plugins, unresolved forms, incompatible item types, and malformed lines are skipped with a warning in `PickpocketingLoot.log`.

If the file is missing, unreadable, or larger than 1 MiB, built-in vanilla pools are used. A readable file with no usable entries intentionally permits only coin generation, including when all entries were invalid, so check warnings after editing. Gold/wealth balancing, category chances, and quantities are not configurable here. A permitted item still has to pass the ordinary category roll, inventory shortfall checks, and any strict value budget. A very expensive enchanted ring may never fit a given profile's budget.

Changes apply to NPCs awaiting first generation or their next victim-timer refresh or genuine inventory reset. Reloading a save, restarting Skyrim, and editing the file do not clear completed generation flags. Existing loot is not removed or rebalanced. Pool edits need no save migration. The victim-timer build automatically migrates older save records.

Keep a backup of your edited INI when updating; mod managers can replace it with the next shipped default. To revert, restore the supplied file or remove it to use built-in defaults.
