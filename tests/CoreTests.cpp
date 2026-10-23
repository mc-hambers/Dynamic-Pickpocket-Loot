#include "Core.h"
#include "Serialization.h"
#include "InventoryAccounting.h"
#include "FormResolution.h"
#include "DistributionLifecycle.h"
#include "LootConfig.h"
#include "VictimTimer.h"
#include "RuntimeExtraData.h"
#include "RestoreData.h"
#include "VanillaData.h"
#include "EnemyBalance.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int checks=0;
void check(bool condition,const char* text) { ++checks; if (!condition) throw std::runtime_error(text); }
// Reproduce the exact-type LookupForm contract from the linked CommonLib headers.
struct MockBoundObject { static constexpr int FORMTYPE=0; };
struct MockForm {
    int type=0;
    MockBoundObject* object=nullptr;
    template<class T> T* As() { return static_cast<T*>(object); }
};
struct MockHandler {
    MockForm form;
    MockForm* LookupForm(std::uint32_t id,const char*) { return id ? &form : nullptr; }
    template<class T> T* LookupForm(std::uint32_t id,const char* file) {
        auto* f=LookupForm(id,file);
        return f && f->type==T::FORMTYPE ? f->template As<T>() : nullptr;
    }
};
void configFixture(const char* path) {
    using namespace PL;
    std::ifstream input(path,std::ios::binary);
    check(input.good(),"Shipped loot INI is accessible");
    const auto config=parseLootConfig(input);
    check(config.issues.empty(),"Shipped INI parses without warnings");
    check(config.entries.size()==std::size(Vanilla::pools),"Shipped INI includes every default item");
    for (std::size_t i=0;i<config.entries.size();++i) {
        const auto& entry=config.entries[i];const auto& original=Vanilla::pools[i];
        check(entry.plugin=="Skyrim.esm" && entry.localID==original.form && entry.category==original.category && entry.minimum==original.minimum && entry.childSafe==original.childSafe && entry.prisonerSafe==original.prisonerSafe,"Editable defaults preserve original item restrictions");
    }
}
void tests() { using namespace PL;
    {
        InventoryItemFacts facts;
        check(!needsPocketValue(facts),"Weapons and unclassified junk do not request pocket valuation");
        facts.kind=ItemKind::Armor;facts.slots=1u<<2;facts.jewelryKeyword=true;
        check(!needsPocketValue(facts),"Body clothes with jewelry keywords do not request valuation");
        facts.slots=1u<<6;
        check(needsPocketValue(facts),"Rings retain enchantment-aware valuation");
        facts.quest=true;check(!needsPocketValue(facts),"Quest jewelry is excluded before valuation");
        facts={};facts.kind=ItemKind::Misc;facts.gemKeyword=true;
        check(needsPocketValue(facts),"Gems retain their value accounting");
        facts.gold=true;check(!needsPocketValue(facts),"Coins use their count rather than engine value");
        using namespace Restore;
        check(!nativePricesNeeded({{false,0},{false,0}},true,true),"Use-all equipment bundles bypass price calculation");
        check(!nativePricesNeeded({{true,0},{false,0}},false,true),"Observed native equipment bypasses alternative prices");
        check(!nativePricesNeeded({{false,0},{false,0}},false,false),"Absent optional branches bypass prices");
        check(!nativePricesNeeded({{false,0}},false,true),"Single guaranteed branch needs no price calculation");
        check(nativePricesNeeded({{false,0},{false,0}},false,true),"Missing guaranteed alternatives still choose the cheaper branch");
        check(nativeCrossbowAllowed(Material::Steel,0,false,80,10,80,10),"A low-damage native steel crossbow can replace itself without using a stronger Dawnguard fallback");
        check(!nativeCrossbowAllowed(Material::Steel,0,false,80,10,120,19),"Crossbow integration cannot cause a stronger, costlier replacement");
        check(nativeCrossbowAllowed(Material::Iron,2,false,80,10,30,8),"A native iron crossbow remains a lower-tier fallback");
        check(!nativeCrossbowAllowed(Material::Steel,0,true,80,10,30,8),"Native enchanted crossbows are excluded");
        check(!nativeCrossbowAllowed(Material::Daedric,0,false,800,30,500,20),"A cheaper high-tier crossbow is still not a steel replacement");
        check(!nativeCrossbowAllowed(Material::Other,0,false,80,10,30,8),"Unrecognized native crossbows do not become arbitrary replacements");
        check(std::any_of(std::begin(forms),std::end(forms),[](const auto& f) {return f.kind==2 && f.form==0x801 && std::string_view(f.plugin)=="Dawnguard.esm";}),"Crossbows have a Dawnguard replacement independent of display name");
    }
    for (const auto& rule : Vanilla::rules) {
        if (rule.form == 0x1327F || rule.form == 0x13280 || rule.form == 0x10B1D8 || rule.form == 0x1BCC0 || rule.form == 0x43599)
            check(Vanilla::commonEnemySource(rule),"Soldiers/bandits use enemy balance sources");
        if (rule.form == 0x253F2 || rule.form == 0x253F3 || rule.form == 0x1325E || rule.form == 0x51596)
            check(!Vanilla::commonEnemySource(rule),"Town guards/jailors/merchants retain ordinary balance");
    }
    const std::array<Item,2> enemySupplyPool{{{1,Category::Food,2},{2,Category::Ingredient,1}}};
    int normalSupplies=0,enemySupplies=0,replacedPurses=0,supplyPurses=0;
    for (int seed=0;seed<3000;++seed) {
        Random normalRng{static_cast<std::uint64_t>(seed)}, enemyRng=normalRng;
        auto normal=generate(Role::Bandit,Wealth::Common,false,{},enemySupplyPool,normalRng);
        auto enemy=generate(Role::Bandit,Wealth::Common,false,{},enemySupplyPool,enemyRng,true);
        if (!enemy.items.empty()) {
            ++supplyPurses;
            if (enemy.gold==0) ++replacedPurses;
        }
        check(enemy.supplyReplacementGoldFallback==0 || (enemy.gold==0 && !enemy.items.empty()),"Gold replacement requires successful supply plan");
        normalSupplies+=static_cast<int>(normal.items.size()); enemySupplies+=static_cast<int>(enemy.items.size());
        check(enemy.gold<=profiles[2].goldHigh/2,"Enemy gold respects half target maximum");
        check(enemy.items.size()<=3,"Supply boost preserves stack cap");
        Inventory rich;rich.gold=1000;
        Random richRng{static_cast<std::uint64_t>(seed)};
        check(generate(Role::Bandit,Wealth::Common,false,rich,enemySupplyPool,richRng,true).gold==0,"Existing gold counts against reduced target");
        Random a{static_cast<std::uint64_t>(seed)},b=a;
        auto before=generate(Role::Guard,Wealth::Noble,false,{}, {},a);
        auto half=generate(Role::Guard,Wealth::Noble,false,{}, {},b,true);
        check(half.gold==std::max(2,before.gold/2),"Empty enemy purses halve gold with two coin minimum");
    }
    check(replacedPurses>supplyPurses*70/100 && replacedPurses<supplyPurses*80/100,"Supply additions replace coins in approximately 75 percent of enemy cycles");
    check(enemySupplies>normalSupplies,"Enemy supply frequency increases across seeds");
    {
        struct Facade {std::uint8_t data[12];};
        std::array<std::uint8_t,64> memory;memory.fill(0xA5);std::size_t requested=0;bool constructed=false;
        auto* object=Restore::allocateRuntimeExtraData<Facade>([&](std::size_t bytes) -> void* {requested=bytes;return memory.data();},[&](void* storage) {constructed=true;auto* bytes=static_cast<std::uint8_t*>(storage);std::fill(bytes,bytes+32,0x11);});
        check(object && constructed && requested==32,"Runtime constructor receives 32 bytes even when the combined-build facade is only 12 bytes");
        check(std::all_of(memory.begin()+requested,memory.end(),[](auto b) {return b==0xA5;}),"Simulated AE constructor does not overwrite the allocation boundary");
        check(Restore::extraDataCapacity(24)==32 && Restore::extraDataCapacity(32)==32 && Restore::extraDataCapacity(48)==48,"SE, modern AE, and larger future facades receive sufficient storage");
        constructed=false;
        check(!Restore::allocateRuntimeExtraData<Facade>([](std::size_t) -> void* {return nullptr;},[&](void*) {constructed=true;}) && !constructed,"Allocation failure does not call the native constructor on a null pointer");
    }
    {
        using namespace PL::Restore;
        check(equipmentArmorDays(0,true)==0.5 && equipmentArmorDays(999999,true)==0.5,"Guards restore armor after twelve game hours regardless of armor value");
        check(equipmentArmorDays(0,false)==2 && equipmentArmorDays(1000,false)==3,"Civilian armor retains its two-to-three-day delay");
        check(guardUniformAllowed(true,true,false,false) && !guardUniformAllowed(false,true,false,false) && !guardUniformAllowed(true,false,false,false) && !guardUniformAllowed(true,true,false,true) && !guardUniformAllowed(true,true,true,false),"Only guards with recognized plain armor retain exact uniforms");
        for (unsigned form:{0xC7F5Bu,0x21525u,0x21522u,0x21520u,0x2151Eu,0x2150Du,0x21508u,0x21507u,0xA6D7Bu,0xA6D7Fu,0x13ED9u,0x136D5u,0x13ED7u,0x136D6u}) check(std::any_of(std::begin(forms),std::end(forms),[&](const auto& f) {return f.form==form && f.kind==3 && std::string_view(f.plugin)=="Skyrim.esm";}),"Vanilla hold and civil-war body/feet uniform bindings are available");
        double guardDue=20;shortenGuardDeadline(guardDue,10);check(guardDue==10.5,"Updating an existing guard timer shortens a long wait");
        shortenGuardDeadline(guardDue,10.25);check(guardDue==10.5,"Repeated checks do not postpone guard replacements");
        guardDue=9;shortenGuardDeadline(guardDue,10);check(guardDue==9,"Expired guard deadlines stay expired");
        guardDue=-1;shortenGuardDeadline(guardDue,10);check(guardDue==-1,"Guard timer adjustment never arms an unnecessary replacement");
        check(cheapNativeClothing(25,false,false) && !cheapNativeClothing(26,false,false),"All NPCs accept only inexpensive native body clothing");
        check(cheapNativeClothing(10,true,false) && !cheapNativeClothing(11,true,false),"Native footwear uses a smaller budget");
        check(!cheapNativeClothing(1,false,true) && !cheapNativeClothing(-1,false,false),"Rich clothing and invalid prices cannot enter the cheap native pool");
        check(armorSlot(2,true) && armorSlot(3,true) && !armorSlot(2,false) && !armorSlot(4,true),"Native gauntlets and helmets restore without enabling clothing accessories");
        check(nativeBranches({{false,50},{false,100}},true,true)==std::vector<unsigned>({0,1}),"A use-all sword/bow bundle retains both missing weapon sources");
        check(nativeBranches({{true,50},{false,100}},true,true)==std::vector<unsigned>({0,1}),"A carried bow cannot hide a missing sword in a native bundle");
        check(nativeBranches({{false,100},{false,50}},false,true)==std::vector<unsigned>({1}),"Random alternatives restore a conservative choice rather than every possible weapon");
        check(nativeBranches({{true,100},{true,50}},false,true)==std::vector<unsigned>({0,1}),"All observed native weapon alternatives remain eligible");
        check(nativeBranches({{false,50}},false,false).empty(),"An absent optional native branch does not create a new weapon entitlement");
        CellReequip gate;
        gate.observe(10,slots[0]);check(!gate.canEquip(0,10),"New mark waits for a complete round trip");
        gate.observe(10,slots[0]);check(!gate.canEquip(0,10),"Same cell checks cannot dress actor");
        gate.observe(0,slots[0]);check(!gate.canEquip(0,10),"Missing parent cell is not travel");
        gate.observe(11,slots[0]);check(!gate.canEquip(0,11),"NPC leaving alone is insufficient");
        gate.observe(10,slots[0]);check(gate.canEquip(0,10),"NPC exit and re-entry qualifies");
        gate.observe(10,0);check(!gate.pending && !gate.ready,"Already dressed cancels pending work");
        gate.observe(10,slots[0]|slots[1]);gate.playerTravel(10,true);check(!gate.ready,"Entry without observed exit is insufficient");
        gate.playerTravel(99,false);gate.playerTravel(99,true);check(!gate.ready,"Other cell events do not qualify");
        gate.playerTravel(10,false);check(!gate.ready,"Player leaving alone is insufficient");
        gate.playerTravel(10,true);check(gate.canEquip(0,10)&&gate.canEquip(1,10),"Player round trip readies marked NPC slots");
        check(!gate.canEquip(0,11),"NPC must still be in the marked cell");
        gate.observe(10,slots[1]);check(!gate.canEquip(0,10)&&gate.canEquip(1,10),"Occupied body remains untouched while feet remain pending");
        gate.observe(10,0);gate.observe(10,slots[0]);check(!gate.ready,"A later undressing requires a new round trip");
        gate.observe(11,slots[0]);gate.playerTravel(11,false);gate.playerTravel(11,true);check(gate.canEquip(0,11),"Player can return to the NPC's newer cell too");
        check(clothingDays(false)==1 && clothingDays(true)==2,"Poor/beggar clothing takes twice as long");
        check(enabledSlot(0,true,true) && enabledSlot(0,false,false),"Body restoration remains available at every wealth level");
        check(enabledSlot(1,false,false) && !enabledSlot(1,true,false) && !enabledSlot(1,false,true),"Feet are restored only for non-poor non-beggars");
        check(!enabledSlot(2,false,false) && !enabledSlot(3,false,false),"Head and hand clothing restoration stays disabled");
        PL::Restore::State restricted;for (auto& c:restricted.wear) {c.due=5;c.armorDue=8;c.temporary=0x123;c.extraBase=0x456;c.extraUID=7;}
        applyClothingPolicy(restricted,true,false);
        check(restricted.wear[0].due==5 && restricted.wear[0].armorDue==8 && restricted.wear[1].due==-1 && restricted.wear[1].armorDue==8 && restricted.wear[2].due==-1 && restricted.wear[3].armorDue==8,"Clothing policy clears disabled clothing timers while retaining armor timers");
        check(restricted.wear[1].temporary==0x123 && restricted.wear[1].extraUID==7,"Disabling foot restoration does not delete existing clothes or their identity");
        check(armorDays(0)==2 && armorDays(500)==2.5 && armorDays(1000)==3 && armorDays(999999)==3,"Armor price determines a bounded two-to-three-day delay");
        check(!newWeaponsAllowed(true,false,false) && !newWeaponsAllowed(false,true,false) && !newWeaponsAllowed(false,false,true) && newWeaponsAllowed(false,false,false),"Poor, beggars, children and prisoners get no generated weapons");
        check(replacementMaterial(Material::Ebony,false,true)==Material::Steel && replacementMaterial(Material::Ebony,true,false)==Material::Elven,"Ebony follows the owner's elven race rather than restoring ebony");
        check(replacementMaterial(Material::Glass,false,false)==Material::Elven,"Glass downgrades to elven for either race");
        for (auto m:{Material::Daedric,Material::Dragonbone,Material::Dwarven,Material::Steel,Material::Other}) check(replacementMaterial(m,true,true)==Material::Steel,"Explicit steel downgrades and fallback do not become expensive elven replacements");
        check(replacementMaterial(Material::Iron,false,false)==Material::Iron,"Cheap iron weapons are not upgraded");
        check(!plainReplacementAllowed(100,10,101,10) && !plainReplacementAllowed(100,10,90,11) && plainReplacementAllowed(100,10,80,8),"Replacement cannot exceed original value or damage");
        double due=-1;armOnce(due,20,1);armOnce(due,20.9,1);check(due==21 && !dueNow(due,20.99) && dueNow(due,21),"Repeated encounters do not restart equipment timers");
        PL::State actor;actor.actor=0x123;actor.base=0x456;actor.generated=true;actor.equipment.initialized=true;
        auto& wear=actor.equipment.wear[0];wear.source=0x10;wear.temporary=0x20;wear.extraBase=actor.actor;wear.extraUID=7;wear.due=12;wear.armorDue=14.5;actor.equipment.weapons[1]={0x30,15};
        check(!rebindTemporary(wear,0x21,actor.actor,7,0x14,9) && rebindTemporary(wear,0x20,actor.actor,7,0x14,9),"Only the tracked temporary copy changes identity on transfer");
        check(wear.extraBase==0x14 && wear.extraUID==9 && wear.armorDue==14.5,"Stolen temporary clothing metadata survives without resetting armor countdown");
        Save::Data data;data.actors.emplace(actor.actor,actor);auto bytes=Save::encode(data);auto restored=Save::decode(4,bytes);
        check(restored && restored->actors.at(actor.actor).equipment.wear[0].armorDue==14.5 && restored->actors.at(actor.actor).equipment.weapons[1].due==15,"All restoration channels survive save/load");
        for (std::size_t n=0;n<bytes.size();++n) check(!Save::decode(4,std::span(bytes).first(n)),"Truncated restoration records fail closed");
        Save::Writer old;old.integer<std::uint64_t>(123);old.integer<std::uint32_t>(1);old.integer<std::uint32_t>(0x123);old.integer<std::uint32_t>(0x456);old.integer<std::uint64_t>(999);old.integer<std::uint32_t>(2);old.integer<std::uint8_t>(1);old.integer<std::uint8_t>(static_cast<std::uint8_t>(Role::Guard));old.integer<std::uint8_t>(static_cast<std::uint8_t>(Wealth::Common));old.real(24.5);
        auto migrated=Save::decode(3,old.bytes);check(migrated && migrated->actors.at(0x123).generated && migrated->actors.at(0x123).pickpocketDue==24.5 && !migrated->actors.at(0x123).equipment.initialized,"Version 1.0.0 schema-3 saves migrate without loot regeneration or timer changes");
    }
    {
        State notYetGenerated;
        check(armVictimTimer(notYetGenerated,5) && victimCooldownBlocks(notYetGenerated,6) && !notYetGenerated.generated,"Theft before first generation blocks an immediate refill until due");
        check(consumeVictimTimer(notYetGenerated,8) && !notYetGenerated.generated,"First-generation victim becomes eligible only at deadline");
        State victim;victim.actor=0x123;victim.base=0x456;victim.generated=true;victim.random=123456;
        check(armVictimTimer(victim,10.5) && victim.pickpocketDue==13.5,"First successful theft starts three game days");
        check(!armVictimTimer(victim,11.5) && victim.pickpocketDue==13.5,"Repeated theft does not extend countdown");
        check(victimCooldownBlocks(victim,13.499) && !consumeVictimTimer(victim,13.499) && victim.generated,"No refresh before deadline");
        check(!victimCooldownBlocks(victim,13.5) && consumeVictimTimer(victim,13.5),"Deadline makes victim reeligible");
        check(!victim.generated && victim.pickpocketDue==-1 && victim.cycle==1 && victim.random==123456,"Expiry clears only generation and timer state, preserving RNG");
        check(!consumeVictimTimer(victim,40) && victim.cycle==1,"Expiry consumed once with no catch-up rolls");
        victim.generated=true;
        check(armVictimTimer(victim,40) && victim.pickpocketDue==43,"New theft after replenishment starts a new countdown");
        resetCycle(victim);
        check(victim.pickpocketDue==-1 && !victim.generated && !consumeVictimTimer(victim,50),"Real reset supersedes timer without a later duplicate refresh");
        check(!armVictimTimer(victim,-1) && !armVictimTimer(victim,std::numeric_limits<double>::quiet_NaN()) && !armVictimTimer(victim,std::numeric_limits<double>::infinity()),"Invalid game clock cannot start a timer");
        check(armVictimTimer(victim,100),"Valid clock arms normally after invalid input");
        check(victimCooldownBlocks(victim,std::numeric_limits<double>::quiet_NaN()) && !consumeVictimTimer(victim,std::numeric_limits<double>::infinity()),"Invalid clocks never expire active timers");
        check(victimCooldownBlocks(victim,20),"Clock moving backwards cannot expire timer early");
        check(!armVictimTimer(victim,200) && victim.pickpocketDue==103,"Expired deferred timer is not restarted by another theft");
        Save::Data timers;timers.seed=123;timers.actors.emplace(victim.actor,victim);
        const auto bytes=Save::encode(timers);
        auto restored=Save::decode(Save::schema,bytes);
        check(restored && restored->actors.at(0x123).pickpocketDue==103 && restored->actors.at(0x123).generated==victim.generated,"Timer deadline and completion survive save/load");
        auto branch=timers;branch.actors.at(0x123).pickpocketDue=150;
        auto branchLoaded=Save::decode(Save::schema,Save::encode(branch));
        check(branchLoaded && branchLoaded->actors.at(0x123).pickpocketDue==150 && restored->actors.at(0x123).pickpocketDue==103,"Save branches retain their own deadlines");
        check(consumeVictimTimer(restored->actors.at(0x123),1000) && !consumeVictimTimer(restored->actors.at(0x123),1000),"Long unloaded interval allows one roll only");
        VictimInbox inbox;
        check(inbox.request(0x123,10) && !inbox.request(0x123,11),"Duplicate theft notifications coalesce at first timestamp");
        Save::Data pending;pending.actors.emplace(victim.actor,victim);pending.actors.at(victim.actor).pickpocketDue=-1;
        for (auto [id,when]:inbox.drain()) armVictimTimer(pending.actors.at(id),when);
        auto savedBeforeTask=Save::decode(Save::schema,Save::encode(pending));
        check(savedBeforeTask && savedBeforeTask->actors.at(0x123).pickpocketDue==13 && inbox.drain().empty(),"Saving before deferred task preserves timer and consumes notification once");
        check(inbox.request(0x456,20),"Victims have separate pending timestamps");inbox.clear();
        check(inbox.drain().empty(),"Game-session clear removes stale pending thefts");
        check(isVictimTransfer(0x123,0x14,1,0x123,true),"Positive NPC-to-player pickpocket transfer starts a timer");
        check(!isVictimTransfer(0x14,0x123,1,0x123,true),"Reverse-pickpocket gifts do not start a timer");
        check(!isVictimTransfer(0x123,0x14,0,0x123,true) && !isVictimTransfer(0x123,0x14,-1,0x123,true),"Failed or empty transfers do not count as theft");
        check(!isVictimTransfer(0x123,0x14,1,0x123,false),"Trade, ordinary looting, and follower exchanges are excluded by menu mode");
        check(!isVictimTransfer(0x456,0x14,1,0x123,true),"Unrelated scripted transfers cannot mark the open-menu victim");
        check(!isVictimTransfer(0,0x14,1,0,true) && !isVictimTransfer(0x123,0x456,1,0x123,true),"World pickups and NPC-to-NPC transfers are excluded");
        auto corrupt=bytes;
        for (auto bad:{-2.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}) {
            auto bits=std::bit_cast<std::uint64_t>(bad);
            for (std::size_t i=0;i<8;++i) corrupt[corrupt.size()-8+i]=static_cast<std::uint8_t>(bits>>(i*8));
            check(!Save::decode(Save::schema,corrupt),"Invalid serialized timer values fail closed");
        }
    }

    {
        std::istringstream text("\xEF\xBB\xBF; UTF-8 config\r\n[Jewelry]\r\n Some Mod.ESP|0x00000812 = CoMfortable, TRUE, 0 ; ring\r\n[Magic]\nLight.esl|801 = Wealthy, false, false\n");
        const auto parsed=parseLootConfig(text);
        check(parsed.issues.empty() && parsed.entries.size()==2,"BOM, CRLF, whitespace, comments and modded plugin names parse");
        check(parsed.entries[0].localID==0x812 && parsed.entries[0].minimum==Wealth::Comfortable && parsed.entries[0].childSafe && !parsed.entries[0].prisonerSafe,"Per-item wealth and safety restrictions retained");
        check(parsed.entries[1].category==Category::Magic && parsed.entries[1].localID==0x801,"Light plugin IDs remain local");
        std::istringstream empty("; intentionally empty\n[Food]\n");
        check(parseLootConfig(empty).entries.empty(),"Empty editable pools stay empty");
        for (const auto* bad:{"Skyrim.esm|0x01000812 = Common, false, false", "Skyrim.esm|0 = Common, false, false", "Skyrim.esm|oops = Common, false, false", "../Mod.esp|812 = Common, false, false", "Mod.dll|812 = Common, false, false", "Mod.esp|812 = Rich, false, false", "Mod.esp|812 = Common, maybe, false", "Mod.esp|812 = Common, false", "Mod.esp|812 = Common, false, false, true", "Mod.esp|812|813 = Common, false, false"}) {
            std::istringstream malformed(std::string("[Food]\n")+bad+"\nSkyrim.esm|65C97 = Destitute, true, true\n");
            const auto result=parseLootConfig(malformed);
            check(result.issues.size()==1 && result.entries.size()==1 && result.entries[0].localID==0x65C97,"Invalid entry logs an issue without discarding valid following items");
        }
        std::istringstream unknown("[Arrows]\nMod.esp|812 = Common, false, false\n[Food]\nSkyrim.esm|65C97 = Destitute, true, true\n");
        const auto skipped=parseLootConfig(unknown);
        check(skipped.issues.size()==2 && skipped.entries.size()==1,"Unknown category cannot inherit previous category");
        std::vector<Item> custom;
        upsertLootItem(custom,{0x123,Category::Gem,100,Wealth::Common,false,false});
        upsertLootItem(custom,{0x456,Category::Food,5,Wealth::Destitute,true,false});
        upsertLootItem(custom,{0x123,Category::Jewelry,300,Wealth::Wealthy,false,false});
        check(custom.size()==2 && custom[0].category==Category::Jewelry && custom[0].value==300 && custom[0].minimum==Wealth::Wealthy,"Last resolved definition replaces duplicate without adding selection weight");
        for (std::uint64_t seed=0;seed<100;++seed) {
            Random random{seed};
            const auto plan=generate(Role::Beggar,Wealth::Destitute,false,{},custom,random);
            for (const auto& added:plan.items) check(added.form!=0x123,"Custom items obey minimum wealth");
        }
        InventoryItemFacts item;item.kind=ItemKind::Armor;item.knownCategory=Category::Jewelry;item.slots=1u<<6;
        check(canGenerateAs(item,Category::Jewelry),"Custom ring does not need vanilla jewelry keywords");
        item.slots|=1u<<2;
        check(!canGenerateAs(item,Category::Jewelry),"Config cannot turn body clothing into valuable jewelry");
        item={};item.kind=ItemKind::Other;
        for (std::size_t i=0;i<categories;++i) check(!canGenerateAs(item,static_cast<Category>(i)),"Keys, ammunition, books, and weapons cannot enter supported loot categories");
        item.kind=ItemKind::Misc;item.gold=true;
        check(!canGenerateAs(item,Category::Gem) && !canGenerateAs(item,Category::Lockpick),"Gold cannot be injected through non-gold pools");
        item.gold=false;
        check(canGenerateAs(item,Category::Gem),"Custom miscellaneous valuables can be explicitly categorized");
        item.kind=ItemKind::Food;
        check(canGenerateAs(item,Category::Food) && !canGenerateAs(item,Category::Potion),"Food cannot be miscategorized as a potion");
        for (std::uint64_t seed=0;seed<100;++seed) {
            Random random{seed};
            auto plan=generate(Role::Noble,Wealth::Noble,false,{},std::span<const Item>{},random);
            check(plan.items.empty() && plan.gold>=2,"Empty custom pools still generate coins without disabling distribution");
        }
    }

    {
        ResetInbox inbox;
        State state; state.actor=123; state.base=456; state.generated=true; state.random=789;
        refreshBaseIdentity(state,999);
        check(state.generated && state.random==789 && state.base==999,"Template changes preserve generation state");
        check(inbox.request(123) && !inbox.request(123),"Duplicate reset notifications coalesce");
        Save::Data data; data.actors.emplace(123,state);
        for(auto id:inbox.drain()) resetCycle(data.actors.at(id));
        auto restored=Save::decode(Save::schema,Save::encode(data));
        check(restored && !restored->actors.at(123).generated && restored->actors.at(123).cycle==1,"Save before queued reset preserves pending generation");
        check(!inbox.take(123),"Consumed save-time reset cannot run twice");
        {auto scope=inbox.prepare(123); check(!inbox.request(123),"Initialization reset folds into pending first generation");}
        check(inbox.request(123) && inbox.take(123),"Later genuine reset remains eligible");
        Plan plan; plan.maxStacks=4;plan.maxValuables=3;plan.valuablesBudget=100;
        Addition gem{1,2,Category::Gem};
        Inventory original,current;
        check(boundedAddition(plan,original,current,gem,30)==2,"Original affordable addition remains intact");
        current.counts[index(Category::Gem)]=1;current.valuables=1;current.valuablesValue=30;current.stacks=1;
        check(boundedAddition(plan,original,current,gem,30)==1,"Other distributor fills part of category shortfall");
        current.counts[index(Category::Gem)]=2;
        check(boundedAddition(plan,original,current,gem,30)==0,"Other distributor satisfies category without duplicate loot");
        current={};current.valuablesValue=80;
        check(boundedAddition(plan,original,current,gem,30)==0,"Shared valuable budget rechecked against live inventory");
        current={};current.stacks=4;
        check(boundedAddition(plan,original,current,gem,30)==0,"Live total stack limit enforced");
        current={};current.valuables=3;
        check(boundedAddition(plan,original,current,gem,30)==0,"Live shared valuable count limit enforced");
        current={};
        check(boundedAddition(plan,original,current,gem,70)==1,"Changed item price cannot exceed strict budget");
        Inventory coins;InventoryItemFacts gold;gold.gold=true;gold.quest=true;gold.count=7;
        accountInventoryItem(coins,gold);
        check(coins.gold==7 && coins.stacks==0,"Existing quest-tagged coins still count toward gold target");
    }

    using namespace PL;
    const std::vector<Item> pool{
        {1,Category::Food,2,Wealth::Destitute,true,true},
        {2,Category::Food,4},
        {3,Category::Jewelry,8,Wealth::Destitute,true,true},
        {4,Category::Jewelry,100},
        {5,Category::Gem,20},
        {6,Category::Potion,20},
        {7,Category::Ingredient,2,Wealth::Destitute,true,false},
        {8,Category::Magic,30},
        {9,Category::Lockpick,2}
    };
    MockBoundObject object;
    MockHandler handler{{1,&object}};
    for (int formType=1;formType<=7;++formType) {
        handler.form.type=formType;
        check(handler.LookupForm<MockBoundObject>(1,"Skyrim.esm")==nullptr,"Old exact-base-type lookup reproduces failed inventory resolution");
        check(resolveVanillaInventoryObject<MockBoundObject>(&handler,1)==&object,"Fixed lookup resolves derived inventory records including gold");
    }
    check(resolveVanillaInventoryObject<MockBoundObject>(&handler,0)==nullptr,"Missing inventory form handled safely");
    handler.form.object=nullptr;
    check(resolveVanillaInventoryObject<MockBoundObject>(&handler,1)==nullptr,"Non-inventory form cannot be cast to a bound object");

    Inventory outfit;
    InventoryItemFacts robe;robe.kind=ItemKind::Armor;robe.count=1;robe.baseValue=10000;robe.slots=1u<<2;
    accountInventoryItem(outfit,robe);
    auto helmet=robe;helmet.slots=1u;accountInventoryItem(outfit,helmet);
    auto boots=robe;boots.slots=1u<<7;accountInventoryItem(outfit,boots);
    auto sword=robe;sword.kind=ItemKind::Other;accountInventoryItem(outfit,sword);
    auto junk=sword;junk.kind=ItemKind::Misc;accountInventoryItem(outfit,junk);
    check(outfit.valuablesValue==0 && outfit.valuables==0 && outfit.stacks==0,"Expensive clothes, armor, weapons, and junk are excluded from pocket budgets");
    robe.jewelryKeyword=true;robe.knownCategory=Category::Jewelry;
    accountInventoryItem(outfit,robe);
    check(outfit.valuablesValue==0,"Even jewelry-tagged body clothing cannot reduce gold");
    auto mixed=robe;mixed.slots|=1u<<6;accountInventoryItem(outfit,mixed);
    check(outfit.valuablesValue==0,"Outfit reserving a ring slot is still not jewelry");
    InventoryItemFacts ring=robe;ring.slots=1u<<6;ring.baseValue=100;
    InventoryItemFacts necklace=ring;necklace.slots=1u<<5;necklace.baseValue=200;
    accountInventoryItem(outfit,ring);accountInventoryItem(outfit,necklace);
    check(outfit.valuablesValue==300 && outfit.valuables==2,"Rings and necklaces count as valuables, worn or unworn");
    JewelryValueAccumulator mixedJewelry(3);mixedJewelry.add(1,500);
    auto enchantedRing=ring;enchantedRing.count=3;enchantedRing.jewelryTotalValue=mixedJewelry.finish(100);
    Inventory enchanted;accountInventoryItem(enchanted,enchantedRing);
    check(enchanted.valuablesValue==700 && enchanted.valuables==3 && enchanted.stacks==1,"One custom enchanted ring and two plain rings retain separate values and one stack");
    JewelryValueAccumulator innate(2);
    check(innate.finish(350)==700,"Built-in enchanted forms use engine value for ordinary instances");
    JewelryValueAccumulator bounded(2);bounded.add(9,500);
    check(bounded.finish(100)==1000,"Extra-data counts cannot overvalue inventory");
    JewelryValueAccumulator negative(2);negative.add(1,-20);
    check(negative.finish(100)==100,"Negative engine prices are clamped");
    auto enchantedClothes=robe;enchantedClothes.jewelryTotalValue=99999;
    Inventory excluded;accountInventoryItem(excluded,enchantedClothes);
    check(excluded.valuablesValue==0,"Enchantment value never promotes clothing into jewelry");
    auto questEnchanted=enchantedRing;questEnchanted.quest=true;
    accountInventoryItem(excluded,questEnchanted);
    check(excluded.valuablesValue==0,"Enchanted quest jewelry stays excluded");
    InventoryItemFacts gem;gem.kind=ItemKind::Misc;gem.count=1;gem.baseValue=50;gem.gemKeyword=true;
    accountInventoryItem(outfit,gem);
    check(outfit.valuablesValue==350,"Gemstone base value counts toward offset");
    auto recognizedRing=necklace;recognizedRing.jewelryKeyword=false;
    check(pocketCategory(recognizedRing)==Category::Jewelry,"Known vanilla jewelry still counts if another mod strips its keyword");
    auto recognizedGem=gem;recognizedGem.gemKeyword=false;recognizedGem.knownCategory=Category::Gem;
    check(pocketCategory(recognizedGem)==Category::Gem,"Known vanilla gemstone still counts if another mod strips its keyword");
    Inventory supplies;
    for (auto kind:{ItemKind::Food,ItemKind::Potion,ItemKind::Ingredient,ItemKind::Magic}) {
        InventoryItemFacts supply;supply.kind=kind;supply.count=1;supply.baseValue=10000;accountInventoryItem(supplies,supply);
    }
    check(supplies.valuablesValue==0 && supplies.valuables==0 && supplies.stacks==4,"Expensive supplies satisfy their own categories but never reduce gold");
    Inventory coins;InventoryItemFacts money;money.gold=true;money.count=7;accountInventoryItem(coins,money);
    check(coins.gold==7 && coins.valuablesValue==0,"Existing gold is subtracted separately, not as a valuable");
    Inventory quest;ring.quest=true;accountInventoryItem(quest,ring);
    check(quest.valuablesValue==0,"Quest jewelry remains excluded from accounting");
    for (std::uint64_t seed=0;seed<100;++seed) {
        Inventory clothed;
        auto ordinaryClothes=robe;ordinaryClothes.jewelryKeyword=false;ordinaryClothes.knownCategory=Category::None;
        accountInventoryItem(clothed,ordinaryClothes);accountInventoryItem(clothed,sword);
        Random bareRng{seed},clothedRng{seed};
        auto barePlan=generate(Role::Merchant,Wealth::Wealthy,false,{},pool,bareRng);
        auto clothedPlan=generate(Role::Merchant,Wealth::Wealthy,false,clothed,pool,clothedRng);
        check(barePlan.gold==clothedPlan.gold,"Wearing expensive non-valuables cannot change the gold roll");
    }
    for (std::uint64_t seed=0;seed<1000;++seed) {
        for (int w=0;w<6;++w) {
            Random rng{seed};
            Inventory empty;
            auto plan=generate(Role::Fallback,static_cast<Wealth>(w),false,empty,pool,rng);
            check(plan.gold>0 || !plan.items.empty(),"Generation must never leave empty pockets");
            if (plan.items.empty()) check(plan.gold>=2,"Coins-only pockets need a few coins");
            check(plan.gold<=profiles[w].goldHigh,"Gold must obey profile ceiling");
            check(plan.items.size()<=static_cast<std::size_t>(profiles[w].maxStacks),"Stack cap");
            int value=0,valuables=0;
            for (auto& addition:plan.items) {
                check(addition.count>0,"Positive additions only");
                const auto item=std::find_if(pool.begin(),pool.end(),[&](auto& i) {return i.form==addition.form;});
                if (item->category==Category::Jewelry || item->category==Category::Gem) {value+=item->value*addition.count; valuables+=addition.count;}
            }
            check(value<=profiles[w].budgetHigh,"Combined valuables value cap");
            check(valuables<=profiles[w].maxValuables,"Combined valuables count cap");
        }
        for (auto role:{Role::Child,Role::Prisoner}) {
            Random rng{seed};
            auto p=generate(role,Wealth::Destitute,false,{},pool,rng);
            check(p.gold>0 || !p.items.empty(),"Restricted profiles cannot be empty");
            check(p.items.size()<=1,"Restricted stack cap");
            for (auto& addition:p.items) {
                auto item=std::find_if(pool.begin(),pool.end(),[&](auto& i) {return i.form==addition.form;});
                check(role==Role::Child ? item->childSafe : item->prisonerSafe,"Restricted pool enforcement");
            }
            check(p.gold<=(role==Role::Child?5:3),"Restricted gold ceiling");
        }
    }
    for (int w=0;w<6;++w) {
        Inventory rich; rich.gold=1000; rich.counts.fill(100); rich.stacks=100; rich.valuables=100; rich.valuablesValue=100000;
        Random rng{42};
        auto plan=generate(Role::Noble,static_cast<Wealth>(w),true,rich,pool,rng);
        check(plan.gold==0 && plan.items.empty(),"Existing above-target inventory must not stack loot");
    }
    const std::vector<Item> expensiveOnly{{1,Category::Jewelry,30},{2,Category::Gem,100}};
    for (std::uint64_t seed=0;seed<1000;++seed) {
        Random strict{seed};
        auto p=generate(Role::Beggar,Wealth::Destitute,false,{},expensiveOnly,strict);
        check(p.items.empty(),"Strict budget never grants an unaffordable valuable");
        check(p.gold>=2 && p.gold<=4,"Unaffordable-only pool falls back to a few coins");
        Inventory stocked;stocked.counts.fill(3);stocked.gold=1000;stocked.stacks=7;
        Random deficits{seed};
        p=generate(Role::Noble,Wealth::Noble,true,stocked,pool,deficits);
        check(p.gold==0 && p.items.empty(),"Current stocked categories satisfy target without spam");
    }
    Inventory worn; worn.gold=20; worn.counts[index(Category::Jewelry)]=2; worn.valuables=2; worn.valuablesValue=1000; worn.stacks=2;
    for (std::uint64_t seed=0;seed<100;++seed) {
        Random rng{seed};
        auto p=generate(Role::Merchant,Wealth::Wealthy,false,worn,pool,rng);
        check(p.gold==0,"Existing valuables reduce gold down to floor before subtracting current gold");
        for (auto& a:p.items) check(a.category!=Category::Jewelry && a.category!=Category::Gem,"Existing valuable budget consumed");
    }
    State state; state.actor=0x123;state.base=0x456;state.random=123456;state.generated=true;
    Save::Data data;data.seed=123;data.actors.emplace(state.actor,state);
    auto bytes=Save::encode(data);
    auto loaded=Save::decode(Save::schema,bytes);
    check(loaded.has_value(),"Valid state roundtrip");
    auto& roundtrip=loaded->actors.at(state.actor);
    check(roundtrip.generated,"Save/load must preserve loot completion");
    check(Save::encode(*loaded)==bytes,"Deterministic save format");
    Save::Data otherBranch=data;otherBranch.actors.at(state.actor).generated=false;
    auto earlier=Save::decode(Save::schema,Save::encode(otherBranch));
    check(!earlier->actors.at(state.actor).generated && loaded->actors.at(state.actor).generated,"Save branches keep independent generation state");
    check(loaded->actors.at(state.actor).random==state.random,"Update/load retains RNG sequence");
    check(!Save::decode(Save::schema+1,bytes),"Future schema must fail closed");
    for (std::size_t size=0;size<bytes.size();++size) check(!Save::decode(Save::schema,std::span(bytes).first(size)),"Truncated state must be rejected");
    bytes.push_back(0);check(!Save::decode(Save::schema,bytes),"Trailing garbage rejected");
    resetCycle(roundtrip);
    check(!roundtrip.generated && roundtrip.cycle==1,"Reset starts one new loot cycle");

    // Independent schema-2 fixture from the pre-timer release.
    const std::string oldV2Hex="7b0000000000000001000000230100005604000040e201000000000007000000010d02";
    std::vector<std::uint8_t> oldV2;
    for (std::size_t i=0;i<oldV2Hex.size();i+=2) oldV2.push_back(static_cast<std::uint8_t>(std::stoi(oldV2Hex.substr(i,2),nullptr,16)));
    auto migratedV2=Save::decode(2,oldV2);
    check(migratedV2 && migratedV2->actors.at(0x123).generated && migratedV2->actors.at(0x123).pickpocketDue==-1 && migratedV2->actors.at(0x123).random==123456,"Pre-timer schema-2 save migrates without regeneration or retroactive timer");
    for (std::size_t size=0;size<oldV2.size();++size) check(!Save::decode(2,std::span(oldV2).first(size)),"Truncated schema-2 saves are rejected");

    // Golden schema-1 fixture encoded independently with Python's little-endian struct
    // format, including discarded legacy ammunition fields and a protected stack entry.
    const std::string fixtureHex="7b0000000000000001000000230100005604000040e20100000000000700000001012d0d020000000000403140010000006300000005000000";
    std::vector<std::uint8_t> legacy;
    for (std::size_t i=0;i<fixtureHex.size();i+=2) legacy.push_back(static_cast<std::uint8_t>(std::stoi(fixtureHex.substr(i,2),nullptr,16)));
    auto migrated=Save::decode(1,legacy);
    check(migrated.has_value(),"v0.1.0 save migrates to current loot-only state");
    auto& old=migrated->actors.at(0x123);
    check(old.generated && old.cycle==7,"Migration preserves completion and reset cycle; no loot reroll");
    check(old.base==0x456 && old.random==123456 && migrated->seed==123,"Migration preserves identity and RNG");
    check(old.role==Role::Fallback && old.wealth==Wealth::Common,"Migration preserves selected profile");
    const auto updated=Save::encode(*migrated);
    check(Save::schema==4,"Updated saves use equipment restoration schema 4; old ammunition fields are migration-only");
    auto reloaded=Save::decode(Save::schema,updated);
    check(reloaded && reloaded->actors.at(0x123).generated,"Updated save remains initialized after reload");
    for (std::size_t size=0;size<legacy.size();++size) check(!Save::decode(1,std::span(legacy).first(size)),"Truncated legacy saves rejected");
    legacy[32]=0; // Same legacy actor pending its first/next genuine generation.
    auto pending=Save::decode(1,legacy);
    check(pending && !pending->actors.at(0x123).generated,"Pending legacy generation remains pending");
}
}
int main(int argc,char** argv) {
    try { check(argc==2,"Pass the shipped INI fixture path"); configFixture(argv[1]); tests(); std::cout<<"Passed "<<checks<<" behavior checks\n"; return 0; }
    catch (const std::exception& e) { std::cerr<<"Failed after "<<checks<<" checks: "<<e.what()<<'\n'; return 1; }
}


