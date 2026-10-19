# Dynamic Pickpocket Loot 1.1.4



See [1.1.4 changes and validation limits](research/v1.1.4-cell-reequip.md).

Inventory-aware pocket loot for Skyrim Special Edition / Anniversary Edition, with a built-in three-day pickpocket-victim cooldown and clothing/armor/weapon restoration. Includes one SKSE DLL and an editable item-pool INI. No ESP, SPID dependency, in-game menu, personal notes, bounties, keys, or ammunition handling.



## Installation and requirements



1. Install SKSE matching your Skyrim executable and the matching Address Library for SKSE Plugins.

2. Install the Microsoft Visual C++ 2015–2022 x64 runtime if needed.

3. Install Dynamic-Pickpocket-Loot-v1.1.3.zip through your mod manager, replacing the previous release. Keep your customized INI backed up.

4. The files belong at Data/SKSE/Plugins/PickpocketingLoot.dll and Data/SKSE/Plugins/DynamicPickpocketLoot.ini. The legacy DLL filename is intentional for update compatibility.

5. Launch through SKSE. Diagnostics are in Documents/My Games/Skyrim Special Edition/SKSE/PickpocketingLoot.log, or the applicable SKSE log directory for your distribution.



SkyUI, MCM, and Pickpocket Reset are not required.



## Supported build targets



- SE: 1.5.97.

- AE: 1.6.317, 318, 323, 342, 353, 629, 640, 659, 1130, 1170, 1179; 1.7.99 and 1.7.104.

- Other executable versions, Legendary Edition, and VR are rejected.



These are compatibility targets, not a claim of in-game validation on every version. One CommonLibSSE-NG DLL supports SE/AE. The installed 1.7.104 executable and Address Library were audited read-only for reset mappings and the added event-source mapping. See VALIDATION.md for actual evidence and remaining tests.



## Loot and existing possessions



Six wealth profiles run from Destitute through Poor, Common, Comfortable, Wealthy, and Noble. Vanilla factions, classes, guard status, and vendor services choose occupations, with Poor–Comfortable fallback wealth. The default item file includes all 87 original pool entries: food/drink, jewelry, gems, restoration potions, ingredients, scrolls, soul gems, and lockpicks, plus a separate coin system.



The mod scans current possessions and adds only category/coin shortfalls. Existing jewelry and gems consume a shared strict valuables budget and offset the normal gold target by 25% of accounted value down to the profile floor. Qualifying worn jewelry counts. Jewelry uses engine inventory valuation including built-in/custom enchantments, with plain and enchanted instances valued separately. Gems use base value. Clothes, armor, weapons, junk, food, potions, ingredients, scrolls, and soul gems never reduce gold; recognized supplies still satisfy their own category limits. Existing possessions above allowances are left alone.



An otherwise empty result receives a small coin target: 2–4, or 2–3 for prisoners, counting existing coins. Clothing/weapons alone do not satisfy the pocket-possession requirement. Unaffordable valuables are skipped. No existing possessions are removed, and item generation uses only the configured pools rather than restoring an NPC's original inventory.



## Three-day victim timer



The first successful theft from an eligible living NPC starts a per-victim deadline three in-game days later. Additional thefts during that countdown do not extend it. Merely opening the menu, failed thefts without a transfer, gifts/reverse pickpocketing, ordinary trading, and corpse looting do not qualify as NPC-to-player thefts. Successful-theft notifications are detected through the pickpocket menu's positive inventory transfers and the engine's ItemsPickpocketed event; their ordering still needs in-game acceptance testing.



At the deadline, the victim becomes eligible for one fresh normal loot roll, using their then-current inventory and normal budgets. Generation waits until the NPC is alive, loaded, and otherwise eligible. Active followers defer additions; recruiting/dismissing them does not restart or cancel the saved countdown. Waiting, sleeping, and travel that advances game time count toward the interval. Real time spent outside the game does not. Long absences do not accumulate repeated refills; another theft is needed to start the next timer.



