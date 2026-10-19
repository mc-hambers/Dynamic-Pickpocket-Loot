# Dynamic Pickpocket Loot 1.1.5

Inventory-aware NPC pocket loot and equipment restoration for Skyrim SE/AE.

## 1.1.5 update

Carried clothing/armor waits for either the NPC to leave and return to the marked cell, or the player to leave and re-enter it with the marked NPC present. Leaving alone and unrelated cell travel do not qualify. Occupied slots cancel pending re-equips. Player travel uses native events and checks marked actors only; NPC travel uses the existing restoration checks, with no new polling loop. Missing equipment keeps its normal replacement timers. Loading clears transient travel marks and requires a fresh round trip. No new dependency, Papyrus script or save schema.

Build, 65,802 behavior checks, DLL loader/metadata, and archive validation passed. Actual OStim and cell-travel testing remain pending. Frameworks that remove clothing entirely or change cells during custom scenes may need explicit integration. See [changes and limits](CHANGES-1.1.5.md).

## Download and source

Use the [1.1.5 release](https://github.com/mc-hambers/Dynamic-Pickpocket-Loot/releases/tag/source-1.1.5) for the runtime ZIP. Install through your mod manager, replacing the previous DLL. Requires matching SKSE, Address Library and the Microsoft Visual C++ x64 runtime. No SPID or ESP required.

[Complete corresponding source and build files](Dynamic-Pickpocket-Loot-source.zip), including tests, configuration, dependency sources and notices. Native source is also browsable in [src/](src/). Extract the source archive for the complete build tree; the repository root is a distribution index.

[Matching precompiled CommonLib](CommonLibSSE.lib.xz) is unchanged. Keep [Restore-CommonLib.py](Restore-CommonLib.py) beside it and run `python Restore-CommonLib.py <extracted-source-folder>` to restore and verify it. This is a development dependency, not a Skyrim runtime file.

## License

Copyright (C) 2026 Maxmc. GPL-3.0-or-later, WITHOUT ANY WARRANTY. See [LICENSE.txt](LICENSE.txt). Dependency licenses and CommonLib exceptions remain in the complete source archive.
