#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
namespace PL::Restore {
inline constexpr std::array<std::uint32_t,4> slots{1u<<2,1u<<7,1u<<3,(1u<<0)|(1u<<1)}; // body, feet, hands, head/hair
inline constexpr unsigned weaponKinds=9;
// A pending slot remembers where undressing was observed. Either actor must
// leave and return to that cell; merely entering a different cell is insufficient.
struct CellReequip {
    std::uint32_t pending=0,ready=0;
    std::array<std::uint32_t,4> origin{};
    std::array<bool,4> npcLeft{},playerLeft{};
    void observe(std::uint32_t current,std::uint32_t emptyOwned) {
        if (!current) return;
        for (unsigned i=0;i<slots.size();++i) {
            const auto slot=slots[i];
            if (!(emptyOwned&slot)) {pending&=~slot;ready&=~slot;origin[i]=0;npcLeft[i]=playerLeft[i]=false;continue;}
            if (!(pending&slot)) {origin[i]=current;pending|=slot;}
            else if (current!=origin[i]) npcLeft[i]=true;
            else if (npcLeft[i]) ready|=slot;
        }
    }
    bool canEquip(unsigned index,std::uint32_t current) const {return index<slots.size() && current && origin[index]==current && (ready&slots[index]);}
    void playerTravel(std::uint32_t cell,bool entering) {
        if (!cell) return;
        for (unsigned i=0;i<slots.size();++i) if ((pending&slots[i]) && origin[i]==cell) {
            if (!entering) playerLeft[i]=true;
            else if (playerLeft[i]) ready|=slots[i];
        }
    }
};
enum class Material : std::uint8_t { Other,Iron,Steel,Elven,Glass,Ebony,Daedric,Dragonbone,Dwarven };
struct Wear {
    std::uint32_t source=0,temporary=0,extraBase=0;
    std::uint16_t extraUID=0;
    double due=-1,armorDue=-1;
};
struct Weapon {std::uint32_t source=0;double due=-1;};
struct State {bool initialized=false;std::array<Wear,4> wear{};std::array<Weapon,weaponKinds> weapons{};};
inline bool cheapNativeClothing(int value,bool feet,bool rich) {return !rich && value>=0 && value<=(feet ? 10 : 25);}
inline double clothingDays(bool poor) {return poor ? 2.0 : 1.0;}
inline bool enabledSlot(unsigned slot,bool poor,bool beggar) {return slot==0 || (slot==1 && !poor && !beggar);}
inline bool armorSlot(unsigned slot,bool nativeArmor) {return nativeArmor && slot<slots.size();}
inline void applyClothingPolicy(State& state,bool poor,bool beggar) {
    for (unsigned i=0;i<state.wear.size();++i) if (!enabledSlot(i,poor,beggar)) state.wear[i].due=-1;
}
struct NativeBranch {bool present;int value;};
inline bool nativePricesNeeded(const std::vector<NativeBranch>& branches,bool useAll,bool guaranteed) {
    return guaranteed && !useAll && branches.size()>1 &&
        std::none_of(branches.begin(),branches.end(),[](const auto& b) {return b.present;});
}
inline std::vector<unsigned> nativeBranches(const std::vector<NativeBranch>& branches,bool useAll,bool guaranteed) {
    std::vector<unsigned> chosen;
    const bool bundle=useAll && (guaranteed || std::any_of(branches.begin(),branches.end(),[](const auto& b) {return b.present;}));
    for (unsigned i=0;i<branches.size();++i) if (bundle || branches[i].present) chosen.push_back(i);
    if (chosen.empty() && guaranteed && !branches.empty()) {
        auto best=std::min_element(branches.begin(),branches.end(),[](const auto& a,const auto& b) {return a.value<b.value;});
        chosen.push_back(static_cast<unsigned>(best-branches.begin()));
    }
    return chosen;
}
inline double armorDays(int sourceValue) {return 2.0+std::clamp(sourceValue,0,1000)/1000.0;}
inline double equipmentArmorDays(int sourceValue,bool guard) {return guard ? 0.5 : armorDays(sourceValue);}
inline bool guardUniformAllowed(bool guard,bool recognized,bool clothing,bool enchanted) {return guard && recognized && !clothing && !enchanted;}
inline void shortenGuardDeadline(double& due,double now) {if (due>=0) due=std::min(due,now+0.5);}
inline bool newWeaponsAllowed(bool poor,bool beggar,bool childOrPrisoner) {return !poor && !beggar && !childOrPrisoner;}
inline Material replacementMaterial(Material original,bool elvenRace,bool wealthy) {
    switch (original) {
    case Material::Glass:return Material::Elven;
    case Material::Ebony:return elvenRace ? Material::Elven : Material::Steel;
    case Material::Daedric:case Material::Dragonbone:case Material::Dwarven:return Material::Steel;
    case Material::Elven:return elvenRace || wealthy ? Material::Elven : Material::Steel;
    case Material::Iron:return Material::Iron;
    default:return Material::Steel;
    }
}
inline void armOnce(double& due,double now,double days) {if (due<0) due=now+days;}
inline bool dueNow(double due,double now) {return due>=0 && now>=due;}
inline bool plainReplacementAllowed(int originalValue,unsigned originalDamage,int value,unsigned damage) {return value<=originalValue && damage<=originalDamage;}
inline bool nativeCrossbowAllowed(Material material,unsigned group,bool enchanted,int sourceValue,unsigned sourceDamage,int value,unsigned damage) {
    return !enchanted && ((group==0 && material==Material::Steel) || (group==2 && material==Material::Iron)) &&
        plainReplacementAllowed(sourceValue,sourceDamage,value,damage);
}
inline bool rebindTemporary(Wear& c,unsigned form,unsigned oldBase,unsigned oldID,unsigned newBase,unsigned newID) {
    if (!c.temporary || c.temporary!=form || c.extraBase!=oldBase || c.extraUID!=oldID || newID>65535) return false;
    c.extraBase=newBase;c.extraUID=static_cast<std::uint16_t>(newID);return true;
}
}
