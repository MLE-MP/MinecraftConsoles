#pragma once

#include "GameRuleDefinition.h"
#include "..\..\..\Minecraft.World\Random.h"

class LayerGenerator;
class AABB;

class LayerGenerationRuleDefinition : public GameRuleDefinition
{
public:
	virtual ConsoleGameRules::EGameRuleType getActionType() { return ConsoleGameRules::eGameRuleType_LayerGeneration; }

	vector<LayerGenerator *> *CreateLayers(Random &random, const AABB *area, unsigned int count, float scale) const;
};
