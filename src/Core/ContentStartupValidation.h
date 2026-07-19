#pragma once

class ContentValidationReport;

namespace ContentStartupValidation
{
    bool CanStartFromReport(
        const ContentValidationReport &report);

    bool ValidateAndLog();
}
