#include "Core.h"
#include "InventoryAccounting.h"
#include "FormResolution.h"
#include "DistributionLifecycle.h"
#include "Serialization.h"
#include "VanillaData.h"
#include "EnemyBalance.h"
#include "LootConfig.h"
#include "VictimTimer.h"
#include "EquipmentRestore.h"
#include <filesystem>
#include <fstream>
#include <spdlog/sinks/basic_file_sink.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_set>

namespace logger = SKSE::log;

namespace {
using namespace PL;
using namespace std::chrono_literals;
constexpr std::uint32_t serializationID = 0x504C5431; // PLT1
constexpr std::uint32_t stateRecord = 0x53544154; // STAT

struct RuntimeRule { RE::TESForm* form; Role role; int priority; bool magical; bool faction; bool commonEnemy; };
struct RoleResult { Role role = Role::Fallback; bool magical = false; bool courtMage = false; bool combatEnemy = false; };
struct RestoreIDChange {std::uint32_t form,oldBase,newBase;std::uint16_t oldID,newID;};
RE::TESBoundObject* vanillaInventoryObject(RE::TESDataHandler* handler,RE::FormID id) {
    // LookupForm<T> compares exact FORMTYPE. TESBoundObject inherits None, so
    // requesting that base type rejects real MISC/ALCH/ARMO/etc. Use As<T>,
    // whose CommonLib FormTraits handle the derived inventory object types.
    return resolveVanillaInventoryObject<RE::TESBoundObject>(handler,id);
}
class Manager final :
    public RE::BSTEventSink<RE::TESObjectLoadedEvent>,
    public RE::BSTEventSink<RE::TESResetEvent>,
    public RE::BSTEventSink<RE::TESFormDeleteEvent>,
    public RE::BSTEventSink<RE::TESContainerChangedEvent>,
    public RE::BSTEventSink<RE::ItemsPickpocketed::Event>,
    public RE::BSTEventSink<RE::TESUniqueIDChangeEvent> {
public:
    static Manager& get() { static Manager instance; return instance; }
    Save::Data saved;
    std::atomic_bool active{false};
    std::atomic_uint64_t session{0};
    bool validSave = true;

    void initialize() {
        auto* handler = RE::TESDataHandler::GetSingleton();
        for (const auto& r : Vanilla::rules) {
            if (auto* form = handler->LookupForm(r.form,"Skyrim.esm")) rules.push_back({form,r.role,r.priority,r.magical,r.faction,Vanilla::commonEnemySource(r)});
            else logger::warn("Missing role form {:08X}",r.form);
        }
        // Retain vanilla recognition even if a form is removed from generation.
        for (const auto& p:Vanilla::pools)
            if (auto* object=vanillaInventoryObject(handler,p.form)) poolCategories[object->GetFormID()]=p.category;
        for (auto id : Vanilla::races) if (auto* race = handler->LookupForm<RE::TESRace>(id,"Skyrim.esm")) races.insert(race->GetFormID());
        gold = vanillaInventoryObject(handler,Vanilla::gold);
        follower = handler->LookupForm<RE::TESFaction>(Vanilla::currentFollower,"Skyrim.esm");
        courtWizard = handler->LookupForm<RE::TESFaction>(Vanilla::courtWizard,"Skyrim.esm");
        gemKeyword = handler->LookupForm<RE::BGSKeyword>(0x914ED,"Skyrim.esm");
        jewelryKeyword = handler->LookupForm<RE::BGSKeyword>(0x6BBE9,"Skyrim.esm");
        ringKeyword = handler->LookupForm<RE::BGSKeyword>(0x10CD09,"Skyrim.esm");
        necklaceKeyword = handler->LookupForm<RE::BGSKeyword>(0x10CD0A,"Skyrim.esm");
        loadPools(handler);
        restoration.initialize();
        auto* source = RE::ScriptEventSourceHolder::GetSingleton();
        source->AddEventSink<RE::TESObjectLoadedEvent>(this);
        source->AddEventSink<RE::TESResetEvent>(this);
        source->AddEventSink<RE::TESFormDeleteEvent>(this);
        source->AddEventSink<RE::TESContainerChangedEvent>(this);
        source->AddEventSink<RE::TESUniqueIDChangeEvent>(this);
        if (auto* thefts=RE::ItemsPickpocketed::GetEventSource()) thefts->AddEventSink(this);
        else logger::warn("Pickpocket statistics event unavailable; using container transfer detection");
        logger::info("Loaded {} active pool items, {} role rules, {} races; gold resolved={}",pool.size(),rules.size(),races.size(),gold != nullptr);
        worker = std::jthread([this](std::stop_token stop) {
            while (!stop.stop_requested()) {
                for (int i=0;i<20 && !stop.stop_requested();++i) std::this_thread::sleep_for(250ms);
                if (stop.stop_requested()) return;
                if (active.load()) queueScan();
            }
        });
    }
    void newSeed() {
        saved.seed = static_cast<std::uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }
    void clear() {
        active = false;
        ++session;
        saved = {};
        validSave = true;
        newSeed();
        resetInbox.clear();
        victimInbox.clear();
        preparingActors.clear();
        {std::scoped_lock lock(restoreMutex);restoreChanges.clear();}
        queuedScanSession = noQueuedScan;
    }
    void enterGame() {
        if (!validSave || !gold) {
            logger::error("Distribution disabled: validSave={}, goldResolved={}, poolSize={}",validSave,gold != nullptr,pool.size());
            return;
        }
        for (auto& [id,state]:saved.actors) state.equipment.initialized=false; // Rescan native weapon bundles; preserve saved deadlines and loot flags.
        active = true;
        diagnosticGenerations = 0;
        diagnosticWarnings = 0;
        diagnosticTimers = 0;
        diagnosticScanLogged = false;
        logger::info("Distribution enabled; preserving loot state for {} actors",saved.actors.size());
        logger::info("Victim refresh enabled: {} game days after first successful theft; no inventory reset",victimCooldownDays);
        queueScan();
    }
    template<class F> void queue(F task) {
        auto generation = session.load();
        if (auto* tasks = SKSE::GetTaskInterface()) tasks->AddTask([this,generation,task=std::move(task)] {
            if (active.load() && session.load() == generation) task();
        });
    }
    bool human(RE::Actor* actor) const {
        return actor && !actor->IsPlayerRef() && !actor->IsSummoned() && !actor->GetCommandingActor() && actor->GetRace() && races.contains(actor->GetRace()->GetFormID());
    }
    bool following(RE::Actor* actor) const {
        return actor->IsPlayerTeammate() || (follower && actor->IsInFaction(follower));
    }
    RoleResult classify(RE::Actor* actor) const {
        auto* base = actor->GetActorBase();
        RoleResult result;
        if (!base) return result;
        int best = 1000;
        for (const auto& r : rules) {
            const bool match = r.faction ? actor->IsInFaction(r.form->As<RE::TESFaction>()) : base->npcClass == r.form;
            if (!match) continue;
            if (r.commonEnemy && !base->IsUnique() && !base->IsEssential() && !base->IsProtected()) result.combatEnemy = true;
            if (r.magical) result.magical = true;
            if (r.priority < best) { best = r.priority; result.role = r.role; }
        }
        if (result.role == Role::Prisoner) return result;
        if (actor->IsChild()) { result.role = Role::Child; return result; }
        if (actor->IsGuard()) result.role = Role::Guard;
        else if (best > 110 && !(result.role == Role::Mage && best <= 130)) {
            // CommonLib's first call calculates the cache but returns its old value.
            auto* vendor=actor->GetVendorFaction();
            if (!vendor) vendor=actor->GetVendorFaction();
            if (vendor) result.role=Role::Merchant;
        }
        if (actor->IsGuard() || (result.role != Role::Bandit && result.role != Role::Guard && result.role != Role::Mage && result.role != Role::Adventurer)) result.combatEnemy = false;
        result.courtMage = courtWizard && actor->IsInFaction(courtWizard);
        return result;
    }
    State& stateFor(RE::Actor* actor) {
        auto id = actor->GetFormID();
        auto base = actor->GetActorBase()->GetFormID();
        auto [it,inserted] = saved.actors.try_emplace(id);
        auto& state = it->second;
        if (inserted) {
            state = {};
            state.actor = id; state.base = base;
            Random seed{saved.seed ^ (static_cast<std::uint64_t>(id) << 32) ^ base};
            state.random = seed.next();
        } else refreshBaseIdentity(state,base);
        return state;
    }
    InventoryItemFacts itemFacts(RE::TESBoundObject* item) const {
        InventoryItemFacts facts;
        facts.gold=item==gold;
        if (auto it=poolCategories.find(item->GetFormID());it != poolCategories.end()) facts.knownCategory=it->second;
        auto keyword=[item](RE::BGSKeyword* k) { return k && item->HasKeywordInArray({k},false); };
        if (auto* armor=item->As<RE::TESObjectARMO>()) {
            facts.kind=ItemKind::Armor;
            facts.slots=armor->GetSlotMask().underlying();
            facts.jewelryKeyword=keyword(jewelryKeyword) || keyword(ringKeyword) || keyword(necklaceKeyword);
        } else if (item->Is(RE::FormType::Misc)) {
            facts.kind=ItemKind::Misc;
            facts.gemKeyword=keyword(gemKeyword);
        } else if (auto* alchemy=item->As<RE::AlchemyItem>()) {
            if (alchemy->IsFood()) facts.kind=ItemKind::Food;
            else if (!alchemy->IsPoison()) facts.kind=ItemKind::Potion;
        } else if (item->Is(RE::FormType::Ingredient)) facts.kind=ItemKind::Ingredient;
        else if (item->Is(RE::FormType::SoulGem) || item->Is(RE::FormType::Scroll)) facts.kind=ItemKind::Magic;
        return facts;
    }
    int poolValue(RE::TESBoundObject* item,Category category) const {
        if (category==Category::Jewelry) {
            RE::InventoryEntryData entry(item,1);
            return std::max(0,entry.GetValue());
        }
        return std::max(0,item->GetGoldValue());
    }
    void loadPools(RE::TESDataHandler* handler) {
        std::vector<LootConfigEntry> configured;
        const auto path=std::filesystem::path(REL::Module::get().filePath().data()).parent_path()/"Data/SKSE/Plugins/DynamicPickpocketLoot.ini";
        std::ifstream input(path,std::ios::binary);
        bool useDefaults=true;
        if (input) {
            input.seekg(0,std::ios::end);
            const auto size=input.tellg();
            if (size>=0 && size<=1024*1024) {
                input.seekg(0);
                auto parsed=parseLootConfig(input);
                if (!input.bad()) {
                    configured=std::move(parsed.entries);
                    useDefaults=false;
                    for (const auto& issue:parsed.issues) logger::warn("Loot INI line {}: {}",issue.line,issue.message);
                }
            }
        }
        if (useDefaults) {
            logger::warn("Loot INI missing, unreadable, or over 1 MiB; using built-in vanilla pools");
            for (const auto& p:Vanilla::pools) configured.push_back({"Skyrim.esm",p.form,p.category,p.minimum,p.childSafe,p.prisonerSafe,0});
        } else logger::info("Loading loot pools from {} ({} valid lines)",path.string(),configured.size());
        for (const auto& entry:configured) {
            const auto* file=handler->LookupModByName(entry.plugin);
            if (!file || file->compileIndex==0xFF || (file->IsLight() && entry.localID>0xFFF)) {
                logger::warn("Loot INI line {}: missing plugin or invalid local ID {}|{:06X}",entry.line,entry.plugin,entry.localID);
                continue;
            }
            auto* form=handler->LookupForm(entry.localID,entry.plugin);
            auto* object=form ? form->As<RE::TESBoundObject>() : nullptr;
            if (!object || !object->GetPlayable() || object->IsDeleted()) {
                logger::warn("Loot INI line {}: missing/unusable item {}|{:06X}",entry.line,entry.plugin,entry.localID);
                continue;
            }
            auto facts=itemFacts(object);
            facts.knownCategory=entry.category;
            if (!canGenerateAs(facts,entry.category)) {
                logger::warn("Loot INI line {}: incompatible item/category {}|{:06X}",entry.line,entry.plugin,entry.localID);
                continue;
            }
            Item value{object->GetFormID(),entry.category,poolValue(object,entry.category),entry.minimum,entry.childSafe,entry.prisonerSafe};
            // A form appears once; the last valid definition wins, not extra weight.
            upsertLootItem(pool,value);
            poolCategories[value.form]=value.category;
        }
        if (pool.empty()) logger::info("No non-gold pool entries active; generation will use coins only");
    }
    Inventory pocketSnapshot(RE::TESObjectREFR::InventoryItemMap& inventory) const {
        Inventory result;
        for (auto& [item,data] : inventory) {
            if (!item || data.first <= 0) continue;
            auto facts=itemFacts(item);
            facts.count=data.first;
            facts.quest=data.second && data.second->IsQuestObject();
            // Ignore weapons, outfits and other non-valuables before entering
            // Skyrim's valuation code. Jewelry below retains enchantment pricing.
            if (needsPocketValue(facts) && pocketCategory(facts)==Category::Gem)
                facts.baseValue=item->GetGoldValue();
            if (!facts.quest && pocketCategory(facts)==Category::Jewelry) {
                JewelryValueAccumulator value(facts.count);
                if (data.second && data.second->extraLists) {
                    for (auto* extra:*data.second->extraLists) {
                        if (!extra) continue;
                        // Own only a temporary list of pointers, never the NPC's
                        // extra-data objects. GetValue uses Skyrim's valuation.
                        RE::InventoryEntryData instance(item,1);
                        instance.AddExtraList(extra);
                        value.add(extra->GetCount(),instance.GetValue());
                    }
                }
                RE::InventoryEntryData ordinary(item,1);
                facts.jewelryTotalValue=value.finish(ordinary.GetValue());
            }
            accountInventoryItem(result,facts);
        }
        return result;
    }
    void logGeneration(RE::Actor* actor,const Inventory& before,const Plan& plan) {
        if (diagnosticGenerations < 20) {
            logger::info("Generated {:08X} '{}': existing gold={}, qualifying jewelry/gem value={}, pocket stacks={}, added gold={}, added stacks={}, common combat enemy={}",actor->GetFormID(),actor->GetName(),before.gold,before.valuablesValue,before.stacks,plan.gold,plan.items.size(),classify(actor).combatEnemy);
            ++diagnosticGenerations;
        }
    }
    void visit(RE::Actor* actor) {
        if (!human(actor) || actor->IsDead() || actor->IsDeleted() || actor->IsDisabled() || !actor->Get3D() || following(actor) || !actor->GetActorBase()) return;
        const auto id=actor->GetFormID();
        const auto generation=session.load();
        if (preparingActors.contains(id) || resetInbox.contains(id)) return;
        flushPendingVictimTimers();
        auto& state=stateFor(actor);
        if (state.generated) restoreEquipment(actor,state);
        const auto now=gameDays();
        if (victimCooldownBlocks(state,now)) return;
        if (consumeVictimTimer(state,now) && diagnosticTimers++<20)
            logger::info("Victim {:08X} became eligible after cooldown; preserving existing inventory",id);
        if (state.generated) return;
        // Inventory initialization may run callbacks. Prevent re-entry and fold
        // its own initialization resets into this still-uninitialized cycle.
        preparingActors.insert(id);
        struct GenerationGuard {
            std::set<std::uint32_t>& actors;
            std::uint32_t id;
            ~GenerationGuard() { actors.erase(id); }
        } guard{preparingActors,id};
        State working=state;
        Random random{working.random};
        const auto role=classify(actor);
        working.role=role.role;
        working.wealth=role.courtMage ? Wealth::Wealthy : selectWealth(role.role,random);
        RE::TESObjectREFR::InventoryItemMap inventory;
        {
            auto preparation=resetInbox.prepare(id);
            if (!actor->GetInventoryChanges()) {
                if (diagnosticWarnings++ < 10) logger::warn("Inventory not ready for {:08X}; leaving generation pending",id);
                return;
            }
            inventory=actor->GetInventory();
        }
        if (session.load()!=generation || !active.load()) return;
        flushPendingVictimTimers();
        const auto currentState=saved.actors.find(id);
        if (currentState==saved.actors.end() || currentState->second.pickpocketDue!=working.pickpocketDue) return;
        const auto snapshot=pocketSnapshot(inventory);
        std::vector<Item> currentPool;
        currentPool.reserve(pool.size());
        for (const auto& candidate:pool) {
            auto* item=RE::TESForm::LookupByID<RE::TESBoundObject>(candidate.form);
            if (!item || item->IsDeleted() || !item->GetPlayable()) continue;
            auto current=candidate;
            current.value=poolValue(item,candidate.category);
            currentPool.push_back(current);
        }
        const auto plan=generate(working.role,working.wealth,role.magical,snapshot,currentPool,random,role.combatEnemy);
        working.generated=true;
        working.random=random.state;
        // Commit before engine additions, and do not overwrite this state after
        // callbacks: a save/reset during an addition may have marked it pending.
        flushRestoreChanges();
        working.equipment=saved.actors.at(id).equipment;
        saved.actors[id]=working;
        int goldAdded=0;
        std::size_t stacksAdded=0;
        bool suppliesAdded=false;
        if (plan.gold>0 && !resetInbox.contains(id)) {
            actor->AddObjectToContainer(gold,nullptr,plan.gold,nullptr);
            goldAdded=plan.gold;
        }
        for (const auto& addition:plan.items) {
            if (session.load()!=generation || !active.load() || resetInbox.contains(id) || !saved.actors.contains(id) || !saved.actors.at(id).generated || actor->IsDead() || following(actor)) break;
            auto* item=RE::TESForm::LookupByID<RE::TESBoundObject>(addition.form);
            if (!item || item->IsDeleted() || !item->GetPlayable()) continue;
            // Another mod may have added possessions in response to our last
            // addition. Recheck live quantities and the original shared budget.
            auto currentInventory=actor->GetInventory();
            if (session.load()!=generation || !active.load() || resetInbox.contains(id)) break;
            const auto current=pocketSnapshot(currentInventory);
            const int count=boundedAddition(plan,snapshot,current,addition,poolValue(item,addition.category));
            if (count>0) { actor->AddObjectToContainer(item,nullptr,count,nullptr); ++stacksAdded;
                if (addition.category==Category::Food || addition.category==Category::Ingredient) suppliesAdded=true;
            }
        }
        // If a supply became invalid or another callback filled the cap, retain the
        // original coin shortfall instead of leaving a failed replacement empty.
        if (!suppliesAdded && plan.supplyReplacementGoldFallback>0 && session.load()==generation && active.load() && !resetInbox.contains(id) && saved.actors.contains(id) && saved.actors.at(id).generated && !actor->IsDead() && !following(actor)) {
            auto fallbackInventory=actor->GetInventory();
            const auto current=pocketSnapshot(fallbackInventory);
            const int deficit=std::max(0,snapshot.gold+plan.supplyReplacementGoldFallback-current.gold);
            if (deficit>0 && session.load()==generation && active.load() && !resetInbox.contains(id)) { actor->AddObjectToContainer(gold,nullptr,deficit,nullptr); goldAdded+=deficit; }
        }
        Plan applied=plan;applied.gold=goldAdded;applied.items.resize(stacksAdded);
        logGeneration(actor,snapshot,applied);
        if (auto found=saved.actors.find(id);found!=saved.actors.end() && found->second.generated) restoreEquipment(actor,found->second);
    }
    void restoreEquipment(RE::Actor* actor,State& state) {
        const auto id=actor->GetFormID(),cycle=state.cycle;const auto generation=session.load();
        const bool alreadyPreparing=preparingActors.contains(id);
        if (!alreadyPreparing) preparingActors.insert(id);
        auto preparation=resetInbox.prepare(id);
        flushRestoreChanges();
        restoration.run(actor,state,gameDays(),[&,this] {return active.load() && session.load()==generation && state.cycle==cycle && !resetInbox.contains(id) && !actor->IsDead() && !following(actor);});
        flushRestoreChanges();
        if (!alreadyPreparing) preparingActors.erase(id);
    }
    void flushRestoreChanges() {
        std::vector<RestoreIDChange> changes;{std::scoped_lock lock(restoreMutex);changes.swap(restoreChanges);}
        for (const auto& e:changes) for (auto& [id,state]:saved.actors) for (auto& c:state.equipment.wear) Restore::rebindTemporary(c,e.form,e.oldBase,e.oldID,e.newBase,e.newID);
    }
    double gameDays() const {
        const auto* calendar=RE::Calendar::GetSingleton();
        return calendar ? static_cast<double>(calendar->GetDaysPassed()) : std::numeric_limits<double>::quiet_NaN();
    }
    RE::Actor* pickpocketTarget() const {
        auto* ui=RE::UI::GetSingleton();
        if (!ui || !ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME) || RE::ContainerMenu::GetContainerMode()!=RE::ContainerMenu::ContainerMode::kPickpocket) return nullptr;
        const auto target=RE::TESObjectREFR::LookupByHandle(RE::ContainerMenu::GetTargetRefHandle());
        return target ? target->As<RE::Actor>() : nullptr;
    }
    void requestVictimTimer(RE::Actor* actor) {
        if (!active.load() || !validSave || !human(actor) || actor->IsDead() || following(actor) || !actor->GetActorBase()) return;
        if (!victimInbox.request(actor->GetFormID(),gameDays())) return;
        queue([this] { flushPendingVictimTimers(); });
    }
    void flushPendingVictimTimers() {
        for (auto [id,when]:victimInbox.drain()) {
            auto* actor=RE::TESForm::LookupByID<RE::Actor>(id);
            // Recruiting a victim after theft must not cancel or restart a timer.
            if (!human(actor) || !actor->GetActorBase()) continue;
            auto& state=stateFor(actor);
            if (armVictimTimer(state,when) && diagnosticTimers++<20)
                logger::info("Victim timer started for {:08X} '{}': due game day {}",id,actor->GetName(),state.pickpocketDue);
        }
    }
    void flushPendingResetState() {
        for (auto id:resetInbox.drain()) {
            auto found=saved.actors.find(id);
            if (found!=saved.actors.end()) resetCycle(found->second);
            // No record means first generation is already naturally pending.
        }
    }
    void queueScan() {
        const auto generation=session.load();
        if (queuedScanSession.exchange(generation)==generation) return;
        if (auto* tasks=SKSE::GetTaskInterface()) tasks->AddTask([this,generation] {
            auto expected=generation;
            queuedScanSession.compare_exchange_strong(expected,noQueuedScan);
            if (active.load() && session.load()==generation) scan();
        });
    }
    void scan() {
        if (!validSave || RE::UI::GetSingleton()->GameIsPaused()) return;
        std::vector<RE::ActorHandle> actors;
        RE::ProcessLists::GetSingleton()->ForAllActors([&](RE::Actor* actor) {
            if (actor) actors.push_back(actor->GetHandle());
            return RE::BSContainer::ForEachResult::kContinue;
        });
        if (!diagnosticScanLogged && !actors.empty()) {
            logger::info("Initial live scan found {} loaded actor handles",actors.size());
            diagnosticScanLogged = true;
        }
        for (auto& handle : actors) if (auto actor = handle.get()) visit(actor.get());
    }
    void requestReset(RE::Actor* actor) {
        if (!active.load() || !human(actor)) return;
        auto id = actor->GetFormID();
        if (!resetInbox.request(id)) return;
        queue([this,id] {
            if (!resetInbox.take(id)) return;
            auto* actor = RE::TESForm::LookupByID<RE::Actor>(id);
            if (!human(actor) || !actor->GetActorBase()) return;
            flushPendingVictimTimers();
            auto& state = stateFor(actor);
            resetCycle(state);
            logger::debug("Actor {:08X} reset cycle {}",id,state.cycle);
            visit(actor);
        });
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* event,RE::BSTEventSource<RE::TESContainerChangedEvent>*) override {
        if (event && active.load() && event->newContainer==0x14 && event->itemCount>0) {
            auto* victim=pickpocketTarget();
            if (victim && isVictimTransfer(event->oldContainer,event->newContainer,event->itemCount,victim->GetFormID(),true)) requestVictimTimer(victim);
        }
        return RE::BSEventNotifyControl::kContinue;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESUniqueIDChangeEvent* e,RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*) override {
        if (e && active.load()) {std::scoped_lock lock(restoreMutex);restoreChanges.push_back({e->objectID,e->oldBaseID,e->newBaseID,e->oldUniqueID,e->newUniqueID});}
        return RE::BSEventNotifyControl::kContinue;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::ItemsPickpocketed::Event* event,RE::BSTEventSource<RE::ItemsPickpocketed::Event>*) override {
        // The synchronous successful-pickpocket stat notification also covers
        // menus that defer their container-change events until after closing.
        if (event && event->numItems>0 && active.load()) requestVictimTimer(pickpocketTarget());
        return RE::BSEventNotifyControl::kContinue;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESObjectLoadedEvent* event,RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override {
        if (event && event->loaded && active.load()) {const auto id=event->formID;queue([this,id] {
            auto* actor=RE::TESForm::LookupByID<RE::Actor>(id);if (human(actor)) visit(actor);
        });}
        return RE::BSEventNotifyControl::kContinue;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESResetEvent* event,RE::BSTEventSource<RE::TESResetEvent>*) override {
        if (event && event->object) requestReset(event->object->As<RE::Actor>());
        return RE::BSEventNotifyControl::kContinue;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* event,RE::BSTEventSource<RE::TESFormDeleteEvent>*) override {
        if (event && active.load()) {
            auto id=event->formID;
            queue([this,id] {
                // A queued deletion must not erase a newly created live reference reusing the ID.
                auto* form=RE::TESForm::LookupByID(id);
                if (!form || form->IsDeleted()) saved.actors.erase(id);
            });
        }
        return RE::BSEventNotifyControl::kContinue;
    }
private:
    Restore::Adapter restoration;
    std::mutex restoreMutex;
    std::vector<RestoreIDChange> restoreChanges;
    std::vector<Item> pool;
    std::vector<RuntimeRule> rules;
    std::unordered_map<std::uint32_t,Category> poolCategories;
    std::unordered_set<std::uint32_t> races;
    RE::TESBoundObject* gold = nullptr;
    RE::TESFaction* follower = nullptr;
    RE::TESFaction* courtWizard = nullptr;
    RE::BGSKeyword* gemKeyword = nullptr;
    RE::BGSKeyword* jewelryKeyword = nullptr;
    RE::BGSKeyword* ringKeyword = nullptr;
    RE::BGSKeyword* necklaceKeyword = nullptr;
    ResetInbox resetInbox;
    VictimInbox victimInbox;
    std::uint32_t diagnosticTimers=0;
    std::set<std::uint32_t> preparingActors;
    static constexpr std::uint64_t noQueuedScan=std::numeric_limits<std::uint64_t>::max();
    std::atomic_uint64_t queuedScanSession{noQueuedScan};
    std::uint32_t diagnosticWarnings=0;
    std::uint32_t diagnosticGenerations = 0;
    bool diagnosticScanLogged = false;
    std::jthread worker;
};

struct ResetHook {
    static inline REL::Relocation<void(*)(RE::Actor*,bool)> actorOriginal;
    static inline REL::Relocation<void(*)(RE::Actor*,bool)> characterOriginal;
    static void actor(RE::Actor* self,bool leveledOnly) { actorOriginal(self,leveledOnly); Manager::get().requestReset(self); }
    static void character(RE::Actor* self,bool leveledOnly) { characterOriginal(self,leveledOnly); Manager::get().requestReset(self); }
    static void install() {
        // CommonLibSSE-NG Actor::ResetInventory uses slot 0x8A on both flat SE and AE.
        REL::Relocation<std::uintptr_t> a{RE::VTABLE_Actor[0]};
        REL::Relocation<std::uintptr_t> c{RE::VTABLE_Character[0]};
        actorOriginal = a.write_vfunc(0x8A,actor);
        characterOriginal = c.write_vfunc(0x8A,character);
    }
};

void save(SKSE::SerializationInterface* api) {
    auto& manager=Manager::get();
    // Preserve an error marker rather than silently replacing unreadable state with fresh loot.
    if (!manager.validSave) { api->OpenRecord(stateRecord,0xFFFFFFFF); return; }
    manager.flushPendingVictimTimers();
    manager.flushPendingResetState();
    manager.flushRestoreChanges();
    auto bytes=Save::encode(manager.saved);
    if (!api->OpenRecord(stateRecord,Save::schema) || !api->WriteRecordData(bytes.data(),static_cast<std::uint32_t>(bytes.size()))) logger::error("Failed to serialize loot state");
}
void load(SKSE::SerializationInterface* api) {
    auto& manager=Manager::get();
    manager.clear();
    std::uint32_t type,version,length;
    bool seen=false;
    while (api->GetNextRecordInfo(type,version,length)) {
        if (type!=stateRecord) continue;
        if (seen || length>64*1024*1024) { manager.validSave=false; continue; }
        seen=true;
        std::vector<std::uint8_t> bytes(length);
        if (api->ReadRecordData(bytes.data(),length)!=length) { manager.validSave=false; continue; }
        auto data=Save::decode(version,bytes);
        if (!data) { manager.validSave=false; logger::error("Unreadable loot state schema {}; generation disabled to prevent duplicates",version); continue; }
        manager.saved.seed=data->seed;
        for (auto& [id,state]:data->actors) {
            std::uint32_t actor,base;
            if (!api->ResolveFormID(id,actor)) continue;
            // A temporary/template base can disappear while the reference still
            // survives. Keep its completed flag; stateFor refreshes base identity.
            if (!api->ResolveFormID(state.base,base)) base=state.base;
            state.actor=actor; state.base=base;
            auto resolve=[api](std::uint32_t id) {std::uint32_t resolved=0;if (id) api->ResolveFormID(id,resolved);return resolved;};
            for (auto& c:state.equipment.wear) {c.source=resolve(c.source);c.temporary=resolve(c.temporary);c.extraBase=resolve(c.extraBase);if (!c.temporary) {c.extraBase=0;c.extraUID=0;}}
            for (auto& c:state.equipment.weapons) {c.source=resolve(c.source);if (!c.source) c.due=-1;}
            manager.saved.actors.emplace(actor,std::move(state));
        }
    }
    const auto timers=std::count_if(manager.saved.actors.begin(),manager.saved.actors.end(),[](const auto& pair) { return pair.second.pickpocketDue>=0; });
    logger::info("Loaded persistent state for {} actors, including {} victim timers",manager.saved.actors.size(),timers);
}
void revert(SKSE::SerializationInterface*) { Manager::get().clear(); }
void message(SKSE::MessagingInterface::Message* message) {
    auto& manager=Manager::get();
    switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded: manager.initialize(); ResetHook::install(); break;
    case SKSE::MessagingInterface::kPreLoadGame: manager.active=false; ++manager.session; break;
    case SKSE::MessagingInterface::kPostLoadGame:
        if (message->data) manager.enterGame();
        break;
    case SKSE::MessagingInterface::kNewGame: manager.clear(); manager.enterGame(); break;
    default: break;
    }
}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    const auto path=SKSE::log::log_directory();
    if (path) {
        auto logPath=*path / "PickpocketingLoot.log";
        auto sink=std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(),true);
        auto log=std::make_shared<spdlog::logger>("PickpocketingLoot",std::move(sink));
        spdlog::set_default_logger(std::move(log));
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::info);
    }
    logger::info("Dynamic Pickpocket Loot 1.1.3; runtime {}",REL::Module::get().version().string());
    const auto version=REL::Module::get().version();
    const std::set<unsigned> aeBuilds{317,318,323,342,353,629,640,659,1130,1170,1179};
    const bool supported=version.major()==1 && ((version.minor()==5 && version.patch()==97) || (version.minor()==6 && aeBuilds.contains(version.patch())) || (version.minor()==7 && (version.patch()==99 || version.patch()==104)));
    if (!supported) { logger::error("Unsupported runtime: {}",version.string()); return false; }
    auto* api=SKSE::GetSerializationInterface();
    if (!api || !SKSE::GetTaskInterface()) return false;
    api->SetUniqueID(serializationID);
    api->SetSaveCallback(save); api->SetLoadCallback(load); api->SetRevertCallback(revert);
    return SKSE::GetMessagingInterface()->RegisterListener(message);
}

