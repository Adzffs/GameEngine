#include "PlayerSaveValidation.h"

#include <algorithm>
#include <utility>

bool PlayerSaveValidationReport::IsValid() const
{
    return issues.empty();
}

std::size_t PlayerSaveValidationReport::GetErrorCount() const
{
    return issues.size();
}

const std::vector<PlayerSaveValidationIssue> &
PlayerSaveValidationReport::GetIssues() const
{
    return issues;
}

bool PlayerSaveValidationReport::Contains(
    PlayerSaveValidationCode code) const
{
    return std::any_of(
        issues.begin(),
        issues.end(),
        [code](const PlayerSaveValidationIssue &issue)
        {
            return issue.code == code;
        });
}

void PlayerSaveValidationReport::AddIssue(
    PlayerSaveValidationCode code,
    int recordIndex,
    std::string message)
{
    issues.push_back(PlayerSaveValidationIssue{
        code,
        recordIndex,
        std::move(message)});
}
