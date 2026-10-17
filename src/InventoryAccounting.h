#pragma once
#include "Core.h"

namespace PL {
enum class ItemKind { Other, Armor, Misc, Food, Potion, Ingredient, Magic };
struct InventoryItemFacts {
    ItemKind kind = ItemKind::Other;
    Category knownCategory = Category::None;
    int count = 0;
    int baseValue = 0;
    std::int64_t jewelryTotalValue = -1; // Engine-valued mixed instances; -1 uses base value.
    std::uint32_t slots = 0;
    bool jewelryKeyword = false;
    bool gemKeyword = false;
    bool quest = false;
    bool gold = false;
};
inline Category pocketCategory(const InventoryItemFacts& item) {
    // Skyrim slots 35 (amulet) and 36 (ring). Outfits/armor occupying any other
    // slot never become valuables just because a mod assigned a jewelry keyword.
    constexpr std::uint32_t jewelrySlots = (1u << 5) | (1u << 6);
    if (item.kind == ItemKind::Armor) {
        const bool onlyJewelrySlots = (item.slots & jewelrySlots) != 0 && (item.slots & ~jewelrySlots) == 0;
        return onlyJewelrySlots && (item.jewelryKeyword || item.knownCategory == Category::Jewelry) ? Category::Jewelry : Category::None;
    }
    if (item.kind == ItemKind::Misc && (item.gemKeyword || item.knownCategory == Category::Gem)) return Category::Gem;
    // Never let an arbitrary weapon/book/junk item inherit a valuables category.
    if (item.knownCategory == Category::Jewelry || item.knownCategory == Category::Gem) return Category::None;
    if (item.knownCategory != Category::None) return item.knownCategory;
    switch (item.kind) {
    case ItemKind::Food: return Category::Food;
    case ItemKind::Potion: return Category::Potion;
    case ItemKind::Ingredient: return Category::Ingredient;
    case ItemKind::Magic: return Category::Magic;
    default: return Category::None;
    }
}
inline bool canGenerateAs(const InventoryItemFacts& item,Category category) {
    if (item.gold || item.quest) return false;
    switch (category) {
    case Category::Food: return item.kind==ItemKind::Food;
    case Category::Jewelry: return pocketCategory(item)==Category::Jewelry;
    case Category::Gem: case Category::Lockpick: return item.kind==ItemKind::Misc;
    case Category::Potion: return item.kind==ItemKind::Potion;
    case Category::Ingredient: return item.kind==ItemKind::Ingredient;
    case Category::Magic: return item.kind==ItemKind::Magic;
    default: return false;
    }
}
// Clamp represented extra-data stacks to the actual inventory count. A custom
// enchantment on one ring must not raise the value of every plain ring sharing it.
class JewelryValueAccumulator {
public:
    explicit JewelryValueAccumulator(int count) : remaining(std::max(0,count)) {}
    void add(int count,int unitValue) {
        const auto used=std::min(remaining,std::max(0,count));
        total=std::min<std::int64_t>(1000000000,total+static_cast<std::int64_t>(used)*std::max(0,unitValue));
        remaining-=used;
    }
    std::int64_t finish(int defaultUnitValue) {
        add(remaining,defaultUnitValue);
        return total;
    }
private:
    int remaining;
    std::int64_t total=0;
};
inline void accountInventoryItem(Inventory& result,const InventoryItemFacts& item) {
    if (item.count <= 0) return;
    if (item.gold) { result.gold = item.count; return; }
    if (item.quest) return;
    const auto c=pocketCategory(item);
    if (c == Category::None) return;
    result.counts[index(c)] = static_cast<int>(std::min<std::int64_t>(std::numeric_limits<int>::max(),static_cast<std::int64_t>(result.counts[index(c)]) + item.count));
    ++result.stacks;
    if (c == Category::Jewelry || c == Category::Gem) {
        result.valuables = static_cast<int>(std::min<std::int64_t>(std::numeric_limits<int>::max(),static_cast<std::int64_t>(result.valuables) + item.count));
        const auto value=c==Category::Jewelry && item.jewelryTotalValue>=0 ? item.jewelryTotalValue : static_cast<std::int64_t>(std::max(0,item.baseValue))*item.count;
        result.valuablesValue = std::min<std::int64_t>(1000000000,result.valuablesValue + value);
    }
}
}
