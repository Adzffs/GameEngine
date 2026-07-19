#include "ContentStartupValidation.h"

#include "../Content/ContentValidator.h"
#include "Logger.h"

#include <string>

namespace
{
    const char *SeverityToText(
        ContentValidationSeverity severity)
    {
        switch (severity)
        {
        case ContentValidationSeverity::ERROR:
            return "ERROR";

        case ContentValidationSeverity::WARNING:
            return "WARNING";
        }

        return "UNKNOWN";
    }
}

namespace ContentStartupValidation
{
    bool CanStartFromReport(
        const ContentValidationReport &report)
    {
        return report.IsValid();
    }

    bool ValidateAndLog()
    {
        ContentValidationReport report =
            ContentValidator::ValidateAll();

        for (const ContentValidationIssue &issue :
             report.GetIssues())
        {
            Logger::Error(
                std::string("[CONTENT] ") +
                SeverityToText(issue.severity) +
                " " +
                issue.category +
                " " +
                issue.contentID +
                ": " +
                issue.message);
        }

        if (!CanStartFromReport(report))
        {
            Logger::Error(
                std::string("[CONTENT] Validation failed with ") +
                std::to_string(report.GetErrorCount()) +
                " errors");
            return false;
        }

        Logger::Info(
            "[CONTENT] Validated items, resources, recipes, rewards, dialogues and starter spawns");

        return true;
    }
}
