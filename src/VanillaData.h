// Generated from Skyrim.esm by scripts/generate_vanilla_data.py. No game assets included.
#pragma once
#include "Core.h"
namespace PL::Vanilla {
struct Rule { std::uint32_t form; Role role; int priority; bool magical; bool faction; };
struct PoolEntry { std::uint32_t form; Category category; Wealth minimum; bool childSafe; bool prisonerSafe; };
inline constexpr std::uint32_t gold = 0x0000000F;
inline constexpr std::uint32_t currentFollower = 0x0005C84E;
inline constexpr std::uint32_t courtWizard = 0x0005091E;
inline constexpr Rule rules[] = {
    {0x000AA784, Role::Prisoner, 0, false, true}, // dunPrisonerFaction
    {0x000284AE, Role::Prisoner, 0, false, true}, // TownCidhnaMinePrisonerFaction
    {0x000C1AA5, Role::Prisoner, 0, false, true}, // MS09NorthwatchPrisonerFaction
    {0x0007E7A3, Role::Prisoner, 0, false, true}, // dunFellglow_PrisonerFaction
    {0x0010FD8E, Role::Prisoner, 0, false, true}, // SolitudeOpeningPrisonerFaction
    {0x00013263, Role::Prisoner, 0, false, false}, // Prisoner
    {0x00050920, Role::Noble, 20, false, true}, // JobJarlFaction
    {0x00050922, Role::Noble, 20, false, true}, // JobStewardFaction
    {0x00102D60, Role::Noble, 20, false, true}, // WERoad02NobleFaction
    {0x0005159B, Role::Noble, 20, false, true}, // JobOrcChiefFaction
    {0x00060028, Role::Beggar, 30, false, true}, // FavorJobsBeggarsFaction
    {0x0001327B, Role::Beggar, 30, false, false}, // Beggar
    {0x0005091B, Role::Innkeeper, 40, false, true}, // JobInnkeeperFaction
    {0x0009CC97, Role::Innkeeper, 40, false, true}, // JobRentRoomFaction
    {0x0005091D, Role::Smith, 50, false, true}, // JobBlacksmithFaction
    {0x00051597, Role::Smith, 50, false, true}, // JobMinerFaction
    {0x00013257, Role::Smith, 50, false, false}, // VendorBlacksmith
    {0x0001326D, Role::Smith, 50, false, false}, // Miner
    {0x0004E509, Role::Hunter, 60, false, true}, // HunterFaction
    {0x00073FC3, Role::Hunter, 60, false, true}, // FishermanFaction
    {0x000AC9C2, Role::Hunter, 60, false, true}, // WEServicesHunterFaction
    {0x00051591, Role::Farmer, 70, false, true}, // JobFarmerFaction
    {0x00051595, Role::Farmer, 70, false, true}, // JobLumberjackFaction
    {0x0001326C, Role::Farmer, 70, false, false}, // Farmer
    {0x0001326E, Role::Farmer, 70, false, false}, // Lumberjack
    {0x0005159A, Role::Priest, 80, false, true}, // JobPriestFaction
    {0x00013276, Role::Priest, 80, false, false}, // Priest
    {0x000806AA, Role::Thief, 90, false, true}, // JobFenceFaction
    {0x00023EEC, Role::Thief, 90, false, true}, // WIThiefFaction
    {0x00051596, Role::Merchant, 100, false, true}, // JobMerchantFaction
    {0x000806A9, Role::Merchant, 100, false, true}, // JobJewelerFaction
    {0x000806A8, Role::Merchant, 100, false, true}, // JobTailorFaction
    {0x0007F6BE, Role::Merchant, 100, false, true}, // JobStreetVendorFaction
    {0x00051592, Role::Merchant, 100, false, true}, // JobFletcherFaction
    {0x0005091C, Role::Merchant, 100, false, true}, // JobApothecaryFaction
    {0x00013270, Role::Merchant, 110, false, false}, // VendorTailor
    {0x0001325B, Role::Merchant, 110, false, false}, // VendorPawnbroker
    {0x00013259, Role::Merchant, 110, false, false}, // VendorFletcher
    {0x00013258, Role::Merchant, 110, false, false}, // VendorApothecary
    {0x00013256, Role::Merchant, 110, false, false}, // VendorFood
    {0x0001BCC0, Role::Bandit, 120, false, true}, // BanditFaction
    {0x00043599, Role::Bandit, 120, false, true}, // ForswornFaction
    {0x0001CE17, Role::Bandit, 125, false, false}, // EncClassBanditMelee
    {0x00015BE7, Role::Bandit, 125, false, false}, // EncClassBanditMissile
    {0x00043BCB, Role::Bandit, 125, false, false}, // EncClassForsworn
    {0x00043BCD, Role::Bandit, 125, false, false}, // EncClassForswornMissile
    {0x00039D30, Role::Bandit, 125, true, false}, // EncClassBanditWizard
    {0x00043BCC, Role::Bandit, 125, true, false}, // EncClassForswornShaman
    {0x0005091E, Role::Mage, 15, true, true}, // JobCourtWizardFaction
    {0x00050921, Role::Mage, 130, true, true}, // JobSpellFaction
    {0x00026724, Role::Mage, 130, true, true}, // WarlockFaction
    {0x0001325A, Role::Mage, 130, true, false}, // VendorSpells
    {0x00048362, Role::Adventurer, 150, false, true}, // CompanionsFaction
    {0x00105D14, Role::Adventurer, 150, false, true}, // WEUnaggressiveAdventurerFaction
    {0x00105D13, Role::Adventurer, 150, false, true}, // WEAggressiveAdventurerFaction
    {0x000BC09E, Role::Adventurer, 150, false, true}, // WEBountyHunter
    {0x0006CD5D, Role::Adventurer, 150, false, true}, // dunIronbindAdventurerFaction
    {0x00029DA9, Role::Thief, 190, false, true}, // ThievesGuildFaction
    {0x0001F259, Role::Mage, 190, true, true}, // CollegeofWinterholdFaction
    {0x0001BDB3, Role::Adventurer, 190, false, true}, // DarkBrotherhoodFaction
    {0x000253F2, Role::Guard, 10, false, false}, // GuardImperial
    {0x000253F3, Role::Guard, 10, false, false}, // GuardSonsSkyrim
    {0x0002A477, Role::Guard, 10, false, false}, // GuardOrc1H
    {0x0010E714, Role::Guard, 10, false, false}, // GuardOrc2H
    {0x0001327F, Role::Guard, 10, false, false}, // SoldierImperialNotGuard
    {0x00013280, Role::Guard, 10, false, false}, // SoldierSonsSkyrimNotGuard
    {0x0010B1D8, Role::Guard, 10, false, false}, // CWSoldierClass
    {0x0001325E, Role::Guard, 10, false, false}, // Jailor
    {0x0010F7F9, Role::Adventurer, 210, false, false}, // NPCclassBelrand
    {0x000FB0DA, Role::Adventurer, 210, false, false}, // CombatNightingale
    {0x000C2914, Role::Mage, 200, true, false}, // TrainerRestorationJourneyman
    {0x000B812C, Role::Mage, 200, true, false}, // TrainerIllusionExpert
    {0x00042DC6, Role::Mage, 200, true, false}, // TrainerIllusionMaster
    {0x000E3A78, Role::Mage, 200, true, false}, // TrainerConjurationExpert
    {0x000E3A77, Role::Mage, 200, true, false}, // TrainerEnchantingMaster
    {0x000E3A76, Role::Mage, 200, true, false}, // TrainerEnchantingExpert
    {0x000E3A75, Role::Mage, 200, true, false}, // TrainerRestorationMaster
    {0x000E3A74, Role::Mage, 200, true, false}, // TrainerRestorationExpert
    {0x000E3A73, Role::Mage, 200, true, false}, // TrainerDestructionMaster
    {0x000E3A72, Role::Mage, 200, true, false}, // TrainerConjurationJourneyman
    {0x000E3A71, Role::Mage, 200, true, false}, // TrainerAlterationExpert
    {0x000E3A6A, Role::Mage, 200, true, false}, // TrainerConjurationMaster
    {0x000E3A69, Role::Mage, 200, true, false}, // TrainerAlterationMaster
    {0x000E3A5C, Role::Mage, 200, true, false}, // TrainerDestructionExpert
    {0x000E2FCD, Role::Mage, 200, true, false}, // TrainerDestructionJourneyman
    {0x000C969F, Role::Mage, 200, true, false}, // CombatMageNecro
    {0x0007D98D, Role::Adventurer, 210, false, false}, // EncClassPenitusOculatus
    {0x0007289D, Role::Adventurer, 210, false, false}, // EncClassThalmorMelee
    {0x00072891, Role::Adventurer, 210, false, false}, // EncClassThalmorMissile
    {0x0006766C, Role::Adventurer, 210, false, false}, // EncClassAlikrMissile
    {0x0006766B, Role::Adventurer, 210, false, false}, // EncClassAlikrMelee
    {0x0001E7D0, Role::Mage, 200, true, false}, // CombatMageDestruction
    {0x0001CE16, Role::Adventurer, 210, false, false}, // CombatBarbarian
    {0x0001CE15, Role::Adventurer, 210, false, false}, // CombatWarrior2H
    {0x0001CE14, Role::Mage, 200, true, false}, // CombatMageConjurer
    {0x00013182, Role::Adventurer, 210, false, false}, // CombatMonk
    {0x00013181, Role::Adventurer, 210, false, false}, // CombatRanger
    {0x00013180, Role::Adventurer, 210, false, false}, // CombatRogue
    {0x0001317F, Role::Adventurer, 210, false, false}, // CombatAssassin
    {0x0001317E, Role::Adventurer, 210, false, false}, // CombatThief
    {0x0001317D, Role::Adventurer, 210, false, false}, // CombatScout
    {0x0001317C, Role::Mage, 200, true, false}, // CombatNightblade
    {0x0001317B, Role::Mage, 200, true, false}, // CombatMystic
    {0x0001317A, Role::Mage, 200, true, false}, // CombatMageElemental
    {0x00013179, Role::Mage, 200, true, false}, // CombatSorcerer
    {0x00013178, Role::Mage, 200, true, false}, // CombatWitchblade
    {0x00013177, Role::Mage, 200, true, false}, // CombatSpellsword
    {0x00013176, Role::Adventurer, 210, false, false}, // CombatWarrior1H
};
inline constexpr PoolEntry pools[] = {
    {0x00065C97, Category::Food, Wealth::Destitute, true, true}, // FoodBread01A
    {0x00065C98, Category::Food, Wealth::Destitute, true, true}, // FoodBread01B
    {0x00064B2E, Category::Food, Wealth::Destitute, true, false}, // FoodApple
    {0x00064B2F, Category::Food, Wealth::Destitute, true, false}, // FoodApple02
    {0x00064B40, Category::Food, Wealth::Destitute, true, false}, // FoodCarrot
    {0x00064B41, Category::Food, Wealth::Destitute, true, false}, // FoodPotato
    {0x00064B31, Category::Food, Wealth::Destitute, true, false}, // FoodCheeseWedge01
    {0x00064B32, Category::Food, Wealth::Destitute, true, false}, // FoodCheeseWedge02
    {0x000721E8, Category::Food, Wealth::Destitute, true, false}, // FoodBeefCooked
    {0x000722BD, Category::Food, Wealth::Destitute, true, false}, // FoodVenisonCooked
    {0x00064B3B, Category::Food, Wealth::Destitute, true, false}, // FoodSalmonCooked
    {0x00034C5E, Category::Food, Wealth::Destitute, false, false}, // Ale
    {0x00034C5D, Category::Food, Wealth::Destitute, false, false}, // FoodMead
    {0x0003133C, Category::Food, Wealth::Destitute, false, false}, // FoodWineBottle02
    {0x0003133B, Category::Food, Wealth::Destitute, false, false}, // FoodWineAlto
    {0x000508CA, Category::Food, Wealth::Destitute, false, false}, // FoodHonningbrewMead
    {0x0002C35A, Category::Food, Wealth::Destitute, false, false}, // FoodBlackBriarMead
    {0x00085368, Category::Food, Wealth::Comfortable, false, false}, // FoodSolitudeSpicedWine
    {0x000F693F, Category::Food, Wealth::Comfortable, false, false}, // FoodBlackBriarMeadPrivateReserve
    {0x0009171B, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryNecklaceSilver
    {0x00087835, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryNecklaceGoldGems
    {0x00087834, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryNecklaceSilverGems
    {0x00087833, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryNecklaceSilverEmerald
    {0x00087832, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryNecklaceSilverSapphire
    {0x000877F1, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryNecklaceGoldDiamond
    {0x000877DC, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryNecklaceGoldRuby
    {0x000877D5, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryNecklaceGold
    {0x000877CB, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryRingGoldSapphire
    {0x000877CA, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryRingGoldEmerald
    {0x000877C9, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryRingGoldDiamond
    {0x000877B1, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryRingSilverRuby
    {0x000877AB, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryRingSilverGarnet
    {0x000877A7, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryRingSilverAmethyst
    {0x0003B97C, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryRingSilver
    {0x0001CF2B, Category::Jewelry, Wealth::Destitute, true, true}, // JewelryRingGold
    {0x00068523, Category::Gem, Wealth::Destitute, false, false}, // GemSapphireFlawless
    {0x00068522, Category::Gem, Wealth::Destitute, false, false}, // gemRubyFlawless
    {0x00068521, Category::Gem, Wealth::Destitute, false, false}, // GemGarnetFlawless
    {0x00068520, Category::Gem, Wealth::Destitute, false, false}, // GemEmeraldFlawless
    {0x0006851F, Category::Gem, Wealth::Destitute, false, false}, // GemDiamondFlawless
    {0x0006851E, Category::Gem, Wealth::Destitute, false, false}, // GemAmethystFlawless
    {0x00063B47, Category::Gem, Wealth::Destitute, false, false}, // GemDiamond
    {0x00063B46, Category::Gem, Wealth::Destitute, false, false}, // GemAmethyst
    {0x00063B45, Category::Gem, Wealth::Destitute, false, false}, // GemGarnet
    {0x00063B44, Category::Gem, Wealth::Destitute, false, false}, // GemSapphire
    {0x00063B43, Category::Gem, Wealth::Destitute, false, false}, // GemEmerald
    {0x00063B42, Category::Gem, Wealth::Destitute, false, false}, // GemRuby
    {0x0003EADD, Category::Potion, Wealth::Destitute, false, false}, // RestoreHealth01
    {0x0003EAE0, Category::Potion, Wealth::Destitute, false, false}, // RestoreMagicka01
    {0x0003EAE5, Category::Potion, Wealth::Destitute, false, false}, // RestoreStamina01
    {0x0003EADE, Category::Potion, Wealth::Poor, false, false}, // RestoreHealth02
    {0x0003EAE1, Category::Potion, Wealth::Poor, false, false}, // RestoreMagicka02
    {0x00039BE8, Category::Potion, Wealth::Poor, false, false}, // RestoreStamina02
    {0x0003EADF, Category::Potion, Wealth::Common, false, false}, // RestoreHealth03
    {0x0003EAE2, Category::Potion, Wealth::Common, false, false}, // RestoreMagicka03
    {0x0003EAE7, Category::Potion, Wealth::Common, false, false}, // RestoreStamina03
    {0x0003EAE3, Category::Potion, Wealth::Comfortable, false, false}, // RestoreHealth04
    {0x0003EAE4, Category::Potion, Wealth::Comfortable, false, false}, // RestoreMagicka04
    {0x0003EAE8, Category::Potion, Wealth::Comfortable, false, false}, // RestoreStamina04
    {0x00077E1C, Category::Ingredient, Wealth::Destitute, true, false}, // MountainFlower01Blue
    {0x00077E1D, Category::Ingredient, Wealth::Destitute, true, false}, // MountainFlower01Red
    {0x00077E1E, Category::Ingredient, Wealth::Destitute, true, false}, // MountainFlower01Purple
    {0x00045C28, Category::Ingredient, Wealth::Destitute, false, false}, // Lavender
    {0x0003F7F8, Category::Ingredient, Wealth::Destitute, false, false}, // TundraCotton
    {0x000134AA, Category::Ingredient, Wealth::Destitute, false, false}, // Thistle01
    {0x00034CDF, Category::Ingredient, Wealth::Destitute, false, false}, // SaltPile
    {0x00034D22, Category::Ingredient, Wealth::Destitute, false, false}, // Garlic
    {0x0001B3BD, Category::Ingredient, Wealth::Destitute, false, false}, // Snowberry
    {0x00034D32, Category::Ingredient, Wealth::Destitute, false, false}, // FrostMirriam
    {0x00034D31, Category::Ingredient, Wealth::Destitute, false, false}, // ElvesEar
    {0x00034CDD, Category::Ingredient, Wealth::Destitute, false, false}, // BoneMeal
    {0x0009659B, Category::Magic, Wealth::Destitute, false, false}, // CandlelightScroll
    {0x0009659C, Category::Magic, Wealth::Destitute, false, false}, // OakfleshScroll
    {0x00096598, Category::Magic, Wealth::Destitute, false, false}, // FireboltScroll
    {0x00096599, Category::Magic, Wealth::Destitute, false, false}, // IceSpikeScroll
    {0x0009659A, Category::Magic, Wealth::Destitute, false, false}, // LightningBoltScroll
    {0x0002E4FF, Category::Magic, Wealth::Wealthy, false, false}, // SoulGemGrandFilled
    {0x0002E4FC, Category::Magic, Wealth::Wealthy, false, false}, // SoulGemGrand
    {0x0002E4FB, Category::Magic, Wealth::Wealthy, false, false}, // SoulGemGreaterFilled
    {0x0002E4F4, Category::Magic, Wealth::Wealthy, false, false}, // SoulGemGreater
    {0x0002E4F3, Category::Magic, Wealth::Common, false, false}, // SoulGemCommonFilled
    {0x0002E4E6, Category::Magic, Wealth::Common, false, false}, // SoulGemCommon
    {0x0002E4E5, Category::Magic, Wealth::Destitute, false, false}, // SoulGemLesserFilled
    {0x0002E4E4, Category::Magic, Wealth::Destitute, false, false}, // SoulGemLesser
    {0x0002E4E3, Category::Magic, Wealth::Destitute, false, false}, // SoulGemPettyFilled
    {0x0002E4E2, Category::Magic, Wealth::Destitute, false, false}, // SoulGemPetty
    {0x0000000A, Category::Lockpick, Wealth::Destitute, false, false}, // Lockpick
};
inline constexpr std::uint32_t races[] = {
    0x00013740, // ArgonianRace
    0x0008883A, // ArgonianRaceVampire
    0x00013741, // BretonRace
    0x0002C65C, // BretonRaceChild
    0x00108272, // BretonRaceChildVampire
    0x0008883C, // BretonRaceVampire
    0x00013742, // DarkElfRace
    0x0008883D, // DarkElfRaceVampire
    0x00067CD8, // ElderRace
    0x000A82BA, // ElderRaceVampire
    0x00013743, // HighElfRace
    0x00088840, // HighElfRaceVampire
    0x00013744, // ImperialRace
    0x0002C659, // ImperialRaceChild
    0x00088844, // ImperialRaceVampire
    0x00013745, // KhajiitRace
    0x00088845, // KhajiitRaceVampire
    0x00013746, // NordRace
    0x0002C65B, // NordRaceChild
    0x00088794, // NordRaceVampire
    0x00013747, // OrcRace
    0x000A82B9, // OrcRaceVampire
    0x00013748, // RedguardRace
    0x0002C65A, // RedguardRaceChild
    0x00088846, // RedguardRaceVampire
    0x00013749, // WoodElfRace
    0x00088884, // WoodElfRaceVampire
};
}
