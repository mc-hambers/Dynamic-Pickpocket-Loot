// Copyright (C) 2026 Maxmc. GPL-3.0-or-later.
#pragma once
namespace PL {
// Main-thread only. False means deferred; true means the single check completed.
inline bool equipInventoryJewelry(RE::Actor* actor){
 if(!actor||actor->IsDead()||!actor->Is3DLoaded()||actor->IsInCombat()||actor->GetCurrentScene()||actor->AsActorState()->GetSitSleepState()==RE::SIT_SLEEP_STATE::kIsSleeping)return false;
 struct Candidate{RE::TESObjectARMO* armor;RE::ExtraDataList* extra;unsigned mask;int value;};std::vector<Candidate> candidates;unsigned occupied=0;
 auto inventory=actor->GetInventory();
 for(auto& [object,data]:inventory)if(object&&data.first>0&&data.second){auto* armor=object->As<RE::TESObjectARMO>();if(!armor)continue;const auto mask=armor->GetSlotMask().underlying();
  if(data.second->IsWorn()){occupied|=mask;continue;}
  if(!(mask&0x60)||!armor->GetPlayable()||data.second->IsQuestObject())continue;
  RE::ExtraDataList* extra=nullptr;if(data.second->extraLists)for(auto* x:*data.second->extraLists)if(x){extra=x;break;}
  candidates.push_back({armor,extra,mask,data.second->GetValue()});
 }
 std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.value!=b.value?a.value>b.value:a.armor->GetFormID()<b.armor->GetFormID();});
 for(const auto& item:candidates)if(!(occupied&item.mask)){
  RE::ActorEquipManager::GetSingleton()->EquipObject(actor,item.armor,item.extra,1,nullptr,true,false,false);occupied|=item.mask;
 }
 return true;
}
}
