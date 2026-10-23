"""Build factual form-ID tables from read-only master inspection, not guessed IDs."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
records = json.loads((root / 'research/vanilla-records.json').read_text(encoding='utf-8'))
by_name = {r.get('editor','').lower(): r for r in records}
roles = ['Beggar','Farmer','Smith','Guard','Hunter','Innkeeper','Merchant','Priest','Mage','Thief','Bandit','Noble','Adventurer','Fallback','Child','Prisoner']

def form(name):
    row = by_name[name.lower()]
    return int(row['id'],16)

rules = []
def rule(kind, role, names, priority=10, magical=False):
    for name in names:
        rules.append((kind,form(name),role,priority,magical,name))

rule('Faction','Prisoner',['dunPrisonerFaction','TownCidhnaMinePrisonerFaction','MS09NorthwatchPrisonerFaction','dunFellglow_PrisonerFaction','SolitudeOpeningPrisonerFaction'],0)
rule('Class','Prisoner',['Prisoner'],0)
rule('Faction','Noble',['JobJarlFaction','JobStewardFaction','WERoad02NobleFaction','JobOrcChiefFaction'],20)
rule('Faction','Beggar',['FavorJobsBeggarsFaction'],30)
rule('Class','Beggar',['Beggar'],30)
rule('Faction','Innkeeper',['JobInnkeeperFaction','JobRentRoomFaction'],40)
rule('Faction','Smith',['JobBlacksmithFaction','JobMinerFaction'],50)
rule('Class','Smith',['VendorBlacksmith','Miner'],50)
rule('Faction','Hunter',['HunterFaction','FishermanFaction','WEServicesHunterFaction'],60)
rule('Faction','Farmer',['JobFarmerFaction','JobLumberjackFaction'],70)
rule('Class','Farmer',['Farmer','Lumberjack'],70)
rule('Faction','Priest',['JobPriestFaction'],80)
rule('Class','Priest',['Priest'],80)
rule('Faction','Thief',['JobFenceFaction','WIThiefFaction'],90)
rule('Faction','Merchant',['JobMerchantFaction','JobJewelerFaction','JobTailorFaction','JobStreetVendorFaction','JobFletcherFaction','JobApothecaryFaction'],100)
rule('Class','Merchant',['VendorTailor','VendorPawnbroker','VendorFletcher','VendorApothecary','VendorFood'],110)
rule('Faction','Bandit',['BanditFaction','ForswornFaction'],120)
rule('Class','Bandit',['EncClassBanditMelee','EncClassBanditMissile','EncClassForsworn','EncClassForswornMissile'],125)
rule('Class','Bandit',['EncClassBanditWizard','EncClassForswornShaman'],125,True)
rule('Faction','Mage',['JobCourtWizardFaction'],15,True)
rule('Faction','Mage',['JobSpellFaction','WarlockFaction'],130,True)
rule('Class','Mage',['VendorSpells'],130,True)
rule('Faction','Adventurer',['CompanionsFaction','WEUnaggressiveAdventurerFaction','WEAggressiveAdventurerFaction','WEBountyHunter','dunIronbindAdventurerFaction'],150)
rule('Faction','Thief',['ThievesGuildFaction'],190)
rule('Faction','Mage',['CollegeofWinterholdFaction'],190,True)
rule('Faction','Adventurer',['DarkBrotherhoodFaction'],190)
rule('Class','Guard',['GuardImperial','GuardSonsSkyrim','GuardOrc1H','GuardOrc2H','SoldierImperialNotGuard','SoldierSonsSkyrimNotGuard','CWSoldierClass','Jailor'],10)
for r in records:
    if r['type'] != 'CLAS': continue
    name = r.get('editor','')
    if any(word in name for word in ['CombatMage','CombatMystic','CombatSorcerer','CombatNightblade','CombatWitchblade','CombatSpellsword']) or re.match(r'Trainer(Alteration|Conjuration|Destruction|Enchanting|Illusion|Restoration)',name):
        rule('Class','Mage',[name],200,True)
    elif name in ['CombatWarrior1H','CombatWarrior2H','CombatBarbarian','CombatRanger','CombatScout','CombatMonk','CombatAssassin','CombatRogue','CombatThief','CombatNightingale','NPCclassBelrand','EncClassPenitusOculatus','EncClassThalmorMelee','EncClassThalmorMissile','EncClassAlikrMelee','EncClassAlikrMissile']:
        rule('Class','Adventurer',[name],210)

pools = []
def pool(category,names,minimum='Destitute',child=False,prisoner=False):
    for name in names:
        row = by_name[name.lower()]
        if row['type'] not in {'ALCH','ARMO','MISC','INGR','SCRL','SLGM'}: raise ValueError(name)
        pools.append((form(name),category,minimum,child,prisoner,name))

pool('Food',['FoodBread01A','FoodBread01B'],child=True,prisoner=True)
pool('Food',['FoodApple','FoodApple02','FoodCarrot','FoodPotato','FoodCheeseWedge01','FoodCheeseWedge02','FoodBeefCooked','FoodVenisonCooked','FoodSalmonCooked'],child=True)
pool('Food',['Ale','FoodMead','FoodWineBottle02','FoodWineAlto','FoodHonningbrewMead','FoodBlackBriarMead'])
pool('Food',['FoodSolitudeSpicedWine','FoodBlackBriarMeadPrivateReserve'],'Comfortable')
pool('Jewelry',[r['editor'] for r in records if r['type']=='ARMO' and r.get('editor','').startswith('Jewelry')],child=True,prisoner=True)
pool('Gem',[r['editor'] for r in records if r['type']=='MISC' and re.match(r'^gem(?:garnet|amethyst|ruby|sapphire|emerald|diamond)(?:flawless)?$',r.get('editor',''),re.I)])
for n in range(1,5):
    pool('Potion',[f'RestoreHealth0{n}',f'RestoreMagicka0{n}',f'RestoreStamina0{n}'],['Destitute','Poor','Common','Comfortable'][n-1])
pool('Ingredient',['MountainFlower01Blue','MountainFlower01Red','MountainFlower01Purple'],child=True)
pool('Ingredient',['Lavender','TundraCotton','Thistle01','SaltPile','Garlic','Snowberry','FrostMirriam','ElvesEar','BoneMeal'])
pool('Magic',['CandlelightScroll','OakfleshScroll','FireboltScroll','IceSpikeScroll','LightningBoltScroll'])
for r in records:
    name = r.get('editor','')
    if r['type'] == 'SLGM' and re.match(r'^SoulGem(Petty|Lesser|Common|Greater|Grand)(Filled)?$',name,re.I):
        minimum = 'Wealthy' if re.search('Greater|Grand',name) else 'Common' if 'Common' in name else 'Destitute'
        pool('Magic',[name],minimum)
pool('Lockpick',['Lockpick'])

race_names = ['ArgonianRace','BretonRace','DarkElfRace','HighElfRace','ImperialRace','KhajiitRace','NordRace','OrcRace','RedguardRace','WoodElfRace']
race_names += [r['editor'] for r in records if r['type']=='RACE' and (r.get('editor','').removesuffix('Vampire') in race_names or 'RaceChild' in r.get('editor',''))]
race_names += ['ElderRace','ElderRaceVampire']
race_names = sorted(set(race_names))

lines = ['// Generated from Skyrim.esm by scripts/generate_vanilla_data.py. No game assets included.','#pragma once','#include "Core.h"','namespace PL::Vanilla {',
         'struct Rule { std::uint32_t form; Role role; int priority; bool magical; bool faction; };',
         'struct PoolEntry { std::uint32_t form; Category category; Wealth minimum; bool childSafe; bool prisonerSafe; };',
         f'inline constexpr std::uint32_t gold = 0x{form("Gold001"):08X};',
         f'inline constexpr std::uint32_t currentFollower = 0x{form("CurrentFollowerFaction"):08X};',
         f'inline constexpr std::uint32_t courtWizard = 0x{form("JobCourtWizardFaction"):08X};',
         'inline constexpr Rule rules[] = {']
for kind,fid,role,priority,magic,name in rules:
    lines.append(f'    {{0x{fid:08X}, Role::{role}, {priority}, {str(magic).lower()}, {str(kind=="Faction").lower()}}}, // {name}')
lines += ['};','inline constexpr PoolEntry pools[] = {']
for fid,cat,minimum,child,prisoner,name in pools:
    lines.append(f'    {{0x{fid:08X}, Category::{cat}, Wealth::{minimum}, {str(child).lower()}, {str(prisoner).lower()}}}, // {name}')
lines += ['};','inline constexpr std::uint32_t races[] = {']
for name in race_names: lines.append(f'    0x{form(name):08X}, // {name}')
lines += ['};','}']
(root / 'src/VanillaData.h').write_text('\n'.join(lines)+'\n',encoding='utf-8')
report = ['# Verified vanilla mapping registry','', 'Extracted read-only from installed Skyrim.esm. This is factual identifier data, not copied game assets.','', '## Occupation mappings','', '| Record | FormID | Role | Priority |','|---|---|---|---|']
for kind,fid,role,priority,magic,name in rules: report.append(f'| {name} ({kind}) | {fid:08X} | {role} | {priority} |')
report += ['', '## Item pool', '', '| Record | FormID | Category | Minimum wealth |','|---|---|---|---|']
for fid,cat,minimum,child,prisoner,name in pools: report.append(f'| {name} | {fid:08X} | {cat} | {minimum} |')
report += ['', '## Eligible race forms', ''] + [f'- {name}: {form(name):08X}' for name in race_names]
(root / 'research/verified-mappings.md').write_text('\n'.join(report)+'\n',encoding='utf-8')
print(f'Generated {len(rules)} role rules, {len(pools)} pool items, {len(race_names)} race mappings')
