#pragma once
#include "VanillaData.h"
namespace PL::Vanilla {
// Separate enemy balance from occupation: town guards share Role::Guard with soldiers.
inline constexpr bool commonEnemySource(const Rule& rule) {
    if (rule.faction) return rule.form == 0x1BCC0 || rule.form == 0x43599 || rule.form == 0x26724;
    return rule.role == Role::Bandit ||
        ((rule.role == Role::Mage || rule.role == Role::Adventurer) && rule.priority >= 200 && rule.priority <= 210) ||
        rule.form == 0x1327F || rule.form == 0x13280 || rule.form == 0x10B1D8;
}
}