The timer never calls ResetInventory, restores outfits/weapons, deletes gifts, or replays unique original items. Ordinary pool items can be rolled again. If users explicitly put a rare item in a custom pool, that item can also be selected again under the normal rules.



Genuine engine inventory resets still allow a new generation cycle and supersede the victim countdown, preventing a second delayed refresh from the old timer. Remaining possessions still count after those resets. Consumption alone, cell changes, combat, death, save/load, edits, and updates do not independently refresh loot. NPCs first encountered dead receive nothing; items already generated remain normal corpse loot.



Pickpocket Reset is no longer needed or recommended just to replenish our pocket loot. Its full inventory resets can still happen if installed; our timer does not prevent another mod from restoring or deleting items. No companion mod is modified by this release.



## Eligibility



The ten playable races and mapped vanilla child/elder/vampire variants are eligible, including mod-added NPCs using those race forms. Hostile NPCs can participate. Children and prisoners use separate restricted pools and gold ranges. The player, creatures, Dremora, summoned/commanded/reanimated actors, active followers, and unrecognized custom races receive no additions. Dismissal preserves completion/timer state rather than forcing a new roll.



## Editable item pools



Edit Data/SKSE/Plugins/DynamicPickpocketLoot.ini to add vanilla/modded items, comment out unwanted entries, or set minimum wealth and child/prisoner permissions. See LOOT-POOLS.md. Restart Skyrim afterward. Changes apply to first generation, the next victim-timer refresh, or a genuine inventory reset; edits do not immediately clear completion flags. The file configures items, not the fixed three-day interval or general balance.



The readable INI replaces the full item list. Missing/unreadable/oversized files fall back to compiled defaults. A readable file with no usable entries allows coin generation only. Invalid entries are skipped and logged. No SPID or game-plugin slot is used.



## Updating and saves



Version 1.1.0 adds equipment restoration while retaining the existing DLL filename, serialization ID, item-pool INI, loot rules, and victim cooldown. Replace the older DLL rather than keeping both active. Matching .ess and .skse files belong together. Save schema 4 migrates schemas 1–3, preserving prior generation flags, selected wealth, reset cycles, RNG state, and victim deadlines. Updating does not reroll pocket loot; restoration begins observing missing native equipment independently. See RESTORATION.md for the full rules.

Timers and generation state follow the loaded save, including older branches. Unreadable/unsupported state fails closed rather than granting duplicate loot. Older binaries cannot read the new schema and will disable generation; keep backups if testing a downgrade. No uninstall/reinstall guarantee or external ledger is provided.



## Building and license



The source archive includes code, behavior tests, the exact tested INI, a verified vanilla identifier registry, CommonLibSSE-NG source, and dependency sources/ports. It excludes game masters, saves, compiler installations, build caches, and personal-note drafts.



Use Visual Studio C++ 2022 (C++23-capable), Windows SDK, CMake, Ninja, and vcpkg, or the workspace portable tool setup. scripts/bootstrap.ps1 prepares tools; scripts/build.ps1 -DependenciesOnly prepares dependencies; scripts/build.ps1 builds and tests the DLL. Generated vanilla tables are included, so Skyrim.esm is not needed to compile.



License: GPL-3.0-or-later with the CommonLibSSE-NG exceptions in EXCEPTIONS.md. Third-party notices accompany the release. The victim timer is independently implemented and includes no Pickpocket Reset assets or scripts.



## Equipment restoration in 1.1.0

Carried clothing is equipped before generation. Only the body slot and eligible feet slot receive a wealth/occupation-appropriate plain replacement after one day, or two days for beggars and Poor/Destitute NPCs. Armor wearers then receive weaker plain armor of the same light/heavy type after a value-based 2–3 days. Only the mod-created temporary clothing copy is removed; existing and stolen clothes are preserved.

