# Dynamic Pickpocket Loot 1.1.4

Inventory-aware NPC pocket loot and equipment restoration for Skyrim SE/AE.

## 1.1.4 update

Carried clothing and armor in empty slots wait for that NPC to change cells before re-equipping. Occupied slots cancel pending re-equips. Missing equipment keeps its normal replacement timers. Vanilla scenes, combat, disabled AI and killmoves defer equipment operations. Loading a save does not count as changing cells. No new dependency, Papyrus script or save schema.

Build, 65,795 behavior checks, DLL loader/metadata, and archive validation passed. Actual OStim and cell-travel testing remain pending. Frameworks that remove clothing entirely or change cells during custom scenes may need explicit integration. See [changes and limits](CHANGES-1.1.4.md).

## Download and source

Use the [1.1.4 release](https://github.com/mc-hambers/Dynamic-Pickpocket-Loot/releases/tag/source-1.1.4) for the runtime ZIP. Install through your mod manager, replacing the previous DLL. Requires matching SKSE, Address Library and the Microsoft Visual C++ x64 runtime. No SPID or ESP required.

[Complete corresponding source and build files](Dynamic-Pickpocket-Loot-source.zip), including tests, configuration, dependency sources and notices. Native source is also browsable in [src/](src/). Extract the source archive for the complete build tree; the repository root is a distribution index.

[Matching precompiled CommonLib](CommonLibSSE.lib.xz) is unchanged. Keep [Restore-CommonLib.py](Restore-CommonLib.py) beside it and run `python Restore-CommonLib.py <extracted-source-folder>` to restore and verify it. This is a development dependency, not a Skyrim runtime file.

## License

Copyright (C) 2026 Maxmc. GPL-3.0-or-later, WITHOUT ANY WARRANTY. See [LICENSE.txt](LICENSE.txt). Dependency licenses and CommonLib exceptions remain in the complete source archive.
