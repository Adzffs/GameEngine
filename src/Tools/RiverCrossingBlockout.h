#pragma once

#include "../Content/Export/StarterWorldSnapshot.h"

namespace RiverCrossingBlockout
{
    // Build a separate, runtime-validated starter-region candidate. Reuse the
    // existing actor identities and lifecycle; never mutate the compiled world.
    StarterWorldExport::Snapshot Build();
}
