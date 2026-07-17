#include "SkillSet.h"

SkillSet::SkillSet()
{
    skills.emplace(
        SkillType::WOODCUTTING,
        Skill());

    skills.emplace(
        SkillType::MINING,
        Skill());

    skills.emplace(
        SkillType::SMITHING,
        Skill());
}

void SkillSet::AddXP(
    SkillType skillType,
    int amount)
{
    skills[skillType].AddXP(amount);
}

const Skill &SkillSet::GetSkill(
    SkillType skillType) const
{
    return skills.at(skillType);
}