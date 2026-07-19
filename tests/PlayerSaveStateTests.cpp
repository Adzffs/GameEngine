#include "TestSupport.h"

#include "../src/Core/DevelopmentConfig.h"
#include "../src/Entity/Manager/EntityManager.h"
#include "../src/Inventory/InventorySlot.h"
#include "../src/Persistence/PlayerSaveState.h"
#include "../src/Player/Player.h"
#include "../src/StatusEffect/StatusEffectDefinition.h"

#include <array>
#include <algorithm>
#include <climits>
#include <memory>
#include <type_traits>

namespace
{
    PlayerSaveData MakeValidEmptySave()
    {
        Player player(1, PlayerInitializationMode::EMPTY);
        return PlayerSaveState::Capture(player);
    }

    int FindMinimumXPForLevel(int targetLevel)
    {
        Skill skill;
        int xp = 0;
        while (skill.GetLevel() < targetLevel)
        {
            skill.AddXP(1);
            ++xp;
        }
        return xp;
    }

    bool SameSave(const PlayerSaveData &left, const PlayerSaveData &right)
    {
        if (left.version != right.version ||
            left.positionX != right.positionX ||
            left.positionY != right.positionY ||
            left.currentHealth != right.currentHealth ||
            left.skills.size() != right.skills.size() ||
            left.equipment.size() != right.equipment.size())
        {
            return false;
        }
        for (int index = 0; index < Inventory::SlotCount; ++index)
        {
            if (left.inventorySlots[index].itemType != right.inventorySlots[index].itemType ||
                left.inventorySlots[index].quantity != right.inventorySlots[index].quantity)
            {
                return false;
            }
        }
        for (std::size_t index = 0; index < left.skills.size(); ++index)
        {
            if (left.skills[index].skillType != right.skills[index].skillType ||
                left.skills[index].xp != right.skills[index].xp)
            {
                return false;
            }
        }
        for (std::size_t index = 0; index < left.equipment.size(); ++index)
        {
            if (left.equipment[index].slotType != right.equipment[index].slotType ||
                left.equipment[index].itemType != right.equipment[index].itemType)
            {
                return false;
            }
        }
        return true;
    }

    void ExpectRejected(
        TestContext &test,
        const PlayerSaveData &saveData,
        PlayerSaveValidationCode code,
        const char *message)
    {
        PlayerSaveValidationReport report;
        std::unique_ptr<Player> player =
            PlayerSaveState::TryCreatePlayer(50, saveData, report);
        test.Expect(player == nullptr, message);
        test.Expect(report.Contains(code), std::string(message) + " reports expected code");
    }
}

