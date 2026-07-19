#include "TestSupport.h"

#include "../src/AI/MonsterAISystem.h"
#include "../src/Entity/Manager/EntityManager.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/NPC/NPC.h"
#include "../src/Player/Player.h"
#include "../src/World/World.h"

#include <type_traits>

namespace
{
    constexpr CombatRatings Ratings{8, 8, 8, 25};
    constexpr MonsterAggressionDefinition Aggression{5, 8};

    struct Fixture
    {
        EntityManager entities;
        MonsterAISystem system;

        Monster *CreateMonster(
            int x = 10,
            int y = 10,
            bool aggressive = true)
        {
            const int id = entities.CreateMonster(
                x,
                y,
                Ratings,
                RewardTableType::NONE,
                std::nullopt,
                aggressive
                    ? std::optional<MonsterAggressionDefinition>(Aggression)
                    : std::nullopt);
            return dynamic_cast<Monster *>(entities.GetEntityByID(id));
        }

        Player *CreatePlayer(int x, int y)
        {
            const int id = entities.CreatePlayer();
            Player *player = dynamic_cast<Player *>(
                entities.GetEntityByID(id));
            player->GetPosition().SetPosition(x, y);
            return player;
        }
    };

    void ExpectType(
        TestContext &test,
        const MonsterAIIntent &intent,
        MonsterAIIntentType expected,
        const std::string &message)
    {
        test.Expect(
            intent.type == expected,
            message);
    }
}

