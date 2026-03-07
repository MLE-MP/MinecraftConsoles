#include "stdafx.h"
#include "PowerupRuleDefinition.h"
#include "ConsoleGameRulesConstants.h"
#include "AddItemRuleDefinition.h"

PowerupRuleDefinition::PowerupRuleDefinition() : NamedAreaRuleDefinition()
{
}

PowerupRuleDefinition::~PowerupRuleDefinition()
{
}

ConsoleGameRules::EGameRuleType PowerupRuleDefinition::getActionType()
{
	return ConsoleGameRules::eGameRuleType_Powerup;
}

void PowerupRuleDefinition::getChildren(vector<GameRuleDefinition *> *out)
{
	for(auto it = m_addItemRules.begin(); it != m_addItemRules.end(); it++)
	{
		out->push_back((GameRuleDefinition *)*it);
	}
}

GameRuleDefinition *PowerupRuleDefinition::addChild(ConsoleGameRules::EGameRuleType type)
{
	if(type != ConsoleGameRules::eGameRuleType_AddItem)
		return NULL;

	AddItemRuleDefinition *childRule = new AddItemRuleDefinition();
	m_addItemRules.push_back(childRule);
	return childRule;
}
