# Dynamic Pickpocket Loot 1.2.0

[Download the install ZIP](Dynamic-Pickpocket-Loot-v1.2.0.zip) · [Versioned release](https://github.com/mc-hambers/Dynamic-Pickpocket-Loot/releases/tag/source-1.2.0) · [Complete source archive](Dynamic-Pickpocket-Loot-source.zip)

## Current changes

- Move jewelry auto-equipping entirely into DPL; PCR is not required.
- Consider the most valuable existing rings and amulets first, including enchantment value, while respecting occupied body slots.
- Persist one completed check per NPC in DPL's SKSE co-save. Only a genuine inventory reset re-arms the check; the three-day victim refill does not.
- Defer equipping during combat, sleep and active scenes. Existing worn jewelry continues to count toward loot/wealth accounting.
- Retain the cycle API v1 export and the existing loot, restoration and victim-timer systems.
- Preserve existing loot-pool/settings INIs during installation. Runtime DLL filename remains PickpocketingLoot.dll.

## Installation and settings

Requires SKSE and the matching Address Library. PCR is not required.

On first launch, `Data/SKSE/Plugins/DynamicPickpocketLoot_Settings.ini` is generated.
Set `[Settings] EquipJewelry=1` and restart Skyrim to enable jewelry equipping;
it defaults to 0/off. The existing `DynamicPickpocketLoot.ini` item pool is
preserved. If absent, built-in loot defaults are used; an editable pool template
is included in the source archive under `config/`.

## Source and build dependencies

The complete corresponding source, dependency sources, build scripts and original
license notices are in [Dynamic-Pickpocket-Loot-source.zip](Dynamic-Pickpocket-Loot-source.zip). Extract it before
building. The loose `src/`, tests and scripts mirror this build for browsing.
The matching precompiled CommonLib remains [CommonLibSSE.lib.xz](CommonLibSSE.lib.xz).
Use `python Restore-CommonLib.py <extracted-source-root>` to restore and verify it.
CommonLib can alternatively be rebuilt from the included source. See BUILD.md
and SOURCE-BUILD.txt for compiler/SDK/Papyrus requirements.

## Validation

65,802 behavior checks, native build, package integrity, cycle API export and version metadata checks passed. **In-game testing remains pending.** Automated tests do not establish
live MCM, confiscation, equipping, AI or third-party compatibility behavior.

## License

Copyright (C) 2026 Maxmc. GPL-3.0-or-later, without warranty.
See [LICENSE.txt](LICENSE.txt). Third-party notices and applicable exceptions
are preserved in the complete source archive and runtime licensing notices.