int main()
{
    TestContext test;

    static_assert(std::is_same_v<decltype(MonsterAIIntent::monsterEntityID), int>);
    static_assert(std::is_same_v<decltype(MonsterAIIntent::targetEntityID), int>);
    static_assert(std::is_same_v<decltype(MonsterAIIntent::destination), Position>);
    static_assert(!std::is_pointer_v<decltype(MonsterAIIntent::destination)>);

    {
        Fixture fixture;
        Monster *monster = fixture.CreateMonster(10, 10, false);
        fixture.CreatePlayer(11, 10);
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::NONE,
                   "Passive monster stays idle and does not proactively acquire");
    }

    {
        Fixture fixture;
        Monster *monster = fixture.CreateMonster();
        monster->ApplyDamage(monster->GetCurrentHealth());
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::NONE, "Dead monster has no intent");
    }

    {
        Fixture fixture;
        Monster *monster = fixture.CreateMonster();
        fixture.CreatePlayer(11, 10);
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities, true),
                   MonsterAIIntentType::NONE,
                   "Respawn-suppressed monster has no intent");
    }

    {
        Fixture fixture;
        Monster *monster = fixture.CreateMonster();
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::NONE,
                   "Aggressive monster with no player stays idle at spawn");
        monster->GetPosition().SetPosition(12, 10);
        const MonsterAIIntent intent =
            fixture.system.Evaluate(*monster, fixture.entities);
        ExpectType(test, intent, MonsterAIIntentType::RETURN_HOME,
                   "Untargeted monster away from spawn returns home");
        test.ExpectEqual(intent.destination.GetX(), 10,
                         "Return-home X is original spawn");
        test.ExpectEqual(intent.destination.GetY(), 10,
                         "Return-home Y is original spawn");
    }

    {
        Fixture fixture;
        Monster *monster = fixture.CreateMonster();
        Player *outside = fixture.CreatePlayer(16, 10);
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::NONE,
                   "Player outside detection radius is ignored");
        outside->GetPosition().SetPosition(13, 10);
        const MonsterAIIntent intent =
            fixture.system.Evaluate(*monster, fixture.entities);
        ExpectType(test, intent, MonsterAIIntentType::CHASE_TARGET,
                   "Player inside detection radius is selected");
        test.ExpectEqual(intent.targetEntityID, outside->GetID(),
                         "Selected intent carries player ID");
    }

    {
        Fixture fixture;
        Monster *monster = fixture.CreateMonster();
        Player *dead = fixture.CreatePlayer(11, 10);
        dead->ApplyDamage(dead->GetCurrentHealth());
        fixture.entities.CreateNPC(10, 11);
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::NONE,
                   "Dead players and non-player entities are ignored");
    }

    {
        Fixture fixture;
        Monster *monster = fixture.CreateMonster();
        Player *farther = fixture.CreatePlayer(13, 10);
        Player *nearer = fixture.CreatePlayer(11, 10);
        MonsterAIIntent intent = fixture.system.Evaluate(*monster, fixture.entities);
        test.ExpectEqual(intent.targetEntityID, nearer->GetID(),
                         "Nearest player is selected");

        nearer->GetPosition().SetPosition(13, 10);
        farther->GetPosition().SetPosition(10, 13);
        intent = fixture.system.Evaluate(*monster, fixture.entities);
        test.ExpectEqual(intent.targetEntityID, farther->GetID(),
                         "Equal distance selects lowest entity ID");
    }

    {
        Fixture fixture;
        Player *target = fixture.CreatePlayer(13, 10);
        Player *closer = fixture.CreatePlayer(11, 10);
        Monster *monster = fixture.CreateMonster();
        monster->SetAggressionTargetEntityID(target->GetID());
        MonsterAIIntent intent = fixture.system.Evaluate(*monster, fixture.entities);
        test.ExpectEqual(intent.targetEntityID, target->GetID(),
                         "Existing valid target is retained despite closer player");

        target->GetPosition().SetPosition(16, 10);
        intent = fixture.system.Evaluate(*monster, fixture.entities);
        test.ExpectEqual(intent.targetEntityID, target->GetID(),
                         "Retained target outside detection remains valid inside leash");
        (void)closer;
    }

    {
        Fixture fixture;
        Monster *monster = fixture.CreateMonster();
        Player *target = fixture.CreatePlayer(18, 10);
        monster->SetAggressionTargetEntityID(target->GetID());
        monster->GetPosition().SetPosition(19, 10);
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::CLEAR_TARGET,
                   "Monster violating unchanged leash boundary clears target");

        monster->GetPosition().SetPosition(10, 10);
        monster->SetAggressionTargetEntityID(9999);
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::CLEAR_TARGET,
                   "Missing target is cleared");

        monster->SetAggressionTargetEntityID(target->GetID());
        target->ApplyDamage(target->GetCurrentHealth());
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::CLEAR_TARGET,
                   "Dead target is cleared");
    }

    {
        Fixture fixture;
        Player *target = fixture.CreatePlayer(13, 10);
        Monster *monster = fixture.CreateMonster();
        monster->SetAggressionTargetEntityID(target->GetID());
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::CHASE_TARGET,
                   "Valid distant retained target is chased");
        target->GetPosition().SetPosition(11, 10);
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::ATTACK_TARGET,
                   "Valid adjacent retained target is attacked");
    }

    {
        Fixture fixture;
        Monster *monster = fixture.CreateMonster();
        Player *target = fixture.CreatePlayer(11, 10);
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::ATTACK_TARGET,
                   "New adjacent target is attacked");
        target->GetPosition().SetPosition(13, 10);
        ExpectType(test, fixture.system.Evaluate(*monster, fixture.entities),
                   MonsterAIIntentType::CHASE_TARGET,
                   "New distant target is chased");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        const int monsterID = world.CreateMonster(
            30, 30, Ratings, RewardTableType::NONE,
            std::nullopt, Aggression);
        Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        Monster *monster = dynamic_cast<Monster *>(world.GetEntityByID(monsterID));
        player->GetPosition().SetPosition(33, 30);
        const int startX = monster->GetPosition().GetX();
        world.Update();
        test.ExpectEqual(monster->GetPosition().GetX(), startX,
                         "Chase queues movement without directly mutating position");
        test.ExpectEqual(monster->GetAggressionTargetEntityID(), playerID,
                         "World applies chase target ID");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        const int monsterID = world.CreateMonster(
            30, 30, Ratings, RewardTableType::NONE,
            std::nullopt, Aggression);
        Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        Monster *monster = dynamic_cast<Monster *>(world.GetEntityByID(monsterID));
        player->GetPosition().SetPosition(31, 30);
        const int health = player->GetCurrentHealth();
        world.Update();
        test.Expect(world.GetActionForEntity(monsterID) != nullptr,
                    "Attack intent starts existing timed engagement");
        test.ExpectEqual(player->GetCurrentHealth(), health,
                         "Attack intent applies no immediate damage");
        const Action *action = world.GetActionForEntity(monsterID);
        world.Update();
        test.Expect(world.GetActionForEntity(monsterID) == action,
                    "Existing active attack is not duplicated");
    }

    return test.Finish();
}
