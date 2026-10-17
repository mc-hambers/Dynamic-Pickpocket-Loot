#pragma once
#include "Core.h"
#include <cmath>
#include <map>
#include <mutex>

namespace PL {
inline constexpr double victimCooldownDays=3.0;
inline bool armVictimTimer(State& state,double now) {
    // The first successful theft starts a countdown. Further thefts cannot
    // postpone it, nor replace an expired but not-yet-consumed countdown.
    if (!std::isfinite(now) || now<0 || state.pickpocketDue>=0) return false;
    const auto due=now+victimCooldownDays;
    if (!std::isfinite(due) || due<=now) return false;
    state.pickpocketDue=due;
    return true;
}
inline bool victimCooldownBlocks(const State& state,double now) {
    return state.pickpocketDue>=0 && (!std::isfinite(now) || now<state.pickpocketDue);
}
inline bool consumeVictimTimer(State& state,double now) {
    if (state.pickpocketDue<0 || !std::isfinite(now) || now<state.pickpocketDue) return false;
    resetCycle(state);
    return true;
}
inline bool isVictimTransfer(std::uint32_t from,std::uint32_t to,int count,std::uint32_t menuTarget,bool pickpocketMode) {
    return pickpocketMode && count>0 && from!=0 && from!=0x14 && to==0x14 && from==menuTarget;
}
// Save callbacks and deferred tasks consume the same first-theft timestamps.
// Saving before the deferred task runs therefore cannot lose a new timer.
class VictimInbox {
public:
    bool request(std::uint32_t actor,double now) {
        if (!actor || !std::isfinite(now) || now<0) return false;
        std::scoped_lock lock(mutex);
        return pending.try_emplace(actor,now).second;
    }
    std::map<std::uint32_t,double> drain() {
        std::scoped_lock lock(mutex);
        std::map<std::uint32_t,double> out;
        out.swap(pending);
        return out;
    }
    void clear() { std::scoped_lock lock(mutex); pending.clear(); }
private:
    std::mutex mutex;
    std::map<std::uint32_t,double> pending;
};
}
