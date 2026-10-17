#pragma once
#include "Core.h"
#include <bit>
#include <cmath>
#include <optional>
#include <unordered_set>
#include <unordered_map>

namespace PL::Save {
inline constexpr std::uint32_t schema = 4;
inline constexpr std::uint32_t maxActors = 100000;
struct Data { std::uint64_t seed = 0; std::unordered_map<std::uint32_t,State> actors; };
struct Writer {
    std::vector<std::uint8_t> bytes;
    template<class T> void integer(T n) {
        for (std::size_t i = 0; i < sizeof(T); ++i) bytes.push_back(static_cast<std::uint8_t>(n >> (8*i)));
    }
    void real(double n) { integer(std::bit_cast<std::uint64_t>(n)); }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t offset = 0;
    bool valid = true;
    template<class T> T integer() {
        if (offset + sizeof(T) > bytes.size()) { valid = false; return 0; }
        T n = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i) n |= static_cast<T>(bytes[offset++]) << (8*i);
        return n;
    }
    double real() { return std::bit_cast<double>(integer<std::uint64_t>()); }
};
inline std::vector<std::uint8_t> encode(const Data& data) {
    Writer w;
    w.integer(data.seed);
    w.integer(static_cast<std::uint32_t>(data.actors.size()));
    // Sorted output makes save-format inspection and reproducible tests straightforward.
    std::vector<std::uint32_t> keys;
    for (auto& [id,state] : data.actors) keys.push_back(id);
    std::sort(keys.begin(),keys.end());
    for (auto id : keys) {
        auto& s = data.actors.at(id);
        w.integer(s.actor); w.integer(s.base); w.integer(s.random); w.integer(s.cycle);
        w.integer(static_cast<std::uint8_t>(s.generated));
        w.integer(static_cast<std::uint8_t>(s.role)); w.integer(static_cast<std::uint8_t>(s.wealth));
        w.real(s.pickpocketDue);
        w.integer(static_cast<std::uint8_t>(s.equipment.initialized));
        for (const auto& c:s.equipment.wear) {w.integer(c.source);w.integer(c.temporary);w.integer(c.extraBase);w.integer(c.extraUID);w.real(c.due);w.real(c.armorDue);}
        for (const auto& c:s.equipment.weapons) {w.integer(c.source);w.real(c.due);}
    }
    return w.bytes;
}
inline std::optional<Data> decode(std::uint32_t version, std::span<const std::uint8_t> bytes) {
    if (version < 1 || version > schema) return std::nullopt; // Unknown schemas fail closed, never reroll old actors.
    Reader r{bytes};
    Data data;
    data.seed = r.integer<std::uint64_t>();
    const auto count = r.integer<std::uint32_t>();
    if (!r.valid || count > maxActors) return std::nullopt;
    for (std::uint32_t i = 0; i < count; ++i) {
        State s;
        s.actor = r.integer<std::uint32_t>(); s.base = r.integer<std::uint32_t>();
        s.random = r.integer<std::uint64_t>(); s.cycle = r.integer<std::uint32_t>();
        auto generated = r.integer<std::uint8_t>();
        s.generated = generated != 0;
        // Schema 1 (v0.1.0) contained ammunition state. Consume and validate it
        // for migration only; schema 2 retains no ammunition data or behavior.
        std::uint8_t legacyInitialized = 0, legacyMaximum = 0;
        if (version == 1) { legacyInitialized = r.integer<std::uint8_t>(); legacyMaximum = r.integer<std::uint8_t>(); }
        auto role = r.integer<std::uint8_t>(), wealth = r.integer<std::uint8_t>();
        s.role = static_cast<Role>(role); s.wealth = static_cast<Wealth>(wealth);
        if (!r.valid || !s.actor || !s.base || generated > 1 || role > static_cast<unsigned>(Role::Prisoner) || wealth > static_cast<unsigned>(Wealth::Noble)) return std::nullopt;
        if (version>=3) {
            s.pickpocketDue=r.real();
            if (!r.valid || !std::isfinite(s.pickpocketDue) || (s.pickpocketDue<0 && s.pickpocketDue!=-1.0)) return std::nullopt;
        }
        if (version>=4) {
            const auto initialized=r.integer<std::uint8_t>();if (initialized>1) return std::nullopt;s.equipment.initialized=initialized!=0;
            auto validDue=[](double value) {return std::isfinite(value) && (value>=0 || value==-1);};
            for (auto& c:s.equipment.wear) {c.source=r.integer<std::uint32_t>();c.temporary=r.integer<std::uint32_t>();c.extraBase=r.integer<std::uint32_t>();c.extraUID=r.integer<std::uint16_t>();c.due=r.real();c.armorDue=r.real();if (!r.valid || !validDue(c.due) || !validDue(c.armorDue) || (!c.temporary && (c.extraBase || c.extraUID))) return std::nullopt;}
            for (auto& c:s.equipment.weapons) {c.source=r.integer<std::uint32_t>();c.due=r.real();if (!r.valid || !validDue(c.due) || (!c.source && c.due!=-1)) return std::nullopt;}
        }
        if (version == 1) {
            const auto legacyDue = r.real();
            const auto legacyEntries = r.integer<std::uint32_t>();
            if (!r.valid || legacyInitialized > 1 || !std::isfinite(legacyDue) || legacyDue < 0 || legacyEntries > 4096 || (legacyMaximum != 0 && (legacyMaximum < 24 || legacyMaximum > 60)) || (legacyInitialized && legacyMaximum == 0)) return std::nullopt;
            std::unordered_set<std::uint32_t> forms;
            for (std::uint32_t j = 0; j < legacyEntries; ++j) {
                auto form = r.integer<std::uint32_t>(), quantity = r.integer<std::uint32_t>();
                if (!r.valid || !form || !quantity || quantity > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) || !forms.insert(form).second) return std::nullopt;
            }
        }
        if (!data.actors.emplace(s.actor,std::move(s)).second) return std::nullopt;
    }
    if (!r.valid || r.offset != bytes.size()) return std::nullopt;
    return data;
}
}
