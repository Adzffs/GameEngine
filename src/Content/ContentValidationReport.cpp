#include "ContentValidationReport.h"

void ContentValidationReport::AddError(
    const std::string &category,
    const std::string &contentID,
    const std::string &message)
{
    AddIssue(
        ContentValidationSeverity::ERROR,
        category,
        contentID,
        message);
}

void ContentValidationReport::AddWarning(
    const std::string &category,
    const std::string &contentID,
    const std::string &message)
{
    AddIssue(
        ContentValidationSeverity::WARNING,
        category,
        contentID,
        message);
}

bool ContentValidationReport::IsValid() const
{
    return errorCount == 0;
}

int ContentValidationReport::GetErrorCount() const
{
    return errorCount;
}

int ContentValidationReport::GetWarningCount() const
{
    return warningCount;
}

const std::vector<ContentValidationIssue> &
ContentValidationReport::GetIssues() const
{
    return issues;
}

void ContentValidationReport::AddIssue(
    ContentValidationSeverity severity,
    const std::string &category,
    const std::string &contentID,
    const std::string &message)
{
    issues.push_back(ContentValidationIssue{
        severity,
        category,
        contentID,
        message});

    if (severity == ContentValidationSeverity::ERROR)
    {
        errorCount++;
    }
    else
    {
        warningCount++;
    }
}
