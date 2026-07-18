#include "TestSupport.h"

#include "../src/Combat/CombatFormulas.h"
#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Skills/SkillType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/Stats/StatType.h"

#include <cmath>
#include <sstream>
#include <string>

namespace
{
    void ExpectNearDouble(
        TestContext &test,
        double actual,
        double expected,
        double tolerance,
        const std::string &message)
    {
        std::ostringstream stream;
        stream << message
               << " | expected: " << expected
               << " actual: " << actual;

        test.Expect(
            std::fabs(actual - expected) <= tolerance,
            stream.str());
    }

    void AdvanceSkillToLevel(
        Player &player,
        SkillType skillType,
        int targetLevel)
    {
        while (player.GetSkills().GetSkill(skillType).GetLevel() < targetLevel)
        {
            const Skill &skill = player.GetSkills().GetSkill(skillType);
            int xpNeeded = skill.GetXPForNextLevel() - skill.GetXP();
            player.GetSkills().AddXP(
                skillType,
                xpNeeded);
        }
    }
}

int main()
{
    TestContext test;

    {
        CombatRatings ratings;
        ratings.attackAccuracy = 7;
        ratings.defence = 9;
        ratings.meleeStrength = 15;

        test.ExpectEqual(
            CombatFormulas::CalculateAttackRoll(ratings),
            7,
            "Positive attack accuracy passes through unchanged");
        test.ExpectEqual(
            CombatFormulas::CalculateDefenceRoll(ratings),
            9,
            "Positive defence passes through unchanged");
        test.ExpectEqual(
            CombatFormulas::CalculateMaximumHit(ratings),
            2,
            "Strength 15 produces maximum hit 2");
    }

    {
        CombatRatings ratings;
        ratings.attackAccuracy = 0;
        ratings.defence = 0;
        ratings.meleeStrength = 0;

        test.ExpectEqual(
            CombatFormulas::CalculateAttackRoll(ratings),
            1,
            "Zero attack accuracy clamps to one");
        test.ExpectEqual(
            CombatFormulas::CalculateDefenceRoll(ratings),
            1,
            "Zero defence clamps to one");
        test.ExpectEqual(
            CombatFormulas::CalculateMaximumHit(ratings),
            1,
            "Zero strength clamps maximum hit to one");
    }

    {
        CombatRatings ratings;
        ratings.attackAccuracy = -12;
        ratings.defence = -4;
        ratings.meleeStrength = -30;

        test.ExpectEqual(
            CombatFormulas::CalculateAttackRoll(ratings),
            1,
            "Negative attack accuracy clamps to one");
        test.ExpectEqual(
            CombatFormulas::CalculateDefenceRoll(ratings),
            1,
            "Negative defence clamps to one");
        test.ExpectEqual(
            CombatFormulas::CalculateMaximumHit(ratings),
            1,
            "Negative strength clamps maximum hit to one");
    }

    {
        CombatRatings baseRatings;

        test.ExpectEqual(
            CombatFormulas::CalculateMaximumHit(baseRatings),
            1,
            "Base strength produces minimum maximum hit");

        baseRatings.meleeStrength = 9;
        test.ExpectEqual(
            CombatFormulas::CalculateMaximumHit(baseRatings),
            1,
            "Strength below 10 still produces maximum hit 1");

        baseRatings.meleeStrength = 10;
        test.ExpectEqual(
            CombatFormulas::CalculateMaximumHit(baseRatings),
            2,
            "Strength 10 produces maximum hit 2");

        baseRatings.meleeStrength = 19;
        test.ExpectEqual(
            CombatFormulas::CalculateMaximumHit(baseRatings),
            2,
            "Strength 19 still produces maximum hit 2");

        baseRatings.meleeStrength = 20;
        test.ExpectEqual(
            CombatFormulas::CalculateMaximumHit(baseRatings),
            3,
            "Strength 20 produces maximum hit 3");
    }

    {
        test.ExpectEqual(
            CombatFormulas::ResolveHit(8, 7),
            true,
            "Greater attacker result hits");
        test.ExpectEqual(
            CombatFormulas::ResolveHit(7, 7),
            false,
            "Equal results miss");
        test.ExpectEqual(
            CombatFormulas::ResolveHit(6, 7),
            false,
            "Lower attacker result misses");

        test.ExpectEqual(
            CombatFormulas::ClampDamageRoll(-4, 8),
            0,
            "Negative damage clamps to zero");
        test.ExpectEqual(
            CombatFormulas::ClampDamageRoll(12, 8),
            8,
            "Damage above maximum clamps to maximum");
        test.ExpectEqual(
            CombatFormulas::ClampDamageRoll(1, 0),
            1,
            "Invalid maximum hit is treated as at least one");
    }

    {
        double equalChance = CombatFormulas::CalculateHitChance(10, 10);
        double higherAttackChance = CombatFormulas::CalculateHitChance(12, 10);
        double higherDefenceChance = CombatFormulas::CalculateHitChance(10, 12);

        test.Expect(
            equalChance >= 0.0 && equalChance <= 1.0,
            "Equal-roll hit chance stays within bounds");
        test.Expect(
            higherAttackChance >= 0.0 && higherAttackChance <= 1.0,
            "Higher-attack hit chance stays within bounds");
        test.Expect(
            higherDefenceChance >= 0.0 && higherDefenceChance <= 1.0,
            "Higher-defence hit chance stays within bounds");

        ExpectNearDouble(
            test,
            equalChance,
            10.0 / 22.0,
            1e-12,
            "Equal rolls use the expected chance formula");
        ExpectNearDouble(
            test,
            higherAttackChance,
            1.0 - ((10.0 + 2.0) / (2.0 * (12.0 + 1.0))),
            1e-12,
            "Higher attack uses the expected chance formula");
        ExpectNearDouble(
            test,
            higherDefenceChance,
            10.0 / (2.0 * (12.0 + 1.0)),
            1e-12,
            "Higher defence uses the expected chance formula");

        test.Expect(
            higherAttackChance > equalChance,
            "Higher attack increases hit chance");
        test.Expect(
            higherDefenceChance < equalChance,
            "Higher defence decreases hit chance");

        double largeChance = CombatFormulas::CalculateHitChance(
            1000000,
            1);
        double clampedChance = CombatFormulas::CalculateHitChance(
            0,
            -1000);

        test.Expect(
            largeChance >= 0.0 && largeChance <= 1.0,
            "Very large attack values remain bounded");
        test.Expect(
            clampedChance >= 0.0 && clampedChance <= 1.0,
            "Zero and negative inputs are safely clamped");
        ExpectNearDouble(
            test,
            clampedChance,
            CombatFormulas::CalculateHitChance(1, 1),
            1e-12,
            "Clamped zero and negative inputs behave like minimum rolls");
    }

    {
        CombatRatings attacker;
        attacker.attackAccuracy = 8;
        attacker.meleeStrength = 14;
        attacker.defence = 3;

        CombatRatings defender;
        defender.defence = 11;

        test.ExpectEqual(
            CombatFormulas::CalculateHitChance(attacker, defender),
            CombatFormulas::CalculateHitChance(
                CombatFormulas::CalculateAttackRoll(attacker),
                CombatFormulas::CalculateDefenceRoll(defender)),
            "Ratings overload delegates to roll-based hit chance");
    }

    {
        Player player(2001);

        CombatRatings ratings = player.GetCombatRatings();
        CombatFormulas::MeleeCombatProfile profile = player.GetMeleeCombatProfile();
        CombatFormulas::MeleeCombatProfile expectedProfile =
            CombatFormulas::BuildMeleeCombatProfile(ratings);

        test.ExpectEqual(
            profile.attackRoll,
            expectedProfile.attackRoll,
            "Player profile attack roll matches combat formulas");
        test.ExpectEqual(
            profile.defenceRoll,
            expectedProfile.defenceRoll,
            "Player profile defence roll matches combat formulas");
        test.ExpectEqual(
            profile.maximumHit,
            expectedProfile.maximumHit,
            "Player profile maximum hit matches combat formulas");

        AdvanceSkillToLevel(
            player,
            SkillType::ATTACK,
            10);

        CombatFormulas::MeleeCombatProfile trainedProfile =
            player.GetMeleeCombatProfile();

        test.ExpectEqual(
            player.GetCombatRatings().attackAccuracy,
            10,
            "Attack XP raises attack accuracy to the expected level");
        test.ExpectEqual(
            trainedProfile.attackRoll,
            10,
            "Attack XP changes the derived attack roll");
        test.ExpectEqual(
            trainedProfile.maximumHit,
            2,
            "Attack XP changes the derived maximum hit");

        AdvanceSkillToLevel(
            player,
            SkillType::DEFENCE,
            5);

        test.ExpectEqual(
            player.GetMeleeCombatProfile().defenceRoll,
            5,
            "Defence XP changes the derived defence roll");
    }

    {
        Player thresholdPlayer(2002);

        AdvanceSkillToLevel(
            thresholdPlayer,
            SkillType::ATTACK,
            6);

        CombatFormulas::MeleeCombatProfile beforeSword =
            thresholdPlayer.GetMeleeCombatProfile();

        test.ExpectEqual(
            beforeSword.attackRoll,
            6,
            "A level 6 attacker has attack roll 6 before weapon bonuses");
        test.ExpectEqual(
            beforeSword.maximumHit,
            1,
            "A level 6 attacker starts with maximum hit 1");

        thresholdPlayer.GetEquipment().Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_SWORD);

        CombatFormulas::MeleeCombatProfile swordProfile =
            thresholdPlayer.GetMeleeCombatProfile();

        test.ExpectEqual(
            swordProfile.attackRoll,
            9,
            "Bronze sword attack bonus flows through Player ratings");
        test.ExpectEqual(
            swordProfile.maximumHit,
            2,
            "Bronze sword strength bonus crosses the 10-point threshold");

        thresholdPlayer.GetEquipment().Unequip(
            EquipmentSlotType::WEAPON);

        CombatFormulas::MeleeCombatProfile noSword =
            thresholdPlayer.GetMeleeCombatProfile();

        test.ExpectEqual(
            noSword.attackRoll,
            beforeSword.attackRoll,
            "Removing the bronze sword restores the previous attack roll");
        test.ExpectEqual(
            noSword.maximumHit,
            beforeSword.maximumHit,
            "Removing the bronze sword restores the previous maximum hit");

        thresholdPlayer.GetEquipment().Equip(
            EquipmentSlotType::SHIELD,
            ItemType::WOODEN_SHIELD);

        CombatFormulas::MeleeCombatProfile shieldProfile =
            thresholdPlayer.GetMeleeCombatProfile();

        test.ExpectEqual(
            shieldProfile.defenceRoll,
            4,
            "Wooden shield defence bonus flows through Player ratings");
        test.ExpectEqual(
            shieldProfile.attackRoll,
            noSword.attackRoll,
            "Wooden shield does not change attacker strength or attack roll");
        test.ExpectEqual(
            shieldProfile.maximumHit,
            noSword.maximumHit,
            "Wooden shield does not change attacker strength or maximum hit");
    }

    {
        Player developmentPlayer(2003);

        developmentPlayer.GetInventory().AddItem(
            ItemType::DEVELOPER_GODSWORD,
            1);

        int godswordSlot = -1;
        const auto &slots = developmentPlayer.GetInventory().GetSlots();

        for (int slotIndex = 0;
             slotIndex < static_cast<int>(slots.size());
             ++slotIndex)
        {
            if (!slots[slotIndex].IsEmpty() &&
                slots[slotIndex].GetItemType() == ItemType::DEVELOPER_GODSWORD)
            {
                godswordSlot = slotIndex;
                break;
            }
        }

        test.Expect(
            godswordSlot >= 0,
            "Developer Godsword is available for the formula path test");

        if (godswordSlot >= 0)
        {
            Equipment &equipment = developmentPlayer.GetEquipment();

            equipment.Equip(
                EquipmentSlotType::WEAPON,
                ItemType::DEVELOPER_GODSWORD);

            CombatFormulas::MeleeCombatProfile developmentProfile =
                developmentPlayer.GetMeleeCombatProfile();

            test.ExpectEqual(
                developmentProfile.attackRoll,
                151,
                "Developer Godsword raises the attack roll through the normal rating path");
            test.ExpectEqual(
                developmentProfile.maximumHit,
                15,
                "Developer Godsword raises the maximum hit through the normal rating path");
        }
    }

    return test.Finish();
}