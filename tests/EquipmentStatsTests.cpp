#include "TestSupport.h"

#include "../src/Equipment/Equipment.h"
#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Combat/CombatFormulas.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Skills/SkillType.h"
#include "../src/Stats/StatBlock.h"
#include "../src/Stats/StatType.h"
#include "../src/World/World.h"

#include <memory>

#include <array>

namespace
{
    constexpr std::array<StatType, 4> CoreStats{{
        StatType::ATTACK_ACCURACY,
        StatType::MELEE_STRENGTH,
        StatType::DEFENCE,
        StatType::MAX_HEALTH,
    }};
}

int main()
{
    TestContext test;

    {
        StatBlock stats;

        for (StatType stat : CoreStats)
        {
            test.ExpectEqual(
                stats.Get(stat),
                0,
                "StatBlock defaults each stat to zero");
        }
    }

    {
        StatBlock stats;

        stats.Set(
            StatType::ATTACK_ACCURACY,
            6);

        stats.Add(
            StatType::ATTACK_ACCURACY,
            4);

        stats.Set(
            StatType::DEFENCE,
            -2);

        test.ExpectEqual(
            stats.Get(StatType::ATTACK_ACCURACY),
            10,
            "StatBlock set and add update the expected stat");
        test.ExpectEqual(
            stats.Get(StatType::DEFENCE),
            -2,
            "StatBlock supports negative bonuses");
        test.ExpectEqual(
            stats.Get(StatType::MELEE_STRENGTH),
            0,
            "StatBlock updates are isolated per stat");
    }

    {
        StatBlock left;
        left.Set(
            StatType::ATTACK_ACCURACY,
            2);
        left.Set(
            StatType::DEFENCE,
            5);

        StatBlock right;
        right.Set(
            StatType::ATTACK_ACCURACY,
            4);
        right.Set(
            StatType::DEFENCE,
            -1);
        right.Set(
            StatType::MAX_HEALTH,
            8);

        StatBlock combined = left + right;

        test.ExpectEqual(
            combined.Get(StatType::ATTACK_ACCURACY),
            6,
            "Combining stat blocks sums attack accuracy");
        test.ExpectEqual(
            combined.Get(StatType::DEFENCE),
            4,
            "Combining stat blocks sums defence including negatives");
        test.ExpectEqual(
            combined.Get(StatType::MAX_HEALTH),
            8,
            "Combining stat blocks includes independent stats");
    }

    {
        StatBlock stats;
        StatType invalidStat =
            static_cast<StatType>(999);

        stats.Set(
            invalidStat,
            77);
        stats.Add(
            invalidStat,
            33);

        test.ExpectEqual(
            stats.Get(invalidStat),
            0,
            "Invalid stat access is handled safely");
        test.ExpectEqual(
            stats.Get(StatType::ATTACK_ACCURACY),
            0,
            "Invalid stat writes do not corrupt supported stats");
    }

    {
        Equipment equipment;
        StatBlock bonuses =
            equipment.GetTotalStatBonuses();

        for (StatType stat : CoreStats)
        {
            test.ExpectEqual(
                bonuses.Get(stat),
                0,
                "Empty equipment produces zero bonuses");
        }
    }

    {
        Equipment equipment;

        bool equipped =
            equipment.Equip(
                EquipmentSlotType::WEAPON,
                ItemType::BRONZE_SWORD);

        test.Expect(
            equipped,
            "Equipping a stat item succeeds on an empty slot");

        StatBlock bonuses =
            equipment.GetTotalStatBonuses();

        test.ExpectEqual(
            bonuses.Get(StatType::ATTACK_ACCURACY),
            3,
            "Equipping one item adds its attack accuracy bonus");
        test.ExpectEqual(
            bonuses.Get(StatType::MELEE_STRENGTH),
            4,
            "Equipping one item adds its melee strength bonus");
    }

    {
        Equipment equipment;

        equipment.Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_SWORD);
        equipment.Equip(
            EquipmentSlotType::SHIELD,
            ItemType::WOODEN_SHIELD);

        StatBlock bonuses =
            equipment.GetTotalStatBonuses();

        test.ExpectEqual(
            bonuses.Get(StatType::ATTACK_ACCURACY),
            3,
            "Multiple equipped items combine attack bonuses");
        test.ExpectEqual(
            bonuses.Get(StatType::MELEE_STRENGTH),
            4,
            "Multiple equipped items combine melee strength bonuses");
        test.ExpectEqual(
            bonuses.Get(StatType::DEFENCE),
            3,
            "Multiple equipped items combine defence bonuses");
        test.ExpectEqual(
            bonuses.Get(StatType::MAX_HEALTH),
            5,
            "Multiple equipped items combine max health bonuses");
    }

    {
        const ItemDefinition &developerGodsword =
            ItemDatabase::Get(ItemType::DEVELOPER_GODSWORD);

        test.ExpectEqual(
            static_cast<int>(developerGodsword.GetItemType()),
            static_cast<int>(ItemType::DEVELOPER_GODSWORD),
            "Developer Godsword exists in ItemDatabase");
        test.ExpectEqual(
            developerGodsword.GetName(),
            std::string("Developer Godsword"),
            "Developer Godsword has the expected name");
        test.Expect(
            developerGodsword.IsEquippable(),
            "Developer Godsword is equippable");
        test.Expect(
            !developerGodsword.IsStackable(),
            "Developer Godsword is non-stackable");
        test.ExpectEqual(
            static_cast<int>(developerGodsword.GetEquipmentSlot()),
            static_cast<int>(EquipmentSlotType::WEAPON),
            "Developer Godsword equips in the weapon slot");
        test.ExpectEqual(
            static_cast<int>(developerGodsword.GetToolType()),
            static_cast<int>(ToolType::NONE),
            "Developer Godsword has no gathering-tool behavior");
        test.ExpectEqual(
            static_cast<int>(developerGodsword.GetRequiredSkill()),
            static_cast<int>(SkillType::NONE),
            "Developer Godsword has no skill requirement");
        test.ExpectEqual(
            developerGodsword.GetRequiredSkillLevel(),
            0,
            "Developer Godsword has no skill level requirement");
        test.ExpectEqual(
            developerGodsword.GetActionDurationTicks(),
            0,
            "Developer Godsword has no action duration");
        test.ExpectEqual(
            developerGodsword.GetEquipmentStatBonuses().Get(
                StatType::ATTACK_ACCURACY),
            150,
            "Developer Godsword grants +150 attack accuracy");
        test.ExpectEqual(
            developerGodsword.GetEquipmentStatBonuses().Get(
                StatType::MELEE_STRENGTH),
            140,
            "Developer Godsword grants +140 melee strength");
        test.ExpectEqual(
            developerGodsword.GetEquipmentStatBonuses().Get(
                StatType::DEFENCE),
            0,
            "Developer Godsword grants no defence");
        test.ExpectEqual(
            developerGodsword.GetEquipmentStatBonuses().Get(
                StatType::MAX_HEALTH),
            0,
            "Developer Godsword grants no max-health bonus");
    }

    {
        Equipment equipment;

        equipment.Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_SWORD);

        equipment.Unequip(
            EquipmentSlotType::WEAPON);

        equipment.Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_PICKAXE);

        StatBlock bonuses =
            equipment.GetTotalStatBonuses();

        test.ExpectEqual(
            bonuses.Get(StatType::ATTACK_ACCURACY),
            0,
            "Unequipping and replacing updates total bonuses");
        test.ExpectEqual(
            bonuses.Get(StatType::MELEE_STRENGTH),
            0,
            "Replacing with a non-combat tool removes previous bonuses");
    }

    {
        Equipment equipment;

        bool equippedNone =
            equipment.Equip(
                EquipmentSlotType::WEAPON,
                ItemType::NONE);

        test.Expect(
            !equippedNone,
            "ItemType::NONE cannot be equipped");

        StatBlock bonuses =
            equipment.GetTotalStatBonuses();

        for (StatType stat : CoreStats)
        {
            test.ExpectEqual(
                bonuses.Get(stat),
                0,
                "ItemType::NONE is safely ignored by aggregation");
        }
    }

    {
        Player player(101);

        player.GetEquipment().Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_SWORD);
        player.GetEquipment().Equip(
            EquipmentSlotType::SHIELD,
            ItemType::WOODEN_SHIELD);

        for (StatType stat : CoreStats)
        {
            test.ExpectEqual(
                player.GetTotalStat(stat),
                player.GetBaseStat(stat) +
                    player.GetEquipmentBonus(stat),
                "Player total stat equals base plus equipment bonuses");
        }

        player.GetEquipment().Unequip(
            EquipmentSlotType::WEAPON);
        player.GetEquipment().Unequip(
            EquipmentSlotType::SHIELD);

        for (StatType stat : CoreStats)
        {
            test.ExpectEqual(
                player.GetTotalStat(stat),
                player.GetBaseStat(stat),
                "Removing equipment returns player totals to base values");
        }

        {
            World world;

            int playerID = world.CreatePlayer();
            Player *player = dynamic_cast<Player *>(
                world.GetEntityByID(playerID));

            test.Expect(
                player != nullptr,
                "World creates a player for the equipment swap test");

            if (player != nullptr)
            {
                player->GetInventory().AddItem(
                    ItemType::BRONZE_SWORD,
                    1);
                player->GetInventory().AddItem(
                    ItemType::DEVELOPER_GODSWORD,
                    1);

                int bronzeSlot = -1;
                int godswordSlot = -1;

                const auto &slots = player->GetInventory().GetSlots();

                for (int slotIndex = 0;
                     slotIndex < static_cast<int>(slots.size());
                     ++slotIndex)
                {
                    if (slots[slotIndex].IsEmpty())
                    {
                        continue;
                    }

                    if (slots[slotIndex].GetItemType() == ItemType::BRONZE_SWORD)
                    {
                        bronzeSlot = slotIndex;
                    }

                    if (slots[slotIndex].GetItemType() == ItemType::DEVELOPER_GODSWORD)
                    {
                        godswordSlot = slotIndex;
                    }
                }

                test.Expect(
                    bronzeSlot >= 0 && godswordSlot >= 0,
                    "Both swords are present for the replacement test");

                if (bronzeSlot >= 0 && godswordSlot >= 0)
                {
                    test.Expect(
                        world.TryEquipInventoryItem(
                            playerID,
                            bronzeSlot),
                        "Bronze sword equips through the normal inventory flow");

                    int attackBefore = player->GetCombatRatings().attackAccuracy;
                    int strengthBefore = player->GetCombatRatings().meleeStrength;

                    test.Expect(
                        world.TryEquipInventoryItem(
                            playerID,
                            godswordSlot),
                        "Developer Godsword replaces the currently equipped weapon");

                    test.ExpectEqual(
                        static_cast<int>(
                            player->GetEquipment().GetEquippedItem(
                                EquipmentSlotType::WEAPON)),
                        static_cast<int>(ItemType::DEVELOPER_GODSWORD),
                        "Developer Godsword ends up equipped in the weapon slot");
                    test.ExpectEqual(
                        player->GetInventory().GetItemAmount(
                            ItemType::BRONZE_SWORD),
                        1,
                        "Replacing the bronze sword returns it to inventory");
                    test.ExpectEqual(
                        player->GetCombatRatings().attackAccuracy,
                        attackBefore + 150 - 3,
                        "Developer Godsword increases attack accuracy by the expected amount");
                    test.ExpectEqual(
                        player->GetCombatRatings().meleeStrength,
                        strengthBefore + 140 - 4,
                        "Developer Godsword increases melee strength by the expected amount");
                    test.ExpectEqual(
                        player->GetCombatRatings().defence,
                        player->GetBaseStat(StatType::DEFENCE),
                        "Weapon swapping does not affect defence");
                    test.ExpectEqual(
                        player->GetCombatRatings().maximumHealth,
                        player->GetBaseStat(StatType::MAX_HEALTH),
                        "Weapon swapping does not affect maximum health");
                }
            }
        }
    }

    {
        Player player(202);

        int baseAccuracy =
            player.GetBaseStat(
                StatType::ATTACK_ACCURACY);
        int baseStrength =
            player.GetBaseStat(
                StatType::MELEE_STRENGTH);
        int baseDefence =
            player.GetBaseStat(
                StatType::DEFENCE);

        player.GetSkills().AddXP(
            SkillType::ATTACK,
            5000);
        player.GetSkills().AddXP(
            SkillType::DEFENCE,
            5000);

        test.Expect(
            player.GetBaseStat(
                StatType::ATTACK_ACCURACY) >
                baseAccuracy,
            "Attack skill level changes increase base attack accuracy");
        test.Expect(
            player.GetBaseStat(
                StatType::MELEE_STRENGTH) >
                baseStrength,
            "Attack skill level changes increase base melee strength");
        test.Expect(
            player.GetBaseStat(
                StatType::DEFENCE) >
                baseDefence,
            "Defence skill level changes increase base defence");
    }

    {
        Player player(303);

        int defenceBefore =
            player.GetTotalStat(
                StatType::DEFENCE);
        int healthBefore =
            player.GetTotalStat(
                StatType::MAX_HEALTH);

        player.GetEquipment().Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_SWORD);

        test.ExpectEqual(
            player.GetTotalStat(
                StatType::DEFENCE),
            defenceBefore,
            "Weapon equipment does not change unrelated defence stat");
        test.ExpectEqual(
            player.GetTotalStat(
                StatType::MAX_HEALTH),
            healthBefore,
            "Weapon equipment does not change unrelated max health stat");
    }

    return test.Finish();
}
