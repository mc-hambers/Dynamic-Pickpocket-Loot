#pragma once
#include "Core.h"
#include "RestoreData.h"
#include "RuntimeExtraData.h"
#include <new>
#include <functional>
#include <cmath>
#include <set>
#include <string>
namespace PL::Restore {
class Adapter {
    struct Binding {RE::TESBoundObject* object;unsigned kind,group;std::string editor;};
    std::vector<Binding> bindings;
    std::vector<std::pair<RE::BGSKeyword*,Material>> materials;
    RE::BGSKeyword* hammer=nullptr;
    RE::BGSKeyword* richClothing=nullptr;
    RE::TESFaction* streetVendors=nullptr;
    // Replacement prices use record values, without enchantment/tempering or
    // inventory valuation callbacks on third-party equipment.
    static int recordValue(RE::TESBoundObject* item) {
        const auto* value=skyrim_cast<const RE::TESValueForm*>(item);
        return value ? std::max(0,value->value) : 0;
    }
    void equipNow(RE::Actor* actor,RE::TESBoundObject* item) const {
        // The inventory owns instance extra data after AddObjectToContainer. Never reuse the
        // supplied raw pointer here, resolve the current engine-owned inventory data instead.
        SKSE::log::info("Immediate equipment begin {:08X} item {:08X}",actor->GetFormID(),item->GetFormID());
        auto* manager=RE::ActorEquipManager::GetSingleton();
        if (!manager) {SKSE::log::error("Equipment manager unavailable; leaving inventory item intact");return;}
        manager->EquipObject(actor,item,nullptr,1,nullptr,false,true,false,true);
        SKSE::log::info("Immediate equipment returned {:08X} item {:08X}",actor->GetFormID(),item->GetFormID());
    }
    bool usable(RE::TESBoundObject* item) const {return item && !item->IsDeleted() && item->GetPlayable();}
    unsigned mask(RE::TESObjectARMO* a) const {return a->GetSlotMask().underlying();}
    int weaponKind(RE::TESObjectWEAP* w) const {
        if (!usable(w) || w->IsBound()) return -1;
        switch (unsigned(w->GetWeaponType())) {
        case 1:return 0;case 2:return 1;case 3:return 2;case 4:return 3;case 5:return 4;
        case 6:return hammer && w->HasKeyword(hammer) ? 6 : 5;
        case 7:return 7;case 9:return 8;default:return -1;
        }
    }
    Material material(RE::TESObjectWEAP* w) const {for (const auto& [keyword,m]:materials) if (w->HasKeyword(keyword)) return m;return Material::Other;}
    bool elf(RE::Actor* actor) const {const auto* race=actor->GetRace();if (!race) return false;const auto id=race->GetFormID();return id==0x13742 || id==0x13743 || id==0x13749 || id==0x8883D || id==0x88840 || id==0x88884;}
    void collect(RE::TESForm* f,unsigned level,std::vector<RE::TESBoundObject*>& out,std::set<unsigned>& path,unsigned depth,unsigned& nodes) const {
        if (!f || depth>16 || ++nodes>4096 || f->IsDeleted()) return;
        if (auto* list=f->As<RE::TESLevItem>()) {
            if (!path.insert(f->GetFormID()).second || list->GetChanceNone()>=100 || !list->entries.data()) return;
            unsigned best=0;for (unsigned i=0;i<std::min<std::size_t>(list->numEntries,list->entries.size());++i) {const auto& e=list->entries[i];if (e.form && e.count && e.level<=level) best=std::max(best,unsigned(e.level));}
            bool all=(unsigned(list->llFlags)&(RE::TESLeveledList::kCalculateFromAllLevelsLTOrEqPCLevel|RE::TESLeveledList::kUseAll))!=0;
            for (unsigned i=0;i<std::min<std::size_t>(list->numEntries,list->entries.size());++i) {const auto& e=list->entries[i];if (e.form && e.count && e.level<=level && (all || e.level==best)) collect(e.form,level,out,path,depth+1,nodes);}
            path.erase(f->GetFormID());
        } else if (auto* o=f->As<RE::TESBoundObject>();usable(o)) out.push_back(o);
    }
    void collectNative(RE::TESForm* form,unsigned level,const std::set<unsigned>& carried,std::vector<RE::TESBoundObject*>& out,std::set<unsigned>& path,unsigned depth,unsigned& nodes) const {
        if (!form || depth>16 || ++nodes>4096 || form->IsDeleted()) return;
        auto* list=form->As<RE::TESLevItem>();
        if (!list) {if (auto* item=form->As<RE::TESBoundObject>();usable(item)) out.push_back(item);return;}
        if (!path.insert(form->GetFormID()).second) return;
        if (list->GetChanceNone()>=100 || !list->entries.data()) {path.erase(form->GetFormID());return;}
        unsigned highest=0;const auto count=std::min<std::size_t>(list->numEntries,list->entries.size());
        for (unsigned i=0;i<count;++i) {const auto& e=list->entries[i];if (e.form && e.count && e.level<=level) highest=std::max(highest,unsigned(e.level));}
        bool all=(unsigned(list->llFlags)&RE::TESLeveledList::kUseAll)!=0;
        bool allLevels=(unsigned(list->llFlags)&RE::TESLeveledList::kCalculateFromAllLevelsLTOrEqPCLevel)!=0 || all;
        std::vector<std::vector<RE::TESBoundObject*>> options;std::vector<NativeBranch> facts;
        for (unsigned i=0;i<count;++i) {
            const auto& e=list->entries[i];if (!e.form || !e.count || e.level>level || (!allLevels && e.level!=highest)) continue;
            std::vector<RE::TESBoundObject*> branch;collectNative(e.form,level,carried,branch,path,depth+1,nodes);if (branch.empty()) continue;
            bool present=std::any_of(branch.begin(),branch.end(),[&](auto* o) {return carried.contains(o->GetFormID());});
            options.push_back(std::move(branch));facts.push_back({present,0});
        }
        if (nativePricesNeeded(facts,all,list->GetChanceNone()==0))
            for (std::size_t i=0;i<options.size();++i) {
                const auto& branch=options[i];
                facts[i].value=recordValue(*std::min_element(branch.begin(),branch.end(),[](auto* a,auto* b) {return recordValue(a)<recordValue(b);}));
            }
        for (auto i:nativeBranches(facts,all,list->GetChanceNone()==0)) out.insert(out.end(),options[i].begin(),options[i].end());
        path.erase(form->GetFormID());
    }
    bool held(RE::Actor* actor,unsigned kind) const {
        const auto inv=actor->GetInventory();for (const auto& [o,e]:inv) if (o && e.first>0) if (auto* w=o->As<RE::TESObjectWEAP>();w && weaponKind(w)==int(kind)) return true;return false;
    }
    unsigned worn(RE::Actor* actor) const {
        unsigned result=0;auto inv=actor->GetInventory();for (const auto& [o,e]:inv) if (o && e.first>0 && e.second && e.second->IsWorn()) if (auto* a=o->As<RE::TESObjectARMO>()) result|=mask(a);return result;
    }
    RE::ExtraDataList* tagged(RE::Actor* actor,const Wear& c) const {
        if (!c.temporary) return nullptr;auto inv=actor->GetInventory([&](RE::TESBoundObject& o) {return o.GetFormID()==c.temporary;});
        for (auto& [o,e]:inv) if (e.first>0 && e.second && e.second->extraLists) for (auto* x:*e.second->extraLists) if (x) {auto* id=x->GetByType<RE::ExtraUniqueID>();if (id && id->baseID==c.extraBase && id->uniqueID==c.extraUID) return x;}return nullptr;
    }
    bool removeTemporary(RE::Actor* actor,Wear& c) const {
        auto* o=RE::TESForm::LookupByID<RE::TESBoundObject>(c.temporary);auto* x=tagged(actor,c);
        if (o && x && (x->HasType<RE::ExtraWorn>() || x->HasType<RE::ExtraWornLeft>())) {
            auto* manager=RE::ActorEquipManager::GetSingleton();if (!manager) return false;
            manager->UnequipObject(actor,o,x,1,nullptr,false,true,false,true);
            // Unequipping can replace instance data. Reacquire before removal.
            x=tagged(actor,c);
            if (x && (x->HasType<RE::ExtraWorn>() || x->HasType<RE::ExtraWornLeft>())) return false;
        }
        c.temporary=0;c.extraBase=0;c.extraUID=0;
        if (o && x) {actor->RemoveItem(o,1,RE::ITEM_REMOVE_REASON::kRemove,x,nullptr);}
        return true;
    }
    bool equipExisting(RE::Actor* actor,unsigned slot,bool clothingOnly) const {
        auto inv=actor->GetInventory();auto occupied=worn(actor);RE::TESObjectARMO* best=nullptr;
        for (auto& [o,e]:inv) {
            auto* a=o ? o->As<RE::TESObjectARMO>() : nullptr;
            if (!a || !usable(a) || e.first<=0 || (clothingOnly && !a->IsClothing()) || !(mask(a)&slot) || (mask(a)&occupied)) continue;
            if (!best || (!a->IsClothing() && best->IsClothing()) || (a->IsClothing()==best->IsClothing() && recordValue(a)<recordValue(best))) {
                best=a;
            }
        }
        if (!best) return false;equipNow(actor,best);return true;
    }
    bool ownsWear(RE::Actor* actor,unsigned slot) const {auto inv=actor->GetInventory();for (const auto& [o,e]:inv) if (o && e.first>0) if (auto* a=o->As<RE::TESObjectARMO>();a && usable(a) && (mask(a)&slot)) return true;return false;}
    bool equipOwnedArmor(RE::Actor* actor,RE::TESObjectARMO* source,unsigned slot) const {
        auto inv=actor->GetInventory();unsigned protectedMask=0;
        for (const auto& [o,e]:inv) if (o && e.first>0 && e.second && e.second->IsWorn()) if (auto* a=o->As<RE::TESObjectARMO>();a && !a->IsClothing()) protectedMask|=mask(a);
        RE::TESObjectARMO* best=nullptr;
        for (const auto& [o,e]:inv) if (o && e.first>0) if (auto* a=o->As<RE::TESObjectARMO>();a && !a->IsClothing() && usable(a) && a->GetArmorType()==source->GetArmorType() && (mask(a)&slot) && !(mask(a)&protectedMask)) {
            if (!best || a->armorRating>best->armorRating) best=a;
        }
        if (!best) return false;equipNow(actor,best);return true;
    }
    bool stallMerchant(RE::Actor* actor) const {return streetVendors && actor->IsInFaction(streetVendors);}
    RE::TESObjectARMO* nativeCheapClothes(RE::Actor* actor,unsigned slot,unsigned occupied) const {
        std::vector<RE::TESBoundObject*> candidates;std::set<unsigned> path;unsigned nodes=0;
        auto* base=actor->GetActorBase();
        base->ForEachContainerObject([&](RE::ContainerObject& entry) {if (entry.count>0) collect(entry.obj,actor->GetLevel(),candidates,path,0,nodes);return RE::BSContainer::ForEachResult::kContinue;});
        if (base->defaultOutfit) base->defaultOutfit->ForEachItem([&](RE::TESForm* form) {collect(form,actor->GetLevel(),candidates,path,0,nodes);return RE::BSContainer::ForEachResult::kContinue;});
        RE::TESObjectARMO* best=nullptr;
        for (auto* object:candidates) if (auto* a=object->As<RE::TESObjectARMO>();a && usable(a) && a->IsClothing() && !a->formEnchanting && (mask(a)&slot) && !(mask(a)&occupied) && !(mask(a)&((slots[0]|slots[1]|slots[2]|slots[3])&~slot))) {
            if (!cheapNativeClothing(recordValue(a),slot==slots[1],richClothing && a->HasKeyword(richClothing))) continue;
            if (!best || recordValue(a)<recordValue(best) || (recordValue(a)==recordValue(best) && a->GetFormID()<best->GetFormID())) best=a;
        }
        return best;
    }
    RE::TESObjectARMO* clothes(RE::Actor* actor,unsigned slot,Role role,Wealth wealth,unsigned occupied,RE::TESObjectARMO* source) const {
        auto simpleSlot=[&](RE::TESObjectARMO* a) {return !(mask(a)&((slots[0]|slots[1]|slots[2]|slots[3])&~slot));};
        if (role==Role::Child) return source && source->IsClothing() && !source->formEnchanting && !(mask(source)&occupied) && simpleSlot(source) ? source : nullptr;
        const bool stall=stallMerchant(actor);
        if (auto* native=nativeCheapClothes(actor,slot,occupied)) return native;
        unsigned group=stall ? 1 : wealth<=Wealth::Poor || role==Role::Beggar || role==Role::Prisoner ? 0 : role==Role::Mage || role==Role::Priest ? 3 : wealth>=Wealth::Wealthy || role==Role::Noble ? 2 : role==Role::Smith ? 4 : role==Role::Innkeeper ? 5 : role==Role::Merchant ? 6 : 1;
        for (unsigned pass=0;pass<2;++pass) {
            RE::TESObjectARMO* best=nullptr;
            for (const auto& b:bindings) if (b.kind==0 && b.group==(pass==0 ? group : 1)) if (auto* a=b.object->As<RE::TESObjectARMO>();a && (mask(a)&slot) && !(mask(a)&occupied) && !a->formEnchanting && simpleSlot(a)) {
                if (!best || mask(a)<mask(best) || (mask(a)==mask(best) && recordValue(a)<recordValue(best))) best=a;
            }
            if (best) return best;
        }return nullptr;
    }
    bool guardUniform(RE::TESObjectARMO* source,bool guard) const {
        bool recognized=std::any_of(bindings.begin(),bindings.end(),[&](const auto& b) {return b.kind==3 && b.object==source;});
        return guardUniformAllowed(guard,recognized,source->IsClothing(),source->formEnchanting!=nullptr);
    }
    RE::TESObjectARMO* armor(RE::Actor* actor,RE::TESObjectARMO* original,unsigned slot,unsigned occupied,Wealth wealth,bool guard) const {
        if (guardUniform(original,guard) && !(mask(original)&occupied)) return original;
        RE::TESObjectARMO* best=nullptr;const bool elven=elf(actor) && wealth>=Wealth::Comfortable;
        for (const auto& b:bindings) if (b.kind==1) if (auto* a=b.object->As<RE::TESObjectARMO>();a && !a->formEnchanting && a->GetArmorType()==original->GetArmorType() && (mask(a)&slot) && !(mask(a)&occupied) && recordValue(a)<=recordValue(original) && a->armorRating<=original->armorRating) {
            bool preferred=a->IsLightArmor() ? b.editor.find(elven ? "Elven" : "Leather")!=std::string::npos : b.editor.find(wealth<=Wealth::Poor ? "Iron" : "Steel")!=std::string::npos;
            auto rank=[&](RE::TESObjectARMO* candidate) {for (const auto& entry:bindings) if (entry.object==candidate) return entry.editor.find(candidate->IsLightArmor() ? (elven ? "Elven" : "Leather") : (wealth<=Wealth::Poor ? "Iron" : "Steel"))!=std::string::npos;return false;};
            if (!best || (preferred && !rank(best)) || (preferred==rank(best) && a->armorRating>best->armorRating)) best=a;
        }return best;
    }
    RE::TESObjectWEAP* weapon(RE::Actor* actor,RE::TESObjectWEAP* original,unsigned kind,Wealth wealth) const {
        // Crossbow overhauls may make the vanilla Dawnguard fallback stronger
        // than a common steel crossbow. Consider plain low-tier crossbows from
        // this NPC's own native lists as well, never arbitrary global weapons.
        std::vector<RE::TESBoundObject*> native;
        if (kind==8) {
            std::set<unsigned> path;unsigned nodes=0;auto* base=actor->GetActorBase();
            if (base) {
                base->ForEachContainerObject([&](RE::ContainerObject& e) {if (e.count>0) collect(e.obj,actor->GetLevel(),native,path,0,nodes);return RE::BSContainer::ForEachResult::kContinue;});
                if (base->defaultOutfit) base->defaultOutfit->ForEachItem([&](RE::TESForm* f) {collect(f,actor->GetLevel(),native,path,0,nodes);return RE::BSContainer::ForEachResult::kContinue;});
            }
        }
        auto target=replacementMaterial(material(original),elf(actor),wealth>=Wealth::Wealthy);
        unsigned group=target==Material::Elven ? 1 : target==Material::Iron ? 2 : 0;
        for (unsigned pass=0;pass<3;++pass) {
            RE::TESObjectWEAP* best=nullptr;const unsigned wanted=pass==0 ? group : pass==1 ? 0 : 2;
            for (const auto& b:bindings) if (b.kind==2 && b.group==wanted) if (auto* w=b.object->As<RE::TESObjectWEAP>();w && weaponKind(w)==int(kind) && !w->formEnchanting && plainReplacementAllowed(recordValue(original),original->attackDamage,recordValue(w),w->attackDamage)) if (!best || w->attackDamage>best->attackDamage || (w->attackDamage==best->attackDamage && recordValue(w)<recordValue(best))) best=w;
            for (auto* object:native) if (auto* w=object->As<RE::TESObjectWEAP>();w && weaponKind(w)==int(kind) && !w->formEnchanting) {
                const auto mat=material(w);
                if (nativeCrossbowAllowed(mat,wanted,w->formEnchanting!=nullptr,recordValue(original),original->attackDamage,recordValue(w),w->attackDamage))
                    if (!best || w->attackDamage>best->attackDamage || (w->attackDamage==best->attackDamage && recordValue(w)<recordValue(best))) best=w;
            }
            if (best) return best;
        }return nullptr;
    }
public:
    void initialize() {
        auto* h=RE::TESDataHandler::GetSingleton();
        streetVendors=h->LookupForm<RE::TESFaction>(0x7F6BE,"Skyrim.esm");
        for (const auto& spec:forms) if (auto* f=h->LookupForm(spec.form,spec.plugin)) if (auto* o=f->As<RE::TESBoundObject>();usable(o)) bindings.push_back({o,spec.kind,spec.group,spec.editor});
        hammer=h->LookupForm<RE::BGSKeyword>(0x6D930,"Skyrim.esm");
        richClothing=h->LookupForm<RE::BGSKeyword>(0xA865D,"Skyrim.esm");
        // Resolve stable record identities; duplicate/missing editor IDs in other
        // plugins must not replace vanilla material keys or require global scans.
        for (const auto& [id,mat]:std::initializer_list<std::pair<unsigned,Material>>{
            {0x1E718,Material::Iron},{0x1E719,Material::Steel},{0x1E71A,Material::Dwarven},
            {0x1E71B,Material::Elven},{0x1E71D,Material::Glass},{0x1E71E,Material::Ebony},{0x1E71F,Material::Daedric}})
            if (auto* key=h->LookupForm<RE::BGSKeyword>(id,"Skyrim.esm")) materials.emplace_back(key,mat);
        if (auto* key=h->LookupForm<RE::BGSKeyword>(0x19822,"Dawnguard.esm")) materials.emplace_back(key,Material::Dragonbone);
        SKSE::log::info("Equipment restoration loaded {} vanilla bindings, {} weapon materials; ExtraDataList facade={} bytes, allocation={} bytes",bindings.size(),materials.size(),sizeof(RE::ExtraDataList),extraDataCapacity(sizeof(RE::ExtraDataList)));
    }
    void baseline(RE::Actor* actor,State& s) const {
        auto* base=actor->GetActorBase();auto live=actor->GetInventory();std::vector<RE::TESBoundObject*> defaults;std::set<unsigned> path;unsigned nodes=0;
        std::array<unsigned,4> wearSources{};std::array<unsigned,weaponKinds> weaponSources{};
        std::set<unsigned> carried;for (const auto& [o,e]:live) if (o && e.first>0) carried.insert(o->GetFormID());
        base->ForEachContainerObject([&](RE::ContainerObject& e) {
            if (e.count>0) collectNative(e.obj,actor->GetLevel(),carried,defaults,path,0,nodes);
            return RE::BSContainer::ForEachResult::kContinue;
        });
        if (base->defaultOutfit) base->defaultOutfit->ForEachItem([&](RE::TESForm* f) {collectNative(f,actor->GetLevel(),carried,defaults,path,0,nodes);return RE::BSContainer::ForEachResult::kContinue;});
        for (auto* o:defaults) {
            if (auto* w=o->As<RE::TESObjectWEAP>()) {auto kind=weaponKind(w);if (kind>=0 && !weaponSources[kind]) weaponSources[kind]=w->GetFormID();}
            if (auto* a=o->As<RE::TESObjectARMO>()) for (unsigned i=0;i<4;++i) if (mask(a)&slots[i]) {
                bool present=live.contains(a) && live.at(a).first>0;
                if (!wearSources[i] || present) wearSources[i]=a->GetFormID();
            }
        }
        for (unsigned i=0;i<4;++i) {s.wear[i].source=wearSources[i];if (!wearSources[i]) {s.wear[i].due=-1;s.wear[i].armorDue=-1;}}
        for (unsigned i=0;i<weaponKinds;++i) {s.weapons[i].source=weaponSources[i];if (!weaponSources[i]) s.weapons[i].due=-1;}
        s.initialized=true;
    }
    void run(RE::Actor* actor,PL::State& saved,double now,std::function<bool()> allowed) const {
        if (!std::isfinite(now) || actor->GetCurrentScene() || actor->IsInCombat() || !allowed()) return;
        auto& s=saved.equipment;if (!s.initialized) baseline(actor,s);
        const bool poor=saved.wealth<=Wealth::Poor;
        const bool guard=saved.role==Role::Guard || actor->IsGuard();
        applyClothingPolicy(s,poor,saved.role==Role::Beggar);
        const bool weapons=newWeaponsAllowed(poor,saved.role==Role::Beggar,saved.role==Role::Child || saved.role==Role::Prisoner);
        // Healthy, fully equipped actors need only one inventory snapshot per periodic check.
        auto snapshot=actor->GetInventory();unsigned wornMask=0,ownedMask=0,clothingMask=0,weaponMask=0;
        for (const auto& [o,e]:snapshot) if (o && e.first>0) {
            if (auto* a=o->As<RE::TESObjectARMO>()) {ownedMask|=mask(a);if (e.second && e.second->IsWorn()) {wornMask|=mask(a);if (a->IsClothing()) clothingMask|=mask(a);}}
            if (auto* w=o->As<RE::TESObjectWEAP>()) if (auto kind=weaponKind(w);kind>=0) weaponMask|=1u<<kind;
        }
        // Observe missing equipment and service saved deadlines during normal loaded checks.
        for (unsigned i=0;i<4;++i) if (s.wear[i].source) {
            auto& c=s.wear[i];bool empty=!(wornMask&slots[i]);
            auto* source=RE::TESForm::LookupByID<RE::TESObjectARMO>(c.source);
            if (!source) continue;
            const bool clothingSlot=enabledSlot(i,poor,saved.role==Role::Beggar);
            const bool armorEnabled=armorSlot(i,!source->IsClothing());
            if (!clothingSlot && !armorEnabled) {c.armorDue=-1;continue;}
            const bool uniform=guardUniform(source,guard);
            if (!clothingSlot && armorEnabled && (empty || (clothingMask&slots[i]))) armOnce(c.armorDue,now,equipmentArmorDays(recordValue(source),guard));
            if (guard && source && !source->IsClothing()) {
                if (empty || (clothingMask&slots[i])) {bool started=c.armorDue<0;armOnce(c.armorDue,now,equipmentArmorDays(recordValue(source),true));if (started) SKSE::log::info("Guard armor missing on {:08X}, slot {}: twelve-hour replacement due {}",actor->GetFormID(),i,c.armorDue);}
                shortenGuardDeadline(c.armorDue,now);
            }
            if (uniform) c.due=-1;
            if (clothingSlot && !uniform && empty && !(ownedMask&slots[i]) && !c.temporary) {bool started=c.due<0;armOnce(c.due,now,clothingDays(poor || saved.role==Role::Beggar));if (started) SKSE::log::info("Clothing missing on {:08X}, slot {}: due {}",actor->GetFormID(),i,c.due);}
            if (!empty) c.due=-1;
            if ((clothingMask&slots[i]) && c.armorDue<0 && source && !source->IsClothing()) c.armorDue=now+equipmentArmorDays(recordValue(source),guard);
        }
        for (unsigned kind=0;kind<weaponKinds;++kind) if (s.weapons[kind].source) {auto& c=s.weapons[kind];if (!weapons || (weaponMask&(1u<<kind))) c.due=-1;else {bool started=c.due<0;armOnce(c.due,now,1);if (started) SKSE::log::info("Weapon missing on {:08X}, kind {}: due {}",actor->GetFormID(),kind,c.due);}}
        bool work=false;
        for (unsigned i=0;i<4;++i) if (s.wear[i].source) {
            const auto& c=s.wear[i];bool empty=!(wornMask&slots[i]);
            if ((empty && ((ownedMask&slots[i]) || c.due<0 || dueNow(c.due,now))) || (!empty && c.due>=0) || dueNow(c.armorDue,now)) work=true;
            if ((clothingMask&slots[i]) && c.armorDue<0) if (auto* source=RE::TESForm::LookupByID<RE::TESObjectARMO>(c.source);source && !source->IsClothing()) work=true;
        }
        for (unsigned kind=0;kind<weaponKinds;++kind) if (s.weapons[kind].source) {const auto& c=s.weapons[kind];bool present=(weaponMask&(1u<<kind))!=0;if ((!weapons || present) ? c.due>=0 : c.due<0 || dueNow(c.due,now)) work=true;}
        if (!work) return;
        for (unsigned i=0;i<4 && allowed();++i) {
            auto& c=s.wear[i];auto* source=RE::TESForm::LookupByID<RE::TESObjectARMO>(c.source);if (!source) continue;
            const bool clothingSlot=enabledSlot(i,poor,saved.role==Role::Beggar);
            if (!clothingSlot && !armorSlot(i,!source->IsClothing())) continue;
            if (c.temporary && !tagged(actor,c)) {c.temporary=0;c.extraBase=0;c.extraUID=0;}
            auto occupied=worn(actor);
            if (!(occupied&slots[i])) {
                if (clothingSlot ? (equipExisting(actor,slots[i],true) || equipExisting(actor,slots[i],false)) : equipOwnedArmor(actor,source,slots[i])) {c.due=-1;continue;}
                else if (clothingSlot && !guardUniform(source,guard) && !ownsWear(actor,slots[i]) && !c.temporary) {armOnce(c.due,now,clothingDays(poor || saved.role==Role::Beggar));
                    if (dueNow(c.due,now)) if (auto* item=clothes(actor,slots[i],saved.role,saved.wealth,occupied,source)) {
                        if (!allowed() || (worn(actor)&mask(item))) continue;
                        auto* changes=actor->GetInventoryChanges();if (!changes) continue;
                        const auto uid=changes->GetNextUniqueID();
                        SKSE::log::info("Temporary clothing allocation begin {:08X} item {:08X}: {} bytes",actor->GetFormID(),item->GetFormID(),extraDataCapacity(sizeof(RE::ExtraDataList)));
                        auto* extra=allocateRuntimeExtraData<RE::ExtraDataList>([](std::size_t bytes) {return RE::malloc(bytes);},[](void* storage) {::new(storage) RE::ExtraDataList();});
                        if (!extra) {SKSE::log::error("Temporary clothing allocation failed; restoration remains pending");continue;}
                        extra->Add(new RE::ExtraUniqueID(actor->GetFormID(),uid));
                        c.temporary=item->GetFormID();c.extraBase=actor->GetFormID();c.extraUID=uid;c.due=-1;
                        if (!source->IsClothing() && c.armorDue<0) c.armorDue=now+equipmentArmorDays(recordValue(source),guard);
                        SKSE::log::info("Temporary clothing inventory add begin {:08X} item {:08X}",actor->GetFormID(),item->GetFormID());
                        actor->AddObjectToContainer(item,extra,1,nullptr);
                        SKSE::log::info("Temporary clothing inventory add returned {:08X} item {:08X}",actor->GetFormID(),item->GetFormID());if (!allowed()) return;
                        equipNow(actor,item);
                        SKSE::log::info("Equipped clothing {:08X} on {:08X}",item->GetFormID(),actor->GetFormID());
                        continue;
                    }
                }
            } else c.due=-1;
            if (!allowed()) return;
            // Wearing carried clothes also starts the armor waiting period, without tagging or deleting those clothes.
            if (!source->IsClothing() && c.armorDue<0) {
                auto inv=actor->GetInventory();for (const auto& [o,e]:inv) if (o && e.first>0 && e.second && e.second->IsWorn()) if (auto* a=o->As<RE::TESObjectARMO>();a && a->IsClothing() && (mask(a)&slots[i])) c.armorDue=now+equipmentArmorDays(recordValue(source),guard);
            }
            if (dueNow(c.armorDue,now)) {
                auto inv=actor->GetInventory();bool armored=false;unsigned overlappingClothes=0;
                for (const auto& [o,e]:inv) if (o && e.first>0 && e.second && e.second->IsWorn()) if (auto* a=o->As<RE::TESObjectARMO>();a && (mask(a)&slots[i])) {if (!a->IsClothing()) armored=true;else overlappingClothes|=mask(a);}
                if (armored) {if (removeTemporary(actor,c)) c.armorDue=-1;continue;}
                if (i!=0 && (overlappingClothes&slots[0])) continue; // Do not strip a hooded robe for a helmet/glove swap.
                if (equipOwnedArmor(actor,source,slots[i])) {if (removeTemporary(actor,c)) c.armorDue=-1;continue;}
                if (auto* item=armor(actor,source,slots[i],worn(actor)&~overlappingClothes,saved.wealth,guard)) {
                    if (!allowed()) return;
                    bool has=false;auto current=actor->GetInventory();if (auto it=current.find(item);it!=current.end()) has=it->second.first>0;
                    if (!has) actor->AddObjectToContainer(item,nullptr,1,nullptr);if (!allowed()) return;
                    equipNow(actor,item);
                    SKSE::log::info("Equipped armor {:08X} on {:08X}",item->GetFormID(),actor->GetFormID());
                    if (removeTemporary(actor,c)) c.armorDue=-1;
                    continue;
                }
            }
        }
        for (unsigned kind=0;kind<weaponKinds && allowed();++kind) {
            auto& c=s.weapons[kind];auto* source=RE::TESForm::LookupByID<RE::TESObjectWEAP>(c.source);
            if (!weapons || !source || held(actor,kind)) {c.due=-1;continue;}
            armOnce(c.due,now,1);
            if (dueNow(c.due,now)) if (auto* item=weapon(actor,source,kind,saved.wealth)) {
                if (!allowed() || held(actor,kind)) continue;c.due=-1;actor->AddObjectToContainer(item,nullptr,1,nullptr);
                if (!allowed()) return;equipNow(actor,item);
                SKSE::log::info("Equipped weapon {:08X} instead of {:08X} on {:08X}",item->GetFormID(),source->GetFormID(),actor->GetFormID());
                continue;
            }
        }
    }
};
}
