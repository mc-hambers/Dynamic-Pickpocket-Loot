# Dynamic Pickpocket Loot 1.2.0

- Move jewelry auto-equipping entirely into DPL; PCR is not required.
- Consider the most valuable existing rings and amulets first, including enchantment value, while respecting occupied body slots.
- Persist one completed check per NPC in DPL's SKSE co-save. Only a genuine inventory reset re-arms the check; the three-day victim refill does not.
- Defer equipping during combat, sleep and active scenes. Existing worn jewelry continues to count toward loot/wealth accounting.
- Retain the cycle API v1 export and the existing loot, restoration and victim-timer systems.
- Preserve existing loot-pool/settings INIs during installation. Runtime DLL filename remains PickpocketingLoot.dll.

## Validation

65,802 behavior checks, native build, package integrity, cycle API export and version metadata checks passed. In-game gameplay and compatibility testing remain pending.
