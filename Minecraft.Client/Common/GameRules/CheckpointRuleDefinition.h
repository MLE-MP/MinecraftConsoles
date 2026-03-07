#pragma once

#include "NamedAreaRuleDefinition.h"

class CheckpointRuleDefinition : public NamedAreaRuleDefinition
{
public:
	void processBeacons(class Level *level);
};
