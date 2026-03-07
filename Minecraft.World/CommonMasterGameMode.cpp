#include "stdafx.h"
#include "CommonMasterGameMode.h"
#include "MiniGameDef.h"
#include "EMiniGameId.h"
#include "TumbleGenerator.h"
#include "WallGenerator.h"

CommonMasterGameMode::~CommonMasterGameMode()
{
}

void CommonMasterGameMode::Setup(EMiniGameId id)
{
}

void CommonMasterGameMode::Tick()
{
}

void CommonMasterGameMode::OnExitedGame()
{
}

long long CommonMasterGameMode::RestartMapGenerator()
{
	long long oldSeed = m_mapGeneratorSeed;
	m_mapGeneratorSeed = -1;
	Random random(oldSeed);

	const MiniGameDef *miniGame = GetMiniGame();
	if(!miniGame)
		return oldSeed;

	EMiniGameId minigameId = miniGame->GetId();
	if(minigameId == MINIGAME_TUMBLE)
	{
		// TODO: get LayerGenerationRuleDefinition from rules and create TumbleGenerator
	}
	else if(minigameId == MINIGAME_BUILD_OFF)
	{
		// TODO: get wall areas from rules and create WallGenerator
	}

	return oldSeed;
}

void CommonMasterGameMode::setChosenThemeWordId(int wordId)
{
}

void CommonMasterGameMode::ResetMapDistances()
{
}

void CommonMasterGameMode::CacheMapDistance()
{
}

void CommonMasterGameMode::OnProgressMade(const shared_ptr<Player> &player, double progress)
{
}

void CommonMasterGameMode::OnLapCompleted(const shared_ptr<Player> &player)
{
}

void CommonMasterGameMode::OnCheckpointEnter(const shared_ptr<Player> &player, unsigned int checkpointId, CheckpointRuleDefinition *checkpoint)
{
}

void CommonMasterGameMode::OnTargetEnter(const shared_ptr<Player> &player, unsigned int targetId, TargetAreaRuleDefinition *target)
{
}

void CommonMasterGameMode::OnPrimaryTargetEnter(const shared_ptr<Player> &player, TargetAreaRuleDefinition *target)
{
}

void CommonMasterGameMode::OnSecondaryTargetEnter(const shared_ptr<Player> &player, TargetAreaRuleDefinition *target)
{
}

void CommonMasterGameMode::OnPowerupEnter(const shared_ptr<Player> &player, unsigned int powerupId, PowerupRuleDefinition *powerup)
{
}

void CommonMasterGameMode::CheckPowerups(const vector<shared_ptr<Player>> &players)
{
}

void CommonMasterGameMode::setCheckpoints(vector<CheckpointRuleDefinition *> checkpoints)
{
	m_glideCheckpoints = checkpoints;
}

const AABB *CommonMasterGameMode::GetTeamArea(Team *team) const
{
	return NULL;
}

void CommonMasterGameMode::SetupMiniGameInstance(const MiniGameDef &def, int param)
{
}

void CommonMasterGameMode::clearBuildOffVotes()
{
}

const MiniGameDef *CommonMasterGameMode::GetMiniGame()
{
	return NULL;
}