Missing native weapon types receive plain, untempered replacements after one day. Beggars and Poor/Destitute NPCs receive no generated weapons. Ebony becomes steel or elven according to race; glass becomes elven; daedric, dragonbone, and dwarven become steel. Existing same-type weapons prevent duplicates. No ammunition is generated. See RESTORATION.md for exact eligibility, fallbacks, persistence, and validation limits.

Restoration covers body clothing for all eligible wealth levels, plus feet only for non-beggars above Poor wealth. No hats, gloves, helmets, or gauntlets are restored. Poor/Destitute NPCs and beggars receive body coverage only. Weapon rules are unchanged.



## 1.1.3: correct native item-data allocation

An audit found that the combined SE/AE facade allocated only 12 bytes for an ExtraDataList whose modern AE native constructor requires 32. This could overwrite heap memory when temporary clothing was created. Restoration now reserves at least 32 bytes from Skyrim's heap and uses placement construction without altering engine class layouts. New stage logs and regression checks accompany the fix. See CRASH-AUDIT.md for the evidence and runtime limits.

## 1.1.4: remove 1.1.1 equipment scheduling

Restoration equips immediately and can service multiple due slots and weapons in the same pass. The one-second spacing limit and delayed temporary-clothing cleanup are removed. Temporary clothing is cleaned up during armor replacement; inventory instance data is reacquired after unequipping. The 1.1.3 native allocation fix and save schema 4 remain. In-game crash testing is still required.

## 1.1.5: remove 1.1.3 reload gating

Restoration no longer requires cell/worldspace travel or an observed NPC unload/load. Due equipment can be restored during the normal five-second loaded-NPC scan, including after loading a save. The fixed reload cutoff and thirty-second opportunity window are removed. Saved deadlines and loot-generation flags are preserved. The allocation fix remains; in-game crash testing is still required.

## 1.1.6: guard uniforms and faster armor

Guards and the existing soldier/guard occupation receive replacement armor after 0.5 in-game days (12 hours), counted from observing an empty or clothing-covered native armor slot. Recognized vanilla hold-guard, Imperial, and Stormcloak armor is restored as the same plain base item, preserving its uniform and light/heavy type. These uniforms bypass temporary-clothing generation and can return directly after the twelve-hour wait. Already-carried clothing or armor is still used first. Only body and eligible feet are restored; helmets, gloves, and shields remain excluded. No enchantment or tempering is recreated.

Other guard armor uses the existing weaker same-type fallback with the shorter guard delay. Unrecognized mod-added uniforms are not copied automatically. Non-guard armor keeps its existing value-based 2�3 days after temporary clothing. Existing longer guard armor deadlines are capped at twelve hours from first observation after updating; already-due timers remain due. No new save schema or dependency is introduced. The allocation fix and normal loaded-NPC checks remain.

## 1.1.7: cheap native clothing for stall merchants

Members of vanilla JobStreetVendorFaction bypass wealth-based fine clothing and the generic merchant outfit pool. When clothing must be generated, restoration first scans their base inventory and default outfit, including level-appropriate nested leveled lists, for plain cheap clothing matching the empty slot. It chooses the cheapest eligible item (up to 25 base gold for body clothing or 10 for footwear), rejects ClothingRich items, and preserves the body/eligible-feet scope. Mod-added clothing in those native lists can qualify under the same rules. If no suitable native option exists, basic farm clothing is the fallback. Existing carried equipment is used first and is never deleted simply for being expensive. Existing timers and loot budgets are unchanged. Stall merchants without the street-vendor faction are not automatically identified by their physical location.

## 1.1.8: native cheap clothing priority for all NPCs

All eligible adult NPCs now search their native base inventory and default outfit, including level-appropriate nested leveled lists, for inexpensive plain clothing before using wealth/occupation fallback pools. This priority no longer requires the street-vendor faction. Existing clothing is equipped first; newly generated native clothes retain the body (25 gold maximum) and feet (10 gold maximum) limits and rich/enchantment/slot-overlap exclusions. Children retain their own native child-clothing restriction. Stall merchants still use basic farm clothing if no cheap native option exists; other adults use their established occupation fallback. Guard armor behavior is unchanged. No timers or saved generation flags are reset. Clothing previously generated is not retroactively deleted or swapped.

