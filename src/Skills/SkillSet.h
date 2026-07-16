#pragma once

#include "Skill.h"
#include "SkillType.h"

#include <map>

class SkillSet
{
public:
    SkillSet();

    void AddXP(
        SkillType skillType,
        int amount);

    const Skill &GetSkill(
        SkillType skillType) const;

private:
    std::map<SkillType, Skill> skills;
};