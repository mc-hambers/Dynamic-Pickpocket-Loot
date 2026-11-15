# Dynamic Pickpocket Loot 1.2.4

Changes since 1.2.0:

- Moved initial NPC discovery to save loading and made loot, restoration and reset processing event-driven.
- Spread changed-NPC and due-timer work across small, deduplicated batches.
- Automatically recreate the missing editable loot-pool INI with all 87 default entries.
- Preserve existing custom settings, generated loot and save history without rerolling NPCs.

Existing saves are supported; no new game or save cleaner is required. Build and automated checks passed. These checks do not prove every gameplay path or hitch improvement.
