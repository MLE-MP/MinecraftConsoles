#pragma once

#include "..\..\..\Minecraft.World\Random.h"
#include "AddItemRuleDefinition.h"
#include "GameRuleDefinition.h"
#include "NamedAreaRuleDefinition.h"

class PowerupRuleDefinition : public NamedAreaRuleDefinition
{
public:
	PowerupRuleDefinition();
	~PowerupRuleDefinition();

	virtual ConsoleGameRules::EGameRuleType getActionType();
	virtual void getChildren(vector<GameRuleDefinition *> *children);
	virtual GameRuleDefinition *addChild(ConsoleGameRules::EGameRuleType ruleType);

	vector<AddItemRuleDefinition *> m_addItemRules;
	Random m_random;
};
