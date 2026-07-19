#pragma once

#include <string>
#include <vector>

enum class ContentValidationSeverity
{
    ERROR,
    WARNING
};

struct ContentValidationIssue
{
    ContentValidationSeverity severity;
    std::string category;
    std::string contentID;
    std::string message;
};

class ContentValidationReport
{
public:
    void AddError(
        const std::string &category,
        const std::string &contentID,
        const std::string &message);

    void AddWarning(
        const std::string &category,
        const std::string &contentID,
        const std::string &message);

    bool IsValid() const;

    int GetErrorCount() const;
    int GetWarningCount() const;

    const std::vector<ContentValidationIssue> &
    GetIssues() const;

private:
    void AddIssue(
        ContentValidationSeverity severity,
        const std::string &category,
        const std::string &contentID,
        const std::string &message);

    std::vector<ContentValidationIssue> issues;
    int errorCount = 0;
    int warningCount = 0;
};
