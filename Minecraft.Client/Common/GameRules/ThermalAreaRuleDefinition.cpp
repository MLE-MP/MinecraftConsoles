#include "stdafx.h"
#include "ThermalAreaRuleDefinition.h"
#include "ConsoleGameRulesConstants.h"

const wchar_t *ThermalAreaRuleDefinition::DIRECTION_NAMES[8] = {
	L"plus_x",      L"minus_x",      L"plus_z",      L"minus_z",
	L"omni_plus_x", L"omni_minus_x", L"omni_plus_z", L"omni_minus_z",
};

ThermalAreaRuleDefinition::ThermalAreaRuleDefinition()
{
	m_liftForceModifier = 1.0;
	m_staticLift = 0.0;
	m_targetHeight = 0.0;
	m_speedBoost = 0.0;
	m_isActive = true;
	m_boostDirection = DIRECTION_OMNI_PLUS_X;
	m_conditionType = 0;
	m_conditionValue0 = 0;
	m_conditionValue1 = 0;
}

ConsoleGameRules::EGameRuleType ThermalAreaRuleDefinition::getActionType()
{
	return ConsoleGameRules::eGameRuleType_ThermalArea;
}

void ThermalAreaRuleDefinition::addAttribute(const wstring &name, const wstring &value)
{
	// check for boostDirection attribute
	bool isBoostDirection = false;
	// TODO: proper attribute name check when attribute names table is extended
	if(!isBoostDirection)
	{
		GameRuleDefinition::addAttribute(name, value);
		return;
	}

	int direction = DIRECTION_UNDEFINED;
	for(int i = 0; i < 8; ++i)
	{
		if(value.compare(DIRECTION_NAMES[i]) == 0)
		{
			direction = i;
			break;
		}
	}

	if(direction != DIRECTION_UNDEFINED)
	{
		m_boostDirection = (eDirection)direction;
	}
}

bool ThermalAreaRuleDefinition::getConditionsMet(shared_ptr<Entity> entity)
{
	return m_isActive;
}

void ThermalAreaRuleDefinition::resetConditions()
{
}

void ThermalAreaRuleDefinition::updateUseCount()
{
}

bool ThermalAreaRuleDefinition::setBoostMods(int &outMod1, int &outMod2)
{
	return false;
}

int ThermalAreaRuleDefinition::getBoostDirection()
{
	return (int)m_boostDirection;
}
