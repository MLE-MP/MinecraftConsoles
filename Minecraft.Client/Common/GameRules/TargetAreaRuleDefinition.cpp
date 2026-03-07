#include "stdafx.h"
#include "TargetAreaRuleDefinition.h"

ConsoleGameRules::EGameRuleType TargetAreaRuleDefinition::getActionType()
{
	return ConsoleGameRules::eGameRuleType_TargetArea;
}

AABB *TargetAreaRuleDefinition::getBoundingVolume()
{
	return getArea();
}

void TargetAreaRuleDefinition::getPointValue()
{
}

GlideRingGenerator::eGlideRingSize TargetAreaRuleDefinition::getSize()
{
	return (GlideRingGenerator::eGlideRingSize)0;
}
