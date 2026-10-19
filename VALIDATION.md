# Dynamic Pickpocket Loot 1.1.3 — validation



## Completed automated and file checks



- Release x64 DLL compiled with portable MSVC, CMake/Ninja, and CommonLibSSE-NG, SE/AE enabled, VR disabled.

- CoreBehavior passes generation, inventory shortfalls, strict budgets, enchantment accounting helpers, editable INI parsing/default equivalence, per-victim timer lifecycle, and schema-1/2/3 migration and restoration rule checks. The exact count is recorded in validation-results.json.

- Timer checks include no early expiry, no extension by repeated theft, one-shot refresh after a long absence, game-clock validation, save/load and branches, pending-theft save handling, natural-reset supersession, and transfer direction/mode filters.

- Shipped INI matches all 87 compiled vanilla entries and permissions. Install/source archives are checked for corruption and byte equality.

- Windows DLL loader, SKSE exports, name Dynamic Pickpocket Loot, and version 1.1.3 checked.

- Role/race/pool identifiers were previously verified against Skyrim.esm using a read-only parser.

- Read-only installed 1.7.104 executable / Address Library audit checks Actor and Character ResetInventory vtable slot 0x8A and the added ItemsPickpocketed event-source/global mappings. These checks validate mapped locations, not runtime semantics.



## 1.1.3 follow-up

Valuation bypass, lazy native branch selection, and restricted low-tier native crossbow replacement regressions pass. The public 1.1.2 crash has not been reproduced; the exact Crossbow Integration plugin is unavailable locally. No live compatibility claim is made.

## Existing live evidence



The user's v0.1.1 log confirmed the original form-resolution failure; v0.1.3 logs subsequently confirmed all default forms resolving, generation running for eligible NPCs, and prior saved completion records loading. This evidence does not cover the current timer, editable-pool adapter, or enchantment-value changes. The present 1.0.0 timer build has not yet been run in Skyrim.



## In-game acceptance checklist



1. Load an existing schema-2 save. Check that prior completed NPCs receive no new roll merely from upgrading and that the INI loads without warnings. No retroactive victim timers should appear.

2. Successfully steal from a living eligible NPC. Confirm a victim-timer-started log entry. Simply opening/closing the menu, failed attempts, gifts/reverse pickpocketing, trade, follower exchanges, corpse looting, and unrelated scripted transfers must not start timers. Check actual event ordering in vanilla and the user's pickpocket UI mods.

3. Repeated theft during the same countdown must retain the original deadline. Save immediately after theft, reload, and confirm the same deadline is preserved.

4. Before three game days, no refill. After three game days, close paused menus and allow a periodic scan. The loaded eligible victim should get one shortfall-only generation; original equipment/rare possessions must not be restored and gifts must remain.

5. Wait several weeks away, then return. At most one deferred roll occurs, with no further refresh unless pickpocketed again or genuinely reset. Travel/sleep/wait advance game time; real time outside the game does not.

6. Recruit/dismiss a victim before/after the deadline. Followers defer additions without restarting timers. Dead/ineligible/unloaded victims must not be forcibly initialized; eligibility resumes normally for surviving references.

7. Trigger a genuine full or partial inventory reset while a timer exists. The real reset should supersede that timer without a second delayed refresh. Check hook/event duplication and other reset mods.

8. Add a modded item to the INI, remove a default entry, and test its next generation. Test missing plugins, light-plugin IDs, wrong categories, and empty pools. Observe budget limits, including enchanted modded jewelry.

9. Check built-in and custom enchanted rings/necklaces against plain copies; expensive outfits and weapons must not reduce gold. Quest valuables remain untouched. Explicitly configured rare items remain repeatable pool contents.

10. Check supported SE/AE deployment runtimes and the complete mod list separately. Keep matched ESS/SKSE backups; older binaries do not support schema 4.



## Material boundaries



No game files were modified or Skyrim launched by the automated checks. Event semantics, UI timing, delayed notifications, follower transitions, natural/partial reset behavior, enchantment valuation, and arbitrary custom items still require in-game acceptance testing. The version label 1.0.0 does not change this boundary. See research/v1.0.0-victim-timer.md for the implementation audit.



## Version 1.1.0 restoration acceptance tests — not yet run in Skyrim

