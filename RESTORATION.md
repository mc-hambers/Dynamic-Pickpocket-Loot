# Clothing, armor, and weapon restoration â€” 1.1.0

Restoration operates alongside pocket-loot generation. It does not reset an NPC's inventory or their three-day pocket-loot victim timer. Existing possessions are checked before adding replacements.

## Clothing

- Generated clothing covers body and eligible feet only, where native inventory/outfit establishes the need. Armor restoration additionally covers native helmets and gauntlets. Jewelry and shields remain excluded.
- Body coverage is available at every wealth level. Feet are restored only for NPCs who are not beggars and whose saved wealth is above Poor: Common, Comfortable, Wealthy, or Noble. This restriction applies to generated clothing; native armor boots, helmets, and gauntlets use armor restoration regardless of the clothing-slot policy.
- Carried clothing is equipped first. Carried armor can fill an empty slot as well. No new copy is created when a matching slot is already supplied in inventory, even if another mod or an incompatible item prevents equipping it.
- Missing clothing is supplied after one game day. Beggars and NPCs whose saved wealth is Destitute or Poor take two days.
- Low-wealth NPCs use rough/beggar clothing. Ordinary workers use everyday clothes; smiths, innkeepers, and merchants have their occupation's clothing; wealthy/noble NPCs use fine clothes; mages and priests use plain robes. Generated body clothing is plain body coverage, such as a tunic or an unhooded robe; no hats, gloves, or hooded combinations are generated.
- Children never receive adult fallback clothes; only a suitable plain native clothing record can be reused. Prisoners use rough clothing and never receive generated weapons.
- Existing equipment is not stripped just to apply a profile. Occupied slots and overlapping worn items block generation.

## Returning to armor

- NPCs whose native equipment is armor receive temporary clothing while waiting for a weaker, plain armor replacement of the same light/heavy type.
- The armor delay begins when temporary/carried clothing covers the missing armor slot. It is `2 + clamp(original base value / 1000, 0, 1)` game days: two days for value 0, two and a half for value 500, three for value 1000 or higher.
- Carried matching armor is equipped before generating another piece at the deadline. A newly worn armor piece also satisfies the restoration.
- Generated armor cannot exceed the original piece's base value or armor rating. Heavy profiles prefer steel, or iron for poor NPCs. Light profiles prefer leather, with elven options for sufficiently wealthy High/Dark/Wood Elves; cheaper matching fallbacks remain available.
- Only the exact temporary clothing copy created by this mod is removed during an armor swap. Its saved unique-item identity follows Skyrim's identity-change events. Existing clothing is unequipped by normal equipment handling and remains in inventory. Stolen temporary clothes are not reclaimed from the player.
- A boot swap waits if it would strip body-covering clothes prematurely. Head/hand clothing timers and ineligible clothing-foot timers are canceled; armor deadlines and existing items are retained.

## Weapons

- Every missing supported native weapon type is restored after one game day, so a bow and sword can both return. Beggars and Destitute/Poor NPCs receive no new weapons. Children and prisoners receive none either.
- The source is the NPC's base inventory/default outfit, including level-eligible native leveled-list entries. Carried candidates identify the actual selected equipment where possible; a deterministic inexpensive native option is used otherwise. Optional chance-none inventory lists do not arm an NPC when no corresponding item is present.
- Any existing weapon of the same type satisfies the requirement. A player-given weapon therefore prevents duplicate generation; it is not removed or downgraded.
- Supported types: sword, dagger, war axe, mace, greatsword, battleaxe, warhammer, bow, and crossbow. Battleaxes and warhammers remain distinct. Staves, bound weapons, shields, and ammunition are outside this feature.
- Generated weapons are unenchanted and untempered. Original rare weapons, enchantments, and tempering are not recreated.
- Ebony becomes steel for non-elves and elven for High/Dark/Wood Elves.
- Glass becomes elven.
- Daedric, dragonbone, and dwarven become steel.
- Steel remains steel. Cheap iron remains iron to avoid an upgrade.
- Native elven weapons remain elven for elves or wealthy NPCs; other eligible owners receive steel.
- Unknown materials use steel as the first fallback. A compatible cheaper fallback is used if steel would exceed the original's value or damage. No incompatible weapon type is substituted.
- Normal missing native equipment is repaired regardless of whether it was stolen, disarmed, or removed by another system. Player-added weapon types do not establish native restoration eligibility.

