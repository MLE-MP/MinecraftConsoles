#include "stdafx.h"
#include "MiniGameDef.h"

static MiniGameDef s_defaultDef;

void MiniGameDef::StaticCtor()
{
}

MiniGameDef &MiniGameDef::GetCustomGameModeById(EMiniGameId id, bool useDefault)
{
	return s_defaultDef;
}

bool MiniGameDef::HasFeatureDirectionalFireworks() const { return false; }
int MiniGameDef::GetLivesPerRound() const { return 0; }
double MiniGameDef::getBaseDamage(int type, double defaultVal) const { return defaultVal; }
float MiniGameDef::getExhaustionAttack(float defaultVal) const { return defaultVal; }
float MiniGameDef::getExhaustionDrop(float defaultVal) const { return defaultVal; }
int MiniGameDef::getExhaustionHeal(int defaultVal) const { return defaultVal; }
float MiniGameDef::getExhaustionIdle(float defaultVal) const { return defaultVal; }
float MiniGameDef::getExhaustionJump(float defaultVal) const { return defaultVal; }
float MiniGameDef::getExhaustionMine(float defaultVal) const { return defaultVal; }
float MiniGameDef::getExhaustionSneak(float defaultVal) const { return defaultVal; }
float MiniGameDef::getExhaustionSprint(float defaultVal) const { return defaultVal; }
float MiniGameDef::getExhaustionSprintJump(float defaultVal) const { return defaultVal; }
float MiniGameDef::getExhaustionSwim(float defaultVal) const { return defaultVal; }
float MiniGameDef::getExhaustionWalk(float defaultVal) const { return defaultVal; }
int MiniGameDef::getHealLevel(int defaultVal) const { return defaultVal; }
int MiniGameDef::getHealthTickCount(int defaultVal) const { return defaultVal; }
int MiniGameDef::getHealthTickCountSaturated(int defaultVal) const { return defaultVal; }
int MiniGameDef::getMaxFood(int defaultVal) const { return defaultVal; }
float MiniGameDef::getMaxSaturation(float defaultVal) const { return defaultVal; }
float MiniGameDef::getStartSaturation(float defaultVal) const { return defaultVal; }
float MiniGameDef::getSprintLevel(float defaultVal) const { return defaultVal; }
int MiniGameDef::getStarveLevel(int defaultVal) const { return defaultVal; }
int MiniGameDef::getPotionDuration(int type, int defaultVal) const { return defaultVal; }
bool MiniGameDef::HasFeatureArrowCanKnockback() const { return true; }
bool MiniGameDef::ArePowerupsActive() const { return false; }
EMiniGameId MiniGameDef::GetId() const { return MINIGAME_NORMAL_WORLD; }
size_t MiniGameDef::GetLayerCount() const { return 0; }
bool MiniGameDef::AllowPvp() const { return true; }
int MiniGameDef::getDifficulty() const { return 2; }

void MiniGameDef::SetLootSet(int lootSetId) const
{
	// TODO: implement - calls app.SetRichPresenceContext
}
