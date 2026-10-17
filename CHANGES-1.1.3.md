# Dynamic Pickpocket Loot 1.1.3 — valuation and crossbow cleanup

- Pocket accounting classifies items before valuation. Ordinary weapons, armor,
  clothing, food, and junk no longer enter engine value calculations during the
  inventory scan. Gems still count; jewelry retains instance enchantment value.
- Native leveled-list discovery only compares prices for missing, guaranteed
  random alternatives. Use-all bundles, observed branches, optional empty lists,
  and single choices do not request prices.
- Restoration comparisons use unenchanted record prices rather than Skyrim's
  inventory-value function, including clothing, armor timers, and weapon choice.
- Material and clothing keywords resolve by vanilla plugin/FormID. This removes
  the all-plugin editor-ID scan and recognizes Dawnguard's actual Dragonbone key.
- Crossbow restoration can consider plain steel/iron crossbows in the actor's
  own native inventory/outfit lists, respecting list level/chance restrictions.
  Replacements must not exceed the source record's price or damage. No bolts
  are added. Bow and crossbow sources retain separate saved slots.
- Added a missing-race guard. Save schema remains 4; existing generation flags,
  inventory, restoration deadlines and pickpocket cooldowns are preserved.

Validation: release build, core regression tests, Windows DLL loader/exports,
metadata, Address Library file audit and archive checks. No Skyrim gameplay test.
The reporter's crash is not reproduced and this is not a confirmed crash fix.

Crossbow Integration (Including Creation Club), Nexus mod 28766, was reviewed
through its public listing. Its exact Complete No WACCF ESP/archive is absent
from the installed Data directory and Downloads, so no record-level or live
integration claim is made. The reported overridden Dawnguard crossbow remains
resolved by Dawnguard.esm:000801, independent of its displayed name. No changes
are made to Crossbow Integration or its records.
