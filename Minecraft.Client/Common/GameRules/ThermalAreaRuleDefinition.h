#pragma once

#include "NamedAreaRuleDefinition.h"

class Entity;

class ThermalAreaRuleDefinition : public NamedAreaRuleDefinition
{
public:
	static const wchar_t *DIRECTION_NAMES[8];

	enum eDirection
	{
		DIRECTION_UNDEFINED = -1,
		DIRECTION_PLUS_X = 0,
		DIRECTION_MINUS_X = 1,
		DIRECTION_PLUS_Z = 2,
		DIRECTION_MINUS_Z = 3,
		DIRECTION_OMNI_PLUS_X = 4,
		DIRECTION_OMNI_MINUS_X = 5,
		DIRECTION_OMNI_PLUS_Z = 6,
		DIRECTION_OMNI_MINUS_Z = 7
	};

	ThermalAreaRuleDefinition();

	virtual ConsoleGameRules::EGameRuleType getActionType();
	virtual void addAttribute(const wstring &attributeName, const wstring &attributeValue);

	bool getConditionsMet(shared_ptr<Entity> entity);
	void resetConditions();
	void updateUseCount();
	bool setBoostMods(int &outMod1, int &outMod2);
	int getBoostDirection();

	double getLiftForceModifier() const
	{
		if(m_isActive)
			return m_liftForceModifier;
		return 1.0;
	}

	double getSpeedBoost() const
	{
		if(m_isActive)
			return m_speedBoost;
		return 0.0;
	}

	double getStaticLift() const
	{
		if(m_isActive)
			return m_staticLift;
		return 0.0;
	}

	double getTargetHeight() const
	{
		if(m_isActive)
			return m_targetHeight;
		return 0.0;
	}

	bool isActive() const { return m_isActive; }

	double m_liftForceModifier;
	double m_staticLift;
	double m_targetHeight;
	double m_speedBoost;
	eDirection m_boostDirection;
	int m_conditionType;
	int m_conditionValue0;
	int m_conditionValue1;
	bool m_isActive;
};
