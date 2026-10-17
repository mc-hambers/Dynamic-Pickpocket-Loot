#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>
#include "RestoreCore.h"

namespace PL {
enum class Wealth : std::uint8_t { Destitute, Poor, Common, Comfortable, Wealthy, Noble };
enum class Role : std::uint8_t { Beggar, Farmer, Smith, Guard, Hunter, Innkeeper, Merchant, Priest, Mage, Thief, Bandit, Noble, Adventurer, Fallback, Child, Prisoner };
enum class Category : std::uint8_t { Food, Jewelry, Gem, Potion, Ingredient, Magic, Lockpick, None };
constexpr std::size_t categories = 7;
constexpr std::size_t index(Category c) { return static_cast<std::size_t>(c); }
struct Profile {
    int goldLow, goldHigh, goldFloor, coinsOnlyChance, maxStacks, budgetLow, budgetHigh, maxValuables;
    std::array<int, categories> chances;
};
inline constexpr std::array<Profile, 6> profiles{{
    {0,4,0,50,1,0,10,1, {20,4,1,1,5,1,3}},
    {2,15,0,25,2,5,30,1, {30,12,3,5,10,2,6}},
    {8,35,2,10,3,15,75,2, {40,25,8,12,15,6,10}},
    {20,65,5,3,3,40,150,2, {45,40,15,20,20,12,12}},
    {40,120,10,1,4,100,350,3, {50,60,30,30,25,20,15}},
    {60,180,20,0,4,200,600,3, {55,75,45,35,30,25,15}}
}};
struct Random {
    std::uint64_t state;
    std::uint64_t next() {
        auto z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
    int between(int lo, int hi) { return lo + static_cast<int>(next() % static_cast<std::uint64_t>(hi - lo + 1)); }
    int lowRoll(int lo, int hi) { return std::min(between(lo,hi), between(lo,hi)); }
    bool chance(int percent) { return between(1,100) <= percent; }
};
struct Item {
    std::uint32_t form;
    Category category;
    int value;
    Wealth minimum = Wealth::Destitute;
    bool childSafe = false;
    bool prisonerSafe = false;
};
struct Inventory {
    int gold = 0;
    std::array<int, categories> counts{};
    int stacks = 0;
    int valuables = 0;
    std::int64_t valuablesValue = 0;
    bool hasPocketPossession() const {
        return gold > 0 || std::any_of(counts.begin(), counts.end(), [](int n) { return n > 0; });
    }
};
struct Addition { std::uint32_t form; int count; Category category; };
struct Plan {
    int gold = 0;
    int supplyReplacementGoldFallback = 0;
    std::vector<Addition> items;
    int maxStacks = 0;
    int maxValuables = 0;
    std::int64_t valuablesBudget = 0;
};
struct State {
    std::uint32_t actor = 0;
    std::uint32_t base = 0;
    std::uint64_t random = 0;
    std::uint32_t cycle = 0;
    bool generated = false;
    Role role = Role::Fallback;
    Wealth wealth = Wealth::Common;
    double pickpocketDue = -1.0; // Absolute game days; -1 means no victim countdown.
    Restore::State equipment;
};

inline Wealth selectWealth(Role role, Random& rng) {
    const int n = rng.between(1,100);
    switch (role) {
    case Role::Beggar: return n <= 85 ? Wealth::Destitute : Wealth::Poor;
    case Role::Farmer: return n <= 60 ? Wealth::Poor : Wealth::Common;
    case Role::Smith: return n <= 20 ? Wealth::Poor : n <= 70 ? Wealth::Common : Wealth::Comfortable;
    case Role::Guard: return n <= 80 ? Wealth::Common : Wealth::Comfortable;
    case Role::Hunter: return n <= 40 ? Wealth::Poor : Wealth::Common;
    case Role::Innkeeper: return n <= 80 ? Wealth::Comfortable : Wealth::Wealthy;
    case Role::Merchant: return n <= 60 ? Wealth::Comfortable : Wealth::Wealthy;
    case Role::Priest: return n <= 70 ? Wealth::Common : Wealth::Comfortable;
    case Role::Mage: return n <= 40 ? Wealth::Common : n <= 85 ? Wealth::Comfortable : Wealth::Wealthy;
    case Role::Thief: case Role::Adventurer: return n <= 50 ? Wealth::Common : n <= 90 ? Wealth::Comfortable : Wealth::Wealthy;
    case Role::Bandit: return n <= 50 ? Wealth::Poor : n <= 90 ? Wealth::Common : Wealth::Comfortable;
    case Role::Noble: return n <= 60 ? Wealth::Wealthy : Wealth::Noble;
    case Role::Child: case Role::Prisoner: return Wealth::Destitute;
    default: return n <= 35 ? Wealth::Poor : n <= 85 ? Wealth::Common : Wealth::Comfortable;
    }
}
inline bool favored(Role role, Category c, bool magicRole) {
    if (magicRole && (c == Category::Ingredient || c == Category::Magic)) return true;
    switch (role) {
    case Role::Beggar: return c == Category::Food;
    case Role::Farmer: case Role::Hunter: return c == Category::Food || c == Category::Ingredient;
    case Role::Smith: return c == Category::Food || c == Category::Gem;
    case Role::Guard: case Role::Adventurer: return c == Category::Food || c == Category::Potion;
    case Role::Innkeeper: return c == Category::Food;
    case Role::Merchant: case Role::Noble: return c == Category::Jewelry || c == Category::Gem;
    case Role::Priest: return c == Category::Jewelry || c == Category::Potion;
    case Role::Mage: return c == Category::Magic || c == Category::Ingredient;
    case Role::Thief: return c == Category::Jewelry || c == Category::Gem || c == Category::Lockpick;
    case Role::Bandit: return c == Category::Food || c == Category::Jewelry || c == Category::Lockpick;
    default: return false;
    }
}
inline Profile profile(Wealth w, Role r) {
    if (r == Role::Child) return {0,5,0,35,1,0,10,1,{35,4,0,0,10,0,0}};
    if (r == Role::Prisoner) return {0,3,0,70,1,0,10,1,{20,3,0,0,0,0,0}};
    return profiles[static_cast<std::size_t>(w)];
}
inline Plan generate(Role role, Wealth wealth, bool magical, const Inventory& existing, std::span<const Item> pool, Random& rng, bool combatEnemy = false) {
    Plan plan;
    combatEnemy = combatEnemy && role != Role::Child && role != Role::Prisoner;
    const auto p = profile(wealth, role);
    plan.maxStacks = p.maxStacks;
    plan.maxValuables = p.maxValuables;
    const int fallbackRoll = rng.between(2, role == Role::Prisoner ? 3 : 4);
    const int coinFallback = combatEnemy ? 2 : fallbackRoll;
    const bool coinsOnly = rng.chance(combatEnemy ? p.coinsOnlyChance / 2 : p.coinsOnlyChance);
    if (coinsOnly) {
        plan.gold = std::max(0, coinFallback - existing.gold);
        return plan;
    }
    int rolledGold = rng.lowRoll(p.goldLow,p.goldHigh);
    if (role == Role::Merchant || role == Role::Innkeeper || role == Role::Noble)
        rolledGold = std::max(rolledGold, rng.between(p.goldLow,p.goldHigh));
    auto adjusted = std::max<std::int64_t>(p.goldFloor, rolledGold - existing.valuablesValue / 4);
    // Halve the target, before subtracting existing coins. Never remove inventory.
    if (combatEnemy) adjusted /= 2;
    plan.gold = static_cast<int>(std::max<std::int64_t>(0, adjusted - existing.gold));
    auto counts = existing.counts;
    int stacksRemaining = std::max(0,p.maxStacks - existing.stacks);
    int valuablesRemaining = std::max(0,p.maxValuables - existing.valuables);
    plan.valuablesBudget = rng.lowRoll(p.budgetLow,p.budgetHigh);
    std::int64_t valueRemaining = std::max<std::int64_t>(0, plan.valuablesBudget - existing.valuablesValue);
    std::array<Category, categories> order{Category::Food,Category::Jewelry,Category::Gem,Category::Potion,Category::Ingredient,Category::Magic,Category::Lockpick};
    for (std::size_t n = order.size()-1; n > 0; --n) std::swap(order[n],order[rng.between(0,static_cast<int>(n))]);
    if (combatEnemy) std::stable_partition(order.begin(),order.end(),[](Category c) { return c == Category::Food || c == Category::Ingredient; });
    for (const auto c : order) {
        if (!stacksRemaining) break;
        int chance = std::min(95, p.chances[index(c)] * (favored(role,c,magical) ? 2 : 1));
        if (combatEnemy && (c == Category::Food || c == Category::Ingredient)) chance = std::min(95,chance * 2);
        if (!rng.chance(chance)) continue;
        int target = c == Category::Food ? rng.between(1,2) : (c == Category::Ingredient || c == Category::Lockpick) ? rng.between(1,3) : 1;
        if (role == Role::Child || role == Role::Prisoner) target = 1;
        const bool valuable = c == Category::Jewelry || c == Category::Gem;
        int deficit = std::max(0,target - counts[index(c)]);
        if (valuable) deficit = std::min(deficit,valuablesRemaining);
        if (!deficit) continue;
        std::vector<const Item*> choices;
        for (const auto& item : pool) {
            if (item.category != c || item.minimum > wealth) continue;
            if (role == Role::Child && !item.childSafe) continue;
            if (role == Role::Prisoner && !item.prisonerSafe) continue;
            if (valuable && item.value > valueRemaining) continue;
            choices.push_back(&item);
        }
        if (choices.empty()) continue;
        std::sort(choices.begin(),choices.end(),[](auto a, auto b) { return a->value == b->value ? a->form < b->form : a->value < b->value; });
        auto* selected = choices[rng.lowRoll(0,static_cast<int>(choices.size()-1))];
        if (valuable && selected->value > 0) deficit = std::min<int>(deficit,static_cast<int>(valueRemaining / selected->value));
        if (c == Category::Food) deficit = std::min(deficit,stacksRemaining);
        if (!deficit) continue;
        plan.items.push_back({selected->form,deficit,c});
        counts[index(c)] += deficit;
        --stacksRemaining;
        if (valuable) { valuablesRemaining -= deficit; valueRemaining -= static_cast<std::int64_t>(selected->value) * deficit; }
    }
    // Only replace coins when a legal supply addition actually made it into the plan.
    const bool suppliesAdded = std::any_of(plan.items.begin(),plan.items.end(),[](const Addition& a) {
        return a.category == Category::Food || a.category == Category::Ingredient;
    });
    if (combatEnemy && suppliesAdded && rng.chance(75)) {
        plan.supplyReplacementGoldFallback = plan.gold;
        plan.gold = 0;
    }
    const bool nonGoldPossession = std::any_of(existing.counts.begin(),existing.counts.end(),[](int n) { return n > 0; }) || !plan.items.empty();
    if (!nonGoldPossession && existing.gold + plan.gold < coinFallback)
        plan.gold = std::max(0,coinFallback - existing.gold);
    return plan;
}
inline void resetCycle(State& state) {
    ++state.cycle;
    state.generated = false;
    state.pickpocketDue = -1.0;
    state.equipment.initialized = false;
}
inline int boundedAddition(const Plan& plan,const Inventory& original,const Inventory& current,const Addition& item,int currentValue) {
    if (item.count <= 0 || current.stacks >= plan.maxStacks) return 0;
    const auto i=index(item.category);
    if (i >= categories) return 0;
    auto deficit=std::min<std::int64_t>(item.count,std::max<std::int64_t>(0,static_cast<std::int64_t>(original.counts[i])+item.count-current.counts[i]));
    if (item.category==Category::Jewelry || item.category==Category::Gem) {
        deficit=std::min<std::int64_t>(deficit,std::max(0,plan.maxValuables-current.valuables));
        if (currentValue>0) deficit=std::min(deficit,std::max<std::int64_t>(0,plan.valuablesBudget-current.valuablesValue)/currentValue);
    }
    return static_cast<int>(deficit);
}
}