## Timers, updates, and performance

Dead, disabled, deleted, summoned, non-human, and currently following actors are excluded under the existing loot eligibility rules. Restoration is also deferred during combat and scenes. Following/dismissal does not reset saved restoration state; elapsed deadlines can be serviced after dismissal.

The existing five-second main-thread scan observes missing gear and services elapsed deadlines while the NPC remains loaded. Healthy equipped actors need one inventory snapshot; absent equipment starts one saved deadline rather than restarting it at every encounter. Missed time does not accumulate extra replacements. Inventory callbacks are guarded against re-entry.

Save schema 4 retains loot-generation flags, wealth, victim deadlines, equipment deadlines, source records, and exact temporary clothing identifiers. Schemas 1â€“3 migrate without granting new pocket loot just because of an update. Keep the matching ESS/SKSE pair. Older binaries cannot read the new schema.

No ESP, SPID, or extra mod dependency is added. Equipment replacement pools are compiled vanilla bindings, separate from the editable pocket-loot item INI.

## Validation boundary

Compilation, core rules, serialization/migration, DLL metadata, and package checks are automated. In-game equipment behavior still requires testing: actual outfits/templates, clothing meshes, equip permissions, overlap handling, item-identity notifications, enchantment removal, leveled-list compatibility, and interaction with wardrobe/AI mods. These are not proven by a successful build.



## 1.1.4: remove 1.1.1 equipment scheduling

Restoration equips immediately and can service multiple due slots and weapons in the same pass. The one-second spacing limit and delayed temporary-clothing cleanup are removed. Temporary clothing is cleaned up during armor replacement; inventory instance data is reacquired after unequipping. The 1.1.3 native allocation fix and save schema 4 remain. In-game crash testing is still required.

## 1.1.5: remove 1.1.2 reload gating

Restoration no longer requires cell/worldspace travel or an observed NPC unload/load. Due equipment can be restored during the normal five-second loaded-NPC scan, including after loading a save. The fixed reload cutoff and thirty-second opportunity window are removed. Saved deadlines and loot-generation flags are preserved. The allocation fix remains; in-game crash testing is still required.

## 1.1.6: guard uniforms and faster armor

Guards and the existing soldier/guard occupation receive replacement armor after 0.5 in-game days (12 hours), counted from observing an empty or clothing-covered native armor slot. Recognized vanilla hold-guard, Imperial, and Stormcloak armor is restored as the same plain base item, preserving its uniform and light/heavy type. These uniforms bypass temporary-clothing generation and can return directly after the twelve-hour wait. Already-carried clothing or armor is still used first. Only body and eligible feet are restored; helmets, gloves, and shields remain excluded. No enchantment or tempering is recreated.

Other guard armor uses the existing weaker same-type fallback with the shorter guard delay. Unrecognized mod-added uniforms are not copied automatically. Non-guard armor keeps its existing value-based 2–3 days after temporary clothing. Existing longer guard armor deadlines are capped at twelve hours from first observation after updating; already-due timers remain due. No new save schema or dependency is introduced. The allocation fix and normal loaded-NPC checks remain.

## 1.1.7: cheap native clothing for stall merchants

Members of vanilla JobStreetVendorFaction bypass wealth-based fine clothing and the generic merchant outfit pool. When clothing must be generated, restoration first scans their base inventory and default outfit, including level-appropriate nested leveled lists, for plain cheap clothing matching the empty slot. It chooses the cheapest eligible item (up to 25 base gold for body clothing or 10 for footwear), rejects ClothingRich items, and preserves the body/eligible-feet scope. Mod-added clothing in those native lists can qualify under the same rules. If no suitable native option exists, basic farm clothing is the fallback. Existing carried equipment is used first and is never deleted simply for being expensive. Existing timers and loot budgets are unchanged. Stall merchants without the street-vendor faction are not automatically identified by their physical location.