Read-only Skyrim.esm verification: Saadia (00013BA2) has BarkeepClothes01 (0005FB81), containing ClothesBarKeeper (0005B6A1, value 5) and ClothesBarKeeperShoes (0005B6A0, value 2). Both are eligible native cheap replacements. Live plugin overrides can change her outfit.

## 1.1.9: multiple native weapons and armor accessories

Native equipment discovery now preserves all entries of use-all leveled-list bundles, including nested bundles, rather than flattening a bow-and-sword loadout into one source. Carried native alternatives are retained; missing guaranteed random alternatives choose one inexpensive branch, not every possible weapon. Each supported missing weapon type has its own one-day timer, and every due type can be restored in the same scan. An existing weapon satisfies only its own type. Existing exclusions for poor/beggars/children/prisoners, staves, bound weapons, and ammunition remain. This restores one replacement per supported native weapon type rather than reproducing duplicate copies of the same type.

Native armor restoration now includes helmets and gauntlets, as well as body armor and boots. Generated clothing remains limited to body and eligible feet: no hats or cloth gloves are generated. Armor-only slots start their value-based 2�3-day timer when missing; guards retain twelve-hour armor replacement and matching recognized vanilla uniforms. Existing suitable armor is equipped first, and body-covering robes block accessory swaps that would strip the body. No shields are generated.

On loading a save, native equipment sources are rescanned once per encountered NPC so old saves gain omitted weapon types. Saved generation flags, existing weapon/armor deadlines, and temporary-item identities remain; schema 4 is unchanged. The memory-allocation fix remains.

Current release version is 1.1.3. It includes all current changes; numbered development sections above describe the development history and do not imply a feature rollback. Save schema remains 4.

## Common combat enemy loot in 1.1.1

Non-unique, non-essential, non-protected bandits, Forsworn, warlocks, combat-class NPCs, and Imperial/Stormcloak soldiers target half their usual generated gold after the jewelry/gem offset. Ordinary guards and civilian occupations retain normal loot. Food and ingredient chances double (maximum 95%); these categories are considered before other items, and coins-only odds halve. Existing stack limits, category shortfalls and strict valuables budgets remain unchanged. Empty purses still receive at least two coins when no other pocket contents exist. Existing gold is subtracted from the reduced target, never removed. Updates preserve generation flags: the adjustment applies on the next normal eligible generation, without rerolling NPCs. Skyrim gameplay validation remains pending.

## Supply replacement in 1.1.3

When an eligible common enemy receives a food or ingredient addition, there is a 75% chance those supplies replace the entire generated coin addition for that cycle. Existing coins are never removed. If no legal supplies can be generated, normal half-gold behavior remains, including the empty-inventory two-coin fallback. If supplies cannot be applied at runtime, the original coin shortfall is restored, accounting for any intervening inventory additions. This does not affect civilian loot or reroll existing generation state.

# 1.1.4 cell-change re-equipping

Carried clothing/armor in empty slots is no longer equipped on periodic checks. Each affected slot waits until the NPC is observed in a different cell after the empty slot was observed. Repeated checks and player-only travel do not qualify. Already occupied slots cancel the pending operation. Missing items retain the existing replacement timers.

Vanilla scenes, combat, disabled AI and killmoves defer all equipment operations. This general carried-item guard does not require OStim or Papyrus. Custom frameworks that remove equipment from inventory entirely or change cells mid-scene without a vanilla scene/AI marker may still require an explicit integration.

Cell tracking is transient and clears on save load: loading does not count as travel; another observed NPC cell change is required. No save-schema change. Automated checks cover same-cell repetition, no-cell transitions, per-slot readiness and cancellation; OStim and in-game travel testing remain pending.
