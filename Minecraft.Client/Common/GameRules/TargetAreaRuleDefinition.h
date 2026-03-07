#pragma once

#include "NamedAreaRuleDefinition.h"
#include "..\..\..\Minecraft.World\GlideRingGenerator.h"

class TargetAreaRuleDefinition : public NamedAreaRuleDefinition
{
public:
	virtual ConsoleGameRules::EGameRuleType getActionType();
	virtual AABB *getBoundingVolume();
	virtual void getPointValue();
	virtual GlideRingGenerator::eGlideRingSize getSize();

	bool isPrimary() { return m_primary; }

	bool m_primary;
};
