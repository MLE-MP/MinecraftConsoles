#include "stdafx.h"
#include "MasterGameMode.h"
#include "MiniGameDef.h"

MasterGameMode::CountdownInfo::CountdownInfo()
{
	m_int0 = 0;
	m_onTimerFunc = NULL;
	m_onTimerFunc1 = NULL;
}

void MasterGameMode::StaticCtor()
{
}

MasterGameMode::~MasterGameMode()
{
}

void MasterGameMode::Tick()
{
	CommonMasterGameMode::Tick();
}

long long MasterGameMode::RestartMapGenerator()
{
	return CommonMasterGameMode::RestartMapGenerator();
}

void MasterGameMode::GetGameModeState() const
{
}

Level *MasterGameMode::GeneratorTargetLevel() const
{
	return NULL;
}

void MasterGameMode::setChosenThemeWordId(int wordId)
{
	CommonMasterGameMode::setChosenThemeWordId(wordId);
}

void MasterGameMode::IsRoundPlaying() const
{
}

void MasterGameMode::getScoreboard() const
{
}

void MasterGameMode::GetSweptVolumeFromLastKnownPosition(shared_ptr<Player> player)
{
}

void MasterGameMode::GetLastCheckpointID(int statsUID)
{
}

void MasterGameMode::OnCheckpointEnter(const shared_ptr<Player> &player, unsigned int checkpointId, CheckpointRuleDefinition *checkpoint)
{
	CommonMasterGameMode::OnCheckpointEnter(player, checkpointId, checkpoint);
}

void MasterGameMode::OnPrimaryTargetEnter(const shared_ptr<Player> &player, TargetAreaRuleDefinition *target)
{
	CommonMasterGameMode::OnPrimaryTargetEnter(player, target);
}

void MasterGameMode::OnSecondaryTargetEnter(const shared_ptr<Player> &player, TargetAreaRuleDefinition *target)
{
	CommonMasterGameMode::OnSecondaryTargetEnter(player, target);
}

void MasterGameMode::OnPowerupEnter(const shared_ptr<Player> &player, unsigned int powerupId, PowerupRuleDefinition *powerup)
{
	CommonMasterGameMode::OnPowerupEnter(player, powerupId, powerup);
}

void MasterGameMode::PopulateChests()
{
}

void MasterGameMode::SetLockPlayerPositions(bool lock)
{
}

void MasterGameMode::SetState(EInternalGameModeState state)
{
}

void MasterGameMode::SetPlayersInvulnerable(bool invulnerable)
{
	m_playersInvulnerable = invulnerable;
}

void MasterGameMode::SelectNewGameRules()
{
}

void MasterGameMode::ChooseNextGameRules(bool forceNew)
{
}

int MasterGameMode::ChooseItemSet(const MiniGameDef &def, bool useRandom)
{
	return 0;
}

void MasterGameMode::SetupTeams()
{
}

void MasterGameMode::GeneratePlaylistSyncInfo()
{
}

void MasterGameMode::OnGameStart(MasterGameMode *_this, void *data)
{
}

void MasterGameMode::OnRefillChestTimer(MasterGameMode *_this, void *data)
{
}

void MasterGameMode::OnGracePeriodEnd(MasterGameMode *_this, void *data)
{
}
