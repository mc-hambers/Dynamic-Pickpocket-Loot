# 1.1.4 cell-change re-equipping

Carried clothing/armor in empty slots is no longer equipped on periodic checks. Each affected slot waits until the NPC is observed in a different cell after the empty slot was observed. Repeated checks and player-only travel do not qualify. Already occupied slots cancel the pending operation. Missing items retain the existing replacement timers.

Vanilla scenes, combat, disabled AI and killmoves defer all equipment operations. This general carried-item guard does not require OStim or Papyrus. Custom frameworks that remove equipment from inventory entirely or change cells mid-scene without a vanilla scene/AI marker may still require an explicit integration.

Cell tracking is transient and clears on save load: loading does not count as travel; another observed NPC cell change is required. No save-schema change. Automated checks cover same-cell repetition, no-cell transitions, per-slot readiness and cancellation; OStim and in-game travel testing remain pending.
