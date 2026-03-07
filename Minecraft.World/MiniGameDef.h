#pragma once

#include "EMiniGameId.h"

class MiniGameDef
{
public:
	static void StaticCtor();

	static MiniGameDef &GetCustomGameModeById(EMiniGameId id, bool useDefault);

	bool HasFeatureDirectionalFireworks() const;
	int GetLivesPerRound() const;
	double getBaseDamage(int type, double defaultVal) const;
	float getExhaustionAttack(float defaultVal) const;
	float getExhaustionDrop(float defaultVal) const;
	int getExhaustionHeal(int defaultVal) const;
	float getExhaustionIdle(float defaultVal) const;
	float getExhaustionJump(float defaultVal) const;
	float getExhaustionMine(float defaultVal) const;
	float getExhaustionSneak(float defaultVal) const;
	float getExhaustionSprint(float defaultVal) const;
	float getExhaustionSprintJump(float defaultVal) const;
	float getExhaustionSwim(float defaultVal) const;
	float getExhaustionWalk(float defaultVal) const;
	int getHealLevel(int defaultVal) const;
	int getHealthTickCount(int defaultVal) const;
	int getHealthTickCountSaturated(int defaultVal) const;
	int getMaxFood(int defaultVal) const;
	float getMaxSaturation(float defaultVal) const;
	float getStartSaturation(float defaultVal) const;
	float getSprintLevel(float defaultVal) const;
	int getStarveLevel(int defaultVal) const;
	int getPotionDuration(int type, int defaultVal) const;
	bool HasFeatureArrowCanKnockback() const;
	bool ArePowerupsActive() const;
	EMiniGameId GetId() const;
	size_t GetLayerCount() const;
	void SetLootSet(int lootSetId) const;
	bool AllowPvp() const;
	int getDifficulty() const;
};