## 1.1.8: native cheap clothing priority for all NPCs

All eligible adult NPCs now search their native base inventory and default outfit, including level-appropriate nested leveled lists, for inexpensive plain clothing before using wealth/occupation fallback pools. This priority no longer requires the street-vendor faction. Existing clothing is equipped first; newly generated native clothes retain the body (25 gold maximum) and feet (10 gold maximum) limits and rich/enchantment/slot-overlap exclusions. Children retain their own native child-clothing restriction. Stall merchants still use basic farm clothing if no cheap native option exists; other adults use their established occupation fallback. Guard armor behavior is unchanged. No timers or saved generation flags are reset. Clothing previously generated is not retroactively deleted or swapped.

Read-only Skyrim.esm verification: Saadia (00013BA2) has BarkeepClothes01 (0005FB81), containing ClothesBarKeeper (0005B6A1, value 5) and ClothesBarKeeperShoes (0005B6A0, value 2). Both are eligible native cheap replacements. Live plugin overrides can change her outfit.

## 1.1.9: multiple native weapons and armor accessories

Native equipment discovery now preserves all entries of use-all leveled-list bundles, including nested bundles, rather than flattening a bow-and-sword loadout into one source. Carried native alternatives are retained; missing guaranteed random alternatives choose one inexpensive branch, not every possible weapon. Each supported missing weapon type has its own one-day timer, and every due type can be restored in the same scan. An existing weapon satisfies only its own type. Existing exclusions for poor/beggars/children/prisoners, staves, bound weapons, and ammunition remain. This restores one replacement per supported native weapon type rather than reproducing duplicate copies of the same type.

Native armor restoration now includes helmets and gauntlets, as well as body armor and boots. Generated clothing remains limited to body and eligible feet: no hats or cloth gloves are generated. Armor-only slots start their value-based 2–3-day timer when missing; guards retain twelve-hour armor replacement and matching recognized vanilla uniforms. Existing suitable armor is equipped first, and body-covering robes block accessory swaps that would strip the body. No shields are generated.

On loading a save, native equipment sources are rescanned once per encountered NPC so old saves gain omitted weapon types. Saved generation flags, existing weapon/armor deadlines, and temporary-item identities remain; schema 4 is unchanged. The memory-allocation fix remains.

# 1.1.4 cell-change re-equipping

Carried clothing/armor in empty slots is no longer equipped on periodic checks. Each affected slot waits until the NPC is observed in a different cell after the empty slot was observed. Repeated checks and player-only travel do not qualify. Already occupied slots cancel the pending operation. Missing items retain the existing replacement timers.

Vanilla scenes, combat, disabled AI and killmoves defer all equipment operations. This general carried-item guard does not require OStim or Papyrus. Custom frameworks that remove equipment from inventory entirely or change cells mid-scene without a vanilla scene/AI marker may still require an explicit integration.

Cell tracking is transient and clears on save load: loading does not count as travel; another observed NPC cell change is required. No save-schema change. Automated checks cover same-cell repetition, no-cell transitions, per-slot readiness and cancellation; OStim and in-game travel testing remain pending.

# 1.1.5 player or NPC round-trip re-equipping

Carried clothing/armor waits until either the marked NPC leaves and returns to the cell where an empty slot was observed, or the player exits and re-enters that cell with the marked NPC present. Leaving alone, unrelated cell changes, initial entry, and loading a save do not qualify. Occupied slots cancel their pending re-equip; equipment is checked again before use.

Player travel uses native cell enter/leave events and visits marked NPCs only. NPC round trips use the existing loaded-NPC restoration checks; no additional polling loop or full actor scan was introduced. Only actors with empty slots and carried equipment receive transient travel state. Normal loot and missing-item timer checks retain their established periodic scan. Very brief NPC trips entirely between observations can be missed and remain deferred safely.

No new dependency, Papyrus script or save schema. Pending travel state clears on save load, requiring a new round trip. Scene/combat/disabled-AI safeguards remain. Gameplay, OStim and native travel-event ordering require in-game testing.