int main()
{
    TestContext test;

    static_assert(std::is_same_v<decltype(PlayerSaveData::inventorySlots),
                                 std::array<SavedInventorySlot, Inventory::SlotCount>>);
    static_assert(std::is_copy_constructible_v<PlayerSaveData>);
    static_assert(std::is_nothrow_copy_assignable_v<InventorySlot>);
    static_assert(std::is_nothrow_copy_assignable_v<
                  std::array<InventorySlot, Inventory::SlotCount>>);

    {
        Player defaults(1);
        test.ExpectEqual(defaults.GetInventory().GetItemAmount(ItemType::BRONZE_AXE), 1,
                         "Default Player retains bronze axe");
        test.ExpectEqual(defaults.GetInventory().GetItemAmount(ItemType::BRONZE_PICKAXE), 1,
                         "Default Player retains bronze pickaxe");
        test.ExpectEqual(defaults.GetInventory().GetItemAmount(ItemType::IRON_PICKAXE), 1,
                         "Default Player retains iron pickaxe");
        test.ExpectEqual(defaults.GetInventory().GetItemAmount(ItemType::STEEL_PICKAXE), 1,
                         "Default Player retains steel pickaxe");
        test.ExpectEqual(
            defaults.GetSkills().GetSkill(SkillType::WOODCUTTING).GetXP(),
            DevelopmentConfig::TEST_WOODCUTTING_XP,
            "Default Player retains development Woodcutting XP");
        test.ExpectEqual(
            defaults.GetSkills().GetSkill(SkillType::MINING).GetXP(),
            DevelopmentConfig::TEST_MINING_XP,
            "Default Player retains development Mining XP");
        test.ExpectEqual(
            defaults.GetSkills().GetSkill(SkillType::SMITHING).GetXP(),
            DevelopmentConfig::TEST_SMITHING_XP,
            "Default Player retains development Smithing XP");

        const std::array<ItemType, 4> expectedStarterItems{
            ItemType::BRONZE_AXE,
            ItemType::BRONZE_PICKAXE,
            ItemType::IRON_PICKAXE,
            ItemType::STEEL_PICKAXE};
        const auto &defaultSlots = defaults.GetInventory().GetSlots();
        for (std::size_t index = 0; index < expectedStarterItems.size(); ++index)
        {
            test.Expect(defaultSlots[index].GetItemType() == expectedStarterItems[index] &&
                            defaultSlots[index].GetAmount() == 1,
                        "Default Player preserves exact starter slot ordering");
        }
        for (int index = static_cast<int>(expectedStarterItems.size());
             index < Inventory::SlotCount;
             ++index)
        {
            test.Expect(defaultSlots[index].IsEmpty() && defaultSlots[index].GetAmount() == 0,
                        "Default Player preserves canonical empty trailing slots");
        }
        test.ExpectEqual(defaults.GetSkills().GetSkill(SkillType::ATTACK).GetXP(), 0,
                         "Default Player retains zero Attack XP");
        test.ExpectEqual(defaults.GetSkills().GetSkill(SkillType::DEFENCE).GetXP(), 0,
                         "Default Player retains zero Defence XP");
        test.ExpectEqual(defaults.GetMaximumHealth(), 100,
                         "Default Player retains permanent maximum health");
        test.ExpectEqual(defaults.GetCurrentHealth(), 100,
                         "Default Player retains full current health");
        test.Expect(defaults.GetPosition().GetX() == 0 && defaults.GetPosition().GetY() == 0,
                    "Default Player retains origin position");

        EntityManager manager;
        int id = manager.CreatePlayer();
        const Player *managed = dynamic_cast<const Player *>(manager.GetEntityByID(id));
        test.Expect(managed != nullptr &&
                        managed->GetInventory().GetItemAmount(ItemType::BRONZE_AXE) == 1,
                    "EntityManager CreatePlayer retains development defaults");
        test.Expect(managed != nullptr &&
                        SameSave(PlayerSaveState::Capture(defaults),
                                 PlayerSaveState::Capture(*managed)),
                    "EntityManager Player has exact default persistent state");
    }

    {
        Player empty(2, PlayerInitializationMode::EMPTY);
        Player unknown(3, static_cast<PlayerInitializationMode>(999));
        for (const Player *player : {&empty, &unknown})
        {
            bool inventoryEmpty = true;
            for (const InventorySlot &slot : player->GetInventory().GetSlots())
            {
                inventoryEmpty = inventoryEmpty && slot.IsEmpty() && slot.GetAmount() == 0;
            }
            test.Expect(inventoryEmpty, "Non-default initialization grants no starter items");
            for (SkillType skillType : PERSISTED_SKILL_TYPES)
            {
                test.ExpectEqual(player->GetSkills().GetSkill(skillType).GetXP(), 0,
                                 "Non-default initialization grants no XP");
                test.ExpectEqual(player->GetSkills().GetSkill(skillType).GetLevel(), 1,
                                 "Non-default initialization starts at level one");
            }
        }
        for (EquipmentSlotType slotType : PERSISTED_EQUIPMENT_SLOT_TYPES)
        {
            test.Expect(empty.GetEquipment().IsSlotEmpty(slotType),
                        "Empty Player equipment starts empty");
        }
        test.Expect(empty.GetStatusEffectManager().GetActiveEffects().empty(),
                    "Empty Player has no status effects");
        test.ExpectEqual(empty.GetMaximumHealth(), 100,
                         "Empty Player has correct derived maximum health");
        test.ExpectEqual(empty.GetCurrentHealth(), empty.GetMaximumHealth(),
                         "Empty Player starts at full health");
        test.ExpectEqual(empty.GetPosition().GetX(), 0, "Empty Player starts at X zero");
        test.ExpectEqual(empty.GetPosition().GetY(), 0, "Empty Player starts at Y zero");
    }

    {
        Inventory inventory;
        inventory.AddItem(ItemType::LOG, 1);
        Inventory before = inventory;
        std::array<InventorySlot, Inventory::SlotCount> slots{};
        slots[4].SetItem(ItemType::COINS, 17);
        slots[9].SetItem(ItemType::COAL, 1);
        slots[10].SetItem(ItemType::COAL, 1);
        test.Expect(inventory.TryReplaceSlotsAtomically(slots),
                    "Exact slot replacement accepts canonical state");
        test.Expect(inventory.GetSlots()[4].GetItemType() == ItemType::COINS &&
                        inventory.GetSlots()[4].GetAmount() == 17 &&
                        inventory.GetSlots()[9].GetItemType() == ItemType::COAL &&
                        inventory.GetSlots()[10].GetItemType() == ItemType::COAL,
                    "Exact slot replacement commits all slots without reordering");

        Inventory committed = inventory;
        slots[0].SetItem(ItemType::NONE, 1);
        test.Expect(!inventory.TryReplaceSlotsAtomically(slots),
                    "Non-canonical empty replacement rejects");
        bool unchanged = true;
        for (int index = 0; index < Inventory::SlotCount; ++index)
        {
            unchanged = unchanged &&
                inventory.GetSlots()[index].GetItemType() == committed.GetSlots()[index].GetItemType() &&
                inventory.GetSlots()[index].GetAmount() == committed.GetSlots()[index].GetAmount();
        }
        test.Expect(unchanged, "Failed exact replacement leaves Inventory unchanged");

        slots = {};
        slots[0].SetItem(ItemType::COINS, 1);
        slots[1].SetItem(ItemType::COINS, 2);
        test.Expect(!before.TryReplaceSlotsAtomically(slots),
                    "Duplicate stackable replacement rejects");
        slots = {};
        slots[0].SetItem(ItemType::LOG, 2);
        test.Expect(!before.TryReplaceSlotsAtomically(slots),
                    "Non-stackable quantity greater than one rejects");
        slots = {};
        slots[0].SetItem(static_cast<ItemType>(999), 1);
        test.Expect(!before.TryReplaceSlotsAtomically(slots),
                    "Unknown replacement item rejects before definition access");

        slots = {};
        for (InventorySlot &slot : slots)
        {
            slot.SetItem(ItemType::LOG, 1);
        }
        test.Expect(before.TryReplaceSlotsAtomically(slots),
                    "All 28 slots may be occupied by valid non-stackable items");
        test.ExpectEqual(before.GetItemAmount(ItemType::LOG), Inventory::SlotCount,
                         "Fully occupied replacement preserves every item");

        slots = {};
        slots[27].SetItem(ItemType::COINS, INT_MAX);
        test.Expect(before.TryReplaceSlotsAtomically(slots),
                    "Stackable INT_MAX replacement is valid");
        test.ExpectEqual(before.GetSlots()[27].GetAmount(), INT_MAX,
                         "Stackable INT_MAX quantity remains exact");
    }

    PlayerSaveData valid = MakeValidEmptySave();
    test.ExpectEqual(valid.version, 1, "Capture writes exact version one");
    test.ExpectEqual(static_cast<int>(valid.inventorySlots.size()), 28,
                     "Capture stores all 28 inventory slots");
    test.ExpectEqual(static_cast<int>(valid.skills.size()), 5,
                     "Capture stores all five supported skills");
    test.ExpectEqual(static_cast<int>(valid.equipment.size()), 5,
                     "Capture stores all five equipment slots");
    for (std::size_t index = 0; index < PERSISTED_SKILL_TYPES.size(); ++index)
    {
        test.Expect(valid.skills[index].skillType == PERSISTED_SKILL_TYPES[index],
                    "Capture uses canonical skill order");
    }
    for (std::size_t index = 0; index < PERSISTED_EQUIPMENT_SLOT_TYPES.size(); ++index)
    {
        test.Expect(valid.equipment[index].slotType == PERSISTED_EQUIPMENT_SLOT_TYPES[index],
                    "Capture uses canonical equipment order");
        test.Expect(valid.equipment[index].itemType == ItemType::NONE,
                    "Capture preserves empty equipment slots");
    }

    {
        PlayerSaveData malformed = valid;
        malformed.inventorySlots[2] = {ItemType::NONE, 1};
        malformed.inventorySlots[5] = {static_cast<ItemType>(999), 1};
        PlayerSaveData before = malformed;
        PlayerSaveValidationReport report = PlayerSaveState::Validate(malformed);
        test.ExpectEqual(report.GetErrorCount(), static_cast<std::size_t>(2),
                         "Validation report exposes exact issue count");
        test.Expect(report.GetIssues()[0].code ==
                            PlayerSaveValidationCode::NON_CANONICAL_EMPTY_INVENTORY_SLOT &&
                        report.GetIssues()[0].recordIndex == 2 &&
                        report.GetIssues()[1].code ==
                            PlayerSaveValidationCode::UNKNOWN_INVENTORY_ITEM &&
                        report.GetIssues()[1].recordIndex == 5,
                    "Validation issues have deterministic order and record indexes");
        test.Expect(SameSave(malformed, before),
                    "Validation does not mutate malformed save data");
    }

    {
        Player original(7, PlayerInitializationMode::EMPTY);
        original.GetPosition().SetPosition(INT_MIN, INT_MAX);
        original.GetSkills().AddXP(SkillType::ATTACK, 5000);
        original.GetSkills().AddXP(SkillType::DEFENCE, 2000);
        original.GetSkills().AddXP(SkillType::WOODCUTTING, INT_MAX);
        std::array<InventorySlot, Inventory::SlotCount> slots{};
        slots[3].SetItem(ItemType::COINS, INT_MAX);
        slots[20].SetItem(ItemType::LOG, 1);
        slots[27].SetItem(ItemType::LOG, 1);
        test.Expect(original.GetInventory().TryReplaceSlotsAtomically(slots),
                    "Round-trip fixture exact inventory is valid");
        test.Expect(original.GetEquipment().Equip(EquipmentSlotType::WEAPON, ItemType::STEEL_AXE),
                    "Round-trip fixture equips requirement-bearing weapon");
        test.Expect(original.GetEquipment().Equip(EquipmentSlotType::SHIELD, ItemType::WOODEN_SHIELD),
                    "Round-trip fixture equips shield");
        original.RefreshDerivedState();
        original.ApplyDamage(7);

        PlayerSaveData captured = PlayerSaveState::Capture(original);
        test.ExpectEqual(captured.positionX, INT_MIN, "Capture preserves exact X position");
        test.ExpectEqual(captured.positionY, INT_MAX, "Capture preserves exact Y position");
        test.ExpectEqual(captured.currentHealth, original.GetCurrentHealth(),
                         "Capture preserves exact current health");
        test.ExpectEqual(captured.inventorySlots[3].quantity, INT_MAX,
                         "Capture preserves exact stack quantity and slot");
        test.ExpectEqual(captured.skills[2].xp, INT_MAX,
                         "Capture safely preserves INT_MAX XP");

        PlayerSaveValidationReport report;
        std::unique_ptr<Player> restored =
            PlayerSaveState::TryCreatePlayer(88, captured, report);
        test.Expect(restored != nullptr && report.IsValid(),
                    "Valid save reconstructs an unregistered Player");
        if (restored)
        {
            test.ExpectEqual(restored->GetID(), 88, "Supplied runtime entity ID is used");
            test.ExpectEqual(restored->GetInventory().GetItemAmount(ItemType::BRONZE_PICKAXE), 0,
                             "Restoration does not add starter items");
            test.ExpectEqual(restored->GetMaximumHealth(), 105,
                             "Derived maximum health is recalculated from equipment");
            test.ExpectEqual(restored->GetCurrentHealth(), captured.currentHealth,
                             "Ordinary current health restores exactly");
            test.ExpectEqual(
                restored->GetSkills().GetSkill(SkillType::WOODCUTTING).GetXP(),
                INT_MAX,
                "INT_MAX XP restores exactly without overflow");
            test.ExpectEqual(
                restored->GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel(),
                99,
                "INT_MAX XP level calculation terminates at level 99");
            test.Expect(restored->GetStatusEffectManager().GetActiveEffects().empty(),
                        "Restored Player has no status effects");
            PlayerSaveData recaptured = PlayerSaveState::Capture(*restored);
            test.Expect(SameSave(captured, recaptured),
                        "Capture restore capture round trip preserves every persistent field");
            test.Expect(recaptured.skills[0].skillType == SkillType::ATTACK &&
                            recaptured.skills[4].skillType == SkillType::SMITHING,
                        "Recapture canonicalizes reordered record order");
        }
    }

    for (int version : {0, -1, 2})
    {
        PlayerSaveData malformed = valid;
        malformed.version = version;
        ExpectRejected(test, malformed, PlayerSaveValidationCode::UNSUPPORTED_VERSION,
                       "Unsupported version rejects");
    }
    for (int health : {0, -1})
    {
        PlayerSaveData malformed = valid;
        malformed.currentHealth = health;
        ExpectRejected(test, malformed, PlayerSaveValidationCode::INVALID_CURRENT_HEALTH,
                       "Non-positive health rejects");
    }

    {
        Player dead(12, PlayerInitializationMode::EMPTY);
        dead.ApplyDamage(dead.GetCurrentHealth());
        PlayerSaveData deadSave = PlayerSaveState::Capture(dead);
        test.ExpectEqual(deadSave.currentHealth, 0,
                         "Capture mechanically records dead Player health zero");
        ExpectRejected(test, deadSave, PlayerSaveValidationCode::INVALID_CURRENT_HEALTH,
                       "Captured dead Player cannot be restored in version 1");
    }

    for (int health : {1, 100, 101, INT_MAX})
    {
        PlayerSaveData healthSave = valid;
        healthSave.currentHealth = health;
        PlayerSaveValidationReport report;
        std::unique_ptr<Player> restored =
            PlayerSaveState::TryCreatePlayer(13, healthSave, report);
        test.Expect(restored != nullptr && report.IsValid(),
                    "Every positive health boundary restores");
        if (restored)
        {
            test.ExpectEqual(restored->GetCurrentHealth(), std::min(health, 100),
                             "Positive health boundary restores or clamps exactly");
            test.Expect(restored->IsAlive(),
                        "Valid restored health always leaves Player alive");
        }
    }
    for (int id : {0, -1})
    {
        PlayerSaveValidationReport report;
        test.Expect(PlayerSaveState::TryCreatePlayer(id, valid, report) == nullptr,
                    "Non-positive runtime entity ID rejects");
        test.Expect(report.Contains(PlayerSaveValidationCode::INVALID_ENTITY_ID),
                    "Invalid runtime ID has structured validation code");
    }

    {
        PlayerSaveData malformed = valid;
        malformed.inventorySlots[0] = {ItemType::NONE, 1};
        ExpectRejected(test, malformed,
                       PlayerSaveValidationCode::NON_CANONICAL_EMPTY_INVENTORY_SLOT,
                       "NONE with positive quantity rejects");
        malformed.inventorySlots[0] = {ItemType::NONE, -1};
        ExpectRejected(test, malformed,
                       PlayerSaveValidationCode::NON_CANONICAL_EMPTY_INVENTORY_SLOT,
                       "NONE with negative quantity rejects");
        malformed.inventorySlots[0] = {ItemType::LOG, 0};
        ExpectRejected(test, malformed, PlayerSaveValidationCode::INVALID_INVENTORY_QUANTITY,
                       "Known item with zero quantity rejects");
        malformed.inventorySlots[0] = {ItemType::LOG, -1};
        ExpectRejected(test, malformed, PlayerSaveValidationCode::INVALID_INVENTORY_QUANTITY,
                       "Known item with negative quantity rejects");
        malformed.inventorySlots[0] = {static_cast<ItemType>(999), 1};
        ExpectRejected(test, malformed, PlayerSaveValidationCode::UNKNOWN_INVENTORY_ITEM,
                       "Unknown inventory item rejects");
        malformed.inventorySlots[0] = {ItemType::LOG, 2};
        ExpectRejected(test, malformed,
                       PlayerSaveValidationCode::INVALID_NON_STACKABLE_QUANTITY,
                       "Non-stackable quantity greater than one rejects");
        malformed.inventorySlots[0] = {ItemType::COINS, 1};
        malformed.inventorySlots[1] = {ItemType::COINS, 2};
        ExpectRejected(test, malformed, PlayerSaveValidationCode::DUPLICATE_STACKABLE_ITEM,
                       "Duplicate stackable inventory item rejects");

        PlayerSaveData duplicateNonStackable = valid;
        duplicateNonStackable.inventorySlots[5] = {ItemType::LOG, 1};
        duplicateNonStackable.inventorySlots[17] = {ItemType::LOG, 1};
        PlayerSaveValidationReport report;
        std::unique_ptr<Player> restored = PlayerSaveState::TryCreatePlayer(
            9, duplicateNonStackable, report);
        test.Expect(restored != nullptr &&
                        restored->GetInventory().GetSlots()[5].GetItemType() == ItemType::LOG &&
                        restored->GetInventory().GetSlots()[17].GetItemType() == ItemType::LOG,
                    "Duplicate non-stackable items preserve exact separate slots");
    }

    {
        PlayerSaveData malformed = valid;
        malformed.skills[0].xp = -1;
        ExpectRejected(test, malformed, PlayerSaveValidationCode::NEGATIVE_SKILL_XP,
                       "Negative skill XP rejects");
        malformed = valid;
        malformed.skills[0].skillType = SkillType::NONE;
        ExpectRejected(test, malformed, PlayerSaveValidationCode::UNKNOWN_SKILL,
                       "NONE skill rejects");
        malformed = valid;
        malformed.skills[0].skillType = static_cast<SkillType>(999);
        ExpectRejected(test, malformed, PlayerSaveValidationCode::UNKNOWN_SKILL,
                       "Unknown skill rejects safely");
        malformed = valid;
        malformed.skills[1] = malformed.skills[0];
        ExpectRejected(test, malformed, PlayerSaveValidationCode::DUPLICATE_SKILL,
                       "Duplicate skill rejects");
        for (SkillType missing : PERSISTED_SKILL_TYPES)
        {
            malformed = valid;
            malformed.skills.erase(
                std::remove_if(malformed.skills.begin(), malformed.skills.end(),
                               [missing](const SavedSkillXP &skill)
                               { return skill.skillType == missing; }),
                malformed.skills.end());
            ExpectRejected(test, malformed, PlayerSaveValidationCode::MISSING_SKILL,
                           "Each missing supported skill rejects");
        }

        PlayerSaveData reordered = valid;
        std::reverse(reordered.skills.begin(), reordered.skills.end());
        PlayerSaveValidationReport report;
        std::unique_ptr<Player> restored = PlayerSaveState::TryCreatePlayer(6, reordered, report);
        test.Expect(restored != nullptr, "Reordered complete skill records validate");
        test.Expect(restored && PlayerSaveState::Capture(*restored).skills[0].skillType == SkillType::ATTACK,
                    "Recapture writes reordered skills canonically");
    }

    {
        PlayerSaveData malformed = valid;
        malformed.equipment[0].slotType = EquipmentSlotType::NONE;
        ExpectRejected(test, malformed, PlayerSaveValidationCode::UNKNOWN_EQUIPMENT_SLOT,
                       "NONE equipment slot rejects");
        malformed = valid;
        malformed.equipment[0].slotType = EquipmentSlotType::COUNT;
        ExpectRejected(test, malformed, PlayerSaveValidationCode::UNKNOWN_EQUIPMENT_SLOT,
                       "COUNT equipment slot rejects");
        malformed = valid;
        malformed.equipment[0].slotType = static_cast<EquipmentSlotType>(999);
        ExpectRejected(test, malformed, PlayerSaveValidationCode::UNKNOWN_EQUIPMENT_SLOT,
                       "Unknown equipment slot rejects safely");
        malformed = valid;
        malformed.equipment[1] = malformed.equipment[0];
        ExpectRejected(test, malformed, PlayerSaveValidationCode::DUPLICATE_EQUIPMENT_SLOT,
                       "Duplicate equipment slot rejects");
        for (EquipmentSlotType missing : PERSISTED_EQUIPMENT_SLOT_TYPES)
        {
            malformed = valid;
            malformed.equipment.erase(
                std::remove_if(
                    malformed.equipment.begin(),
                    malformed.equipment.end(),
                    [missing](const SavedEquipmentSlot &slot)
                    { return slot.slotType == missing; }),
                malformed.equipment.end());
            ExpectRejected(test, malformed, PlayerSaveValidationCode::MISSING_EQUIPMENT_SLOT,
                           "Each missing supported equipment slot rejects");
        }
        malformed = valid;
        malformed.equipment[3].itemType = static_cast<ItemType>(999);
        ExpectRejected(test, malformed, PlayerSaveValidationCode::UNKNOWN_EQUIPPED_ITEM,
                       "Unknown equipped item rejects");
        malformed.equipment[3].itemType = ItemType::LOG;
        ExpectRejected(test, malformed, PlayerSaveValidationCode::ITEM_NOT_EQUIPPABLE,
                       "Non-equippable item rejects");
        malformed.equipment[3].itemType = ItemType::WOODEN_SHIELD;
        ExpectRejected(test, malformed, PlayerSaveValidationCode::EQUIPMENT_SLOT_MISMATCH,
                       "Shield in weapon slot rejects");
        malformed = valid;
        malformed.equipment[4].itemType = ItemType::BRONZE_SWORD;
        ExpectRejected(test, malformed, PlayerSaveValidationCode::EQUIPMENT_SLOT_MISMATCH,
                       "Weapon in shield slot rejects");
        const int levelTenXP = FindMinimumXPForLevel(10);
        malformed = valid;
        malformed.equipment[3].itemType = ItemType::STEEL_AXE;
        malformed.skills[2].xp = levelTenXP - 1;
        ExpectRejected(test, malformed,
                       PlayerSaveValidationCode::EQUIPMENT_REQUIREMENT_NOT_MET,
                       "Skill-restricted tool one XP below required level rejects");
        malformed.skills[2].xp = levelTenXP;
        PlayerSaveValidationReport report;
        std::unique_ptr<Player> restored = PlayerSaveState::TryCreatePlayer(5, malformed, report);
        test.Expect(restored != nullptr &&
                        restored->GetEquipment().GetEquippedItem(EquipmentSlotType::WEAPON) ==
                            ItemType::STEEL_AXE,
                    "Skill-restricted tool succeeds at exactly the required level");
        malformed.skills[2].xp = INT_MAX;
        restored = PlayerSaveState::TryCreatePlayer(6, malformed, report);
        test.Expect(restored != nullptr,
                    "Skill-restricted tool succeeds when requirement is exceeded");
    }

    {
        Player boosted(10, PlayerInitializationMode::EMPTY);
        StatusEffectDefinition effect{
            StatusEffectType::MAX_HEALTH_BOOST,
            20,
            StatusEffectModifiers{0, 0, 0, 50}};
        test.Expect(boosted.GetStatusEffectManager().Apply(effect, 0),
                    "Temporary maximum-health effect applies for save test");
        boosted.RefreshDerivedState();
        boosted.Heal(50);
        test.ExpectEqual(boosted.GetCurrentHealth(), 150,
                         "Fixture health rises above permanent maximum");
        PlayerSaveData captured = PlayerSaveState::Capture(boosted);
        test.ExpectEqual(captured.currentHealth, 150,
                         "Capture preserves health raised by temporary maximum");
        test.Expect(boosted.GetStatusEffectManager().HasEffect(StatusEffectType::MAX_HEALTH_BOOST),
                    "Capture does not mutate or clear status effects");

        PlayerSaveValidationReport report;
        std::unique_ptr<Player> restored = PlayerSaveState::TryCreatePlayer(11, captured, report);
        test.Expect(restored != nullptr && report.IsValid(),
                    "Positive over-maximum saved health restores successfully");
        if (restored)
        {
            test.Expect(restored->GetStatusEffectManager().GetActiveEffects().empty(),
                        "Temporary status effect is absent after reconstruction");
            test.ExpectEqual(restored->GetMaximumHealth(), 100,
                             "Permanent maximum health excludes temporary effect");
            test.ExpectEqual(restored->GetCurrentHealth(), 100,
                             "Positive over-maximum health clamps to permanent maximum");
            PlayerSaveData normalized = PlayerSaveState::Capture(*restored);
            test.ExpectEqual(normalized.currentHealth, 100,
                             "Recapture records normalized permanent health");
        }
    }

    return test.Finish();
}
