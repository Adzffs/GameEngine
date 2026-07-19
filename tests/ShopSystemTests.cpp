#include "TestSupport.h"
#include "../src/Shop/ShopSystem.h"

#include <limits>

int main()
{
    TestContext test;
    ShopSystem system;
    const auto first = system.Start(4, 1, NpcType::DEVELOPMENT_GUIDE,
        ShopId::DEVELOPMENT_GUIDE_SUPPLIES, 1);
    test.Expect(first != InvalidShopSessionId, "Start allocates nonzero identity");
    const auto replacement = system.Start(4, 1, NpcType::DEVELOPMENT_GUIDE,
        ShopId::DEVELOPMENT_GUIDE_SUPPLIES, 2);
    test.Expect(replacement != first, "Restart allocates fresh identity");
    test.ExpectEqual(system.GetSessionCount(), std::size_t{1}, "One session per actor");
    const auto other = system.Start(5, 1, NpcType::DEVELOPMENT_GUIDE,
        ShopId::DEVELOPMENT_GUIDE_SUPPLIES, 2);
    test.Expect(other != replacement, "Actors receive unique identities");
    test.Expect(!system.Close(4, first), "Stale close rejects");
    test.Expect(system.GetSession(4) != nullptr, "Stale close preserves replacement");
    test.Expect(system.CancelActor(4), "Actor cancellation succeeds");
    test.Expect(system.GetSession(5) != nullptr, "Actor cancellation is isolated");
    test.ExpectEqual(system.CancelTarget(1), std::size_t{1}, "Target cancellation closes all matches");
    ShopSystem exhausted(std::numeric_limits<ShopSessionId>::max());
    test.Expect(exhausted.Start(4, 1, NpcType::DEVELOPMENT_GUIDE,
        ShopId::DEVELOPMENT_GUIDE_SUPPLIES, 1) != InvalidShopSessionId,
        "Final nonzero identity may be allocated");
    test.Expect(exhausted.Start(5, 1, NpcType::DEVELOPMENT_GUIDE,
        ShopId::DEVELOPMENT_GUIDE_SUPPLIES, 1) == InvalidShopSessionId,
        "Exhaustion fails safely");
    return test.Finish();
}
