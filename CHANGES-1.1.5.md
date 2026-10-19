# 1.1.5 player or NPC round-trip re-equipping

Carried clothing/armor waits until either the marked NPC leaves and returns to the cell where an empty slot was observed, or the player exits and re-enters that cell with the marked NPC present. Leaving alone, unrelated cell changes, initial entry, and loading a save do not qualify. Occupied slots cancel their pending re-equip; equipment is checked again before use.

Player travel uses native cell enter/leave events and visits marked NPCs only. NPC round trips use the existing loaded-NPC restoration checks; no additional polling loop or full actor scan was introduced. Only actors with empty slots and carried equipment receive transient travel state. Normal loot and missing-item timer checks retain their established periodic scan. Very brief NPC trips entirely between observations can be missed and remain deferred safely.

No new dependency, Papyrus script or save schema. Pending travel state clears on save load, requiring a new round trip. Scene/combat/disabled-AI safeguards remain. Gameplay, OStim and native travel-event ordering require in-game testing.