- Steal body clothes and boots independently; verify body restoration at all wealth levels and boots only for non-beggars above Poor wealth. Steal gloves and hats/helmets and confirm no restoration. Verify occupied overlapping slots are preserved and exact one-day/two-day deadlines survive repeated encounters and save/load.
- Give clothing or a matching weapon back before its deadline. Confirm the NPC uses it and adds no duplicate, including when an item cannot equip because of another mod.
- Test beggars and Poor/Destitute NPCs: two-day clothing and no generated weapons. Test smith, innkeeper, merchant, noble, mage, child, and prisoner outfits.
- For light/heavy armor wearers, confirm the 2–3 day value-based armor phase begins on temporary clothing, preserves armor type, and supplies a weaker unenchanted piece.
- Mix existing clothes with generated temporary clothes. Only the exact generated copy may be removed on the armor swap. Steal, sell, drop, transfer, and reload that copy; never reclaim a player-held copy or delete an unrelated item.
- Test every supported weapon kind, including separate battleaxe/warhammer and bow/crossbow paths. Ebony should become steel for non-elves and elven for High/Dark/Wood Elves; glass becomes elven; daedric/dragonbone/dwarven become steel. Confirm no enchantment or tempering and no stronger or more expensive replacement.
- Test nested native lists, optional chance-none sources, real resets, follower recruitment/dismissal, combat, scenes, and NPC base/template changes.
- Load a schema-3 save with active pickpocket timers and already-generated loot. Upgrade must preserve both while adding empty restoration state. Verify matched saves, FormID remapping, and modified load orders.

Restoration is implemented but these runtime behaviors are not verified by the automated tests. The package does not install itself into the game.

The body/feet-only policy also needs an old-save test: existing head/hand and poor-NPC boot deadlines must cancel without deleting already-owned items.


## 1.1.1 crash investigation

The inspected 1.1.0 log resolves 70 restoration bindings and 7 material keywords, loads 115 saved actors, and records successful pickpocket timers. Its final two lines restore farm body clothes and boots to Whiterun Guard 0003706A. The user reports that time was accelerated using timescale. No crash logger stack report was found in the inspected game/SKSE log directories, so no specific faulting instruction is established.

The original adapter used forced immediate EquipObject calls and reused the incoming ExtraDataList pointer after AddObjectToContainer. It also unequipped and removed temporary clothing through the same pointer and could update multiple equipment slots in one pass. Version 1.1.1 replaces those paths with queued by-form equipping, one equipment operation per actor/pass, a real-time spacing guard, and deferred cleanup of already-unworn temporary clothes after replacement armor is confirmed worn.

Retest the same clothes thefts at the same increased timescale, including the guard, body+boots deadlines, poor NPCs at two days, and the later armor deadline. Check queued-equipment and cleanup logs. The crash is not confirmed fixed until that runtime test succeeds.



## 1.1.3 allocation audit

Pre-fix compiled disassembly confirms a 12-byte allocation before the native ExtraDataList constructor. Read-only inspection of the installed 1.7.104 constructor confirms pointer writes at offsets 8 and 16 plus lock construction at 24. Runtime allocation now reserves at least 32 bytes. The new guard-buffer regression covers this cross-runtime facade/native-size mismatch, null allocation, and larger facades. Equipment relocation checks and staged allocation/add/equip logs were added. The same-timescale Skyrim reproduction still needs to be run; no claim is made that the original crash was debugged to its sole faulting instruction.

## 1.1.4 scheduling rollback

Build and existing core checks pass. Removed queued equipment, the one-operation-per-pass limit, one-second spacing, and deferred cleanup. Immediate equipment and same-pass cleanup need in-game validation; core checks do not execute native equipment calls. Save schema 4 and allocation regression coverage remain.

## 1.1.5: remove 1.1.3 reload gating

Restoration no longer requires cell/worldspace travel or an observed NPC unload/load. Due equipment can be restored during the normal five-second loaded-NPC scan, including after loading a save. The fixed reload cutoff and thirty-second opportunity window are removed. Saved deadlines and loot-generation flags are preserved. The allocation fix remains; in-game crash testing is still required.

## 1.1.6 guard armor checks

Automated coverage checks twelve-hour guard timing, unchanged ordinary armor timing, plain-uniform restrictions, vanilla hold/civil-war body and feet bindings, and shortening existing deadlines without postponing them. Runtime checks remain: remove a Whiterun guard cuirass and eligible boots, verify the logged deadline, advance twelve hours and verify the correct uniform returns; repeat with Imperial and Stormcloak uniforms. Verify normal civilians still follow ordinary armor timing, existing carried gear prevents duplicates, and modded or enchanted sources use ordinary plain fallbacks. Gameplay has not been tested by this build.

## 1.1.7 stall merchant coverage

Core checks cover inexpensive body/feet limits and rich-clothing exclusion. Build, DLL verification, and package checks pass. Native leveled-list traversal reuses the existing depth/node/cycle bounds. Runtime checks remain: strip a JobStreetVendorFaction merchant, advance the clothing deadline, confirm inexpensive native clothing takes priority even at Wealthy/Noble wealth, then verify basic fallback when their native outfit has no cheap candidate. Confirm existing carried clothes and ordinary shopkeepers retain their behavior.

## 1.1.8 universal native-clothing priority

Build and existing core checks pass. Source inspection confirms the native cheap-clothing lookup is unconditional for eligible adults and runs before wealth/occupation fallback. The installed vanilla master confirms Saadia's barkeeper outfit and qualifying base values. Native equipment selection and actual equip behavior still require in-game verification; test Saadia after removing previously generated clothes and advancing her replacement deadline. Also test a rich non-stall NPC with a cheap native outfit, and a native outfit containing no eligible cheap clothing.

## 1.1.9 equipment coverage

Automated tests cover missing and partly carried bow/sword use-all bundles, conservative random alternatives, optional branch exclusion, all observed native alternatives, separate armor accessory eligibility, and preservation of armor deadlines when clothing slots are disabled. Build and core tests pass. Runtime validation remains: remove a native bow and sword, advance one day, and verify both weaker types return without duplicate additions on later scans. Repeat while retaining one weapon type. Remove native helmets and gauntlets from ordinary armor wearers and guards; verify plain same-type armor after their appropriate timers and no generated cloth hats/gloves. Check an older save and nested use-all weapon lists. No live gameplay result is claimed.

Current release version is 1.1.3. It includes all current changes; numbered development sections above describe the development history and do not imply a feature rollback. Save schema remains 4.

## Common combat enemy loot in 1.1.1

Non-unique, non-essential, non-protected bandits, Forsworn, warlocks, combat-class NPCs, and Imperial/Stormcloak soldiers target half their usual generated gold after the jewelry/gem offset. Ordinary guards and civilian occupations retain normal loot. Food and ingredient chances double (maximum 95%); these categories are considered before other items, and coins-only odds halve. Existing stack limits, category shortfalls and strict valuables budgets remain unchanged. Empty purses still receive at least two coins when no other pocket contents exist. Existing gold is subtracted from the reduced target, never removed. Updates preserve generation flags: the adjustment applies on the next normal eligible generation, without rerolling NPCs. Skyrim gameplay validation remains pending.

## Supply replacement in 1.1.3

When an eligible common enemy receives a food or ingredient addition, there is a 75% chance those supplies replace the entire generated coin addition for that cycle. Existing coins are never removed. If no legal supplies can be generated, normal half-gold behavior remains, including the empty-inventory two-coin fallback. If supplies cannot be applied at runtime, the original coin shortfall is restored, accounting for any intervening inventory additions. This does not affect civilian loot or reroll existing generation state.

# 1.1.4 cell-change re-equipping

Carried clothing/armor in empty slots is no longer equipped on periodic checks. Each affected slot waits until the NPC is observed in a different cell after the empty slot was observed. Repeated checks and player-only travel do not qualify. Already occupied slots cancel the pending operation. Missing items retain the existing replacement timers.

Vanilla scenes, combat, disabled AI and killmoves defer all equipment operations. This general carried-item guard does not require OStim or Papyrus. Custom frameworks that remove equipment from inventory entirely or change cells mid-scene without a vanilla scene/AI marker may still require an explicit integration.

Cell tracking is transient and clears on save load: loading does not count as travel; another observed NPC cell change is required. No save-schema change. Automated checks cover same-cell repetition, no-cell transitions, per-slot readiness and cancellation; OStim and in-game travel testing remain pending.
