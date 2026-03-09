#include "stdafx.h"
#include "MasterGameMode.h"
#include "MiniGameDef.h"
#include "EMiniGameId.h"
#include "LevelSettings.h"
#include "..\Minecraft.Client\Common\GameRules\LevelRuleset.h"
#include "..\Minecraft.Client\Common\GameRules\CheckpointRuleDefinition.h"
#include "..\Minecraft.Client\Common\GameRules\TargetAreaRuleDefinition.h"
#include "..\Minecraft.Client\Common\GameRules\PowerupRuleDefinition.h"
#include "BlockDegradeRoutine.h"
#include "GameStats.h"
#include "..\Minecraft.Client\Common\GameRules\DegradationSequenceRuleDefinition.h"
#include "..\Minecraft.Client\Common\GameRules\ConsoleGameRulesConstants.h"
#include "..\Minecraft.Client\Minecraft.h"
#include "..\Minecraft.Client\MinecraftServer.h"
#include "..\Minecraft.Client\PlayerList.h"
#include "..\Minecraft.Client\ServerPlayer.h"
#include "..\Minecraft.Client\ServerPlayerGameMode.h"

MasterGameMode::CountdownInfo::CountdownInfo()
{
	m_int0 = 0;
	m_onTimerFunc = NULL;
	m_onTimerFunc1 = NULL;
}

void MasterGameMode::StaticCtor()
{
}

MasterGameMode::MasterGameMode()
{
	m_state = STATE_INIT;
	m_tickCounter = 0;
	m_roundNumber = 0;
	m_playersInvulnerable = true;
	m_degradeRoutine = NULL;
	m_gameStats = new GameStats();
}

MasterGameMode::~MasterGameMode()
{
	delete m_degradeRoutine;
	delete m_gameStats;
}

void MasterGameMode::Tick()
{
	CommonMasterGameMode::Tick();

	const MiniGameDef *miniGame = GetMiniGame();
	if(!miniGame || miniGame->GetId() == MINIGAME_NORMAL_WORLD)
		return;

	switch(m_state)
	{
	case STATE_INIT:
		{
			const MiniGameDef &def = MiniGameDef::GetCustomGameModeById(miniGame->GetId(), true);
			SetupMiniGameInstance(def, 0);

			m_tickCounter = 0;
			SetState(STATE_LOBBY_WAIT);
			app.DebugPrintf("MasterGameMode: Entered LOBBY_WAIT state for minigame %d\n", miniGame->GetId());
		}
		break;

	case STATE_LOBBY_WAIT:
		{
			m_tickCounter++;

			MinecraftServer *server = MinecraftServer::getInstance();
			if(!server) break;

			PlayerList *playerList = server->getPlayers();
			if(!playerList) break;

			for(auto it = playerList->players.begin(); it != playerList->players.end(); it++)
			{
				shared_ptr<ServerPlayer> player = *it;
				if(!player) continue;

				if(player->gameMode->getGameModeForPlayer() != GameType::LOBBY)
				{
					player->setGameMode(GameType::LOBBY);
					app.DebugPrintf("MasterGameMode: Set player to LOBBY mode\n");
				}
			}
		}
		break;

	case STATE_MAP_SELECT:
	case STATE_LOADING:
	case STATE_PRE_ROUND:
	case STATE_IN_ROUND_A:
	case STATE_IN_ROUND_B:
	case STATE_POST_ROUND:
	case STATE_ROUND_RESULTS:
	case STATE_TRANSITION_A:
	case STATE_INTER_ROUND:
	case STATE_FINAL_RESULTS:
	case STATE_TRANSITION_B:
	case STATE_GAME_OVER:
	case STATE_SHUTDOWN:
		m_tickCounter++;
		break;
	}
}

long long MasterGameMode::RestartMapGenerator()
{
	long long result = CommonMasterGameMode::RestartMapGenerator();

	// broadcast seed to clients via packet type 25
	// TODO: send MapSelectInfoPacket with new seed

	return result;
}

void MasterGameMode::GetGameModeState() const
{
}

Level *MasterGameMode::GeneratorTargetLevel() const
{
	return NULL; // TODO: return MinecraftServer::getInstance()->getLevel(0)
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
	LevelRuleset *rules = NULL; // TODO: CConsoleMinecraftApp::sInstance.getGameRuleDefinitions()
	if(!rules)
		return;

	vector<GameRuleDefinition *> children;
	rules->getChildren(&children);

	// find OnInitialiseWorld rule
	GameRuleDefinition *initWorld = NULL;
	for(auto it = children.begin(); it != children.end(); it++)
	{
		// OnInitialiseWorld type is 30 in reference enum
		// but in this project it would need to be in ConsoleGameRulesConstants
		// For now just check through children for container placement rules
	}

	// evaluate PlaceContainer, DistributeItems, PopulateContainer rules
	for(auto it = children.begin(); it != children.end(); it++)
	{
		if((*it)->getActionType() == ConsoleGameRules::eGameRuleType_PlaceContainer)
		{
			// evaluate placement rule
		}
	}
}

void MasterGameMode::SetLockPlayerPositions(bool lock)
{
	// TODO: broadcast GameModePacket::LockPlayerLocation to all players
	// TODO: set SetPositionLocked on all ServerPlayers
}

void MasterGameMode::SetState(EInternalGameModeState state)
{
	m_state = state;
	m_tickCounter = 0;
}

void MasterGameMode::SetPlayersInvulnerable(bool invulnerable)
{
	m_playersInvulnerable = invulnerable;

	MinecraftServer *server = MinecraftServer::getInstance();
	if(!server) return;

	PlayerList *playerList = server->getPlayers();
	if(!playerList) return;

	for(auto it = playerList->players.begin(); it != playerList->players.end(); it++)
	{
		shared_ptr<ServerPlayer> player = *it;
		if(!player) continue;

		player->abilities.invulnerable = invulnerable;
		player->onUpdateAbilities();
	}
}

void MasterGameMode::SelectNewGameRules()
{
	ChooseNextGameRules(false);
}

void MasterGameMode::ChooseNextGameRules(bool forceNew)
{
	// TODO: load base save data, get game rules, choose item set
}

int MasterGameMode::ChooseItemSet(const MiniGameDef &def, bool useRandom)
{
	return 0;
}

void MasterGameMode::SetupTeams()
{
	// TODO: assign players to teams based on minigame type
}

void MasterGameMode::GeneratePlaylistSyncInfo()
{
	// TODO: generate sync info for clients
}

void MasterGameMode::OnGameStart(MasterGameMode *_this, void *data)
{
	const MiniGameDef *minigame = CommonMasterGameMode::GetMiniGame();
	if(!minigame)
		return;

	_this->PopulateChests();

	if(minigame->GetId() == MINIGAME_GLIDE)
	{
		// setup elytra, checkpoints, targets for glide
		// TODO: get Level, spawn EnderCrystals at powerup locations if powerups active
		// TODO: start fall flying for players with elytra
		// TODO: process checkpoints and beacons
		// TODO: setup glide targets from TargetAreaRuleDefinitions

		_this->m_glideTargets.clear();
	}

	_this->SetLockPlayerPositions(false);

	// TODO: send GameModePacket with player bounds to all players
	// TODO: setup area bounds per team for BuildOff

	_this->SetState((MasterGameMode::EInternalGameModeState)10);

	if(minigame->GetId() == MINIGAME_BATTLE)
	{
		// TODO: add chest refill timer (30 second interval)
	}

	LevelRuleset *gameRules = NULL; // TODO: CConsoleMinecraftApp::sInstance.getGameRuleDefinitions()
	if(gameRules)
	{
		vector<GameRuleDefinition *> ruleChildren;
		gameRules->getChildren(&ruleChildren);

		for(auto it = ruleChildren.begin(); it != ruleChildren.end(); it++)
		{
			if((*it)->getActionType() == ConsoleGameRules::eGameRuleType_DegradationSequence)
			{
				DegradationSequenceRuleDefinition *degradeRule = (DegradationSequenceRuleDefinition *)(*it);
				_this->m_degradeRoutine = new BlockDegradeRoutine(NULL, degradeRule);
				break;
			}
		}
	}

	// start grace period timer
	_this->SetPlayersInvulnerable(false);

	if(_this->m_gameStats)
		_this->m_gameStats->OnRoundStart();
}

void MasterGameMode::OnRefillChestTimer(MasterGameMode *_this, void *data)
{
	_this->PopulateChests();
}

void MasterGameMode::OnGracePeriodEnd(MasterGameMode *_this, void *data)
{
	_this->SetPlayersInvulnerable(false);
}

void MasterGameMode::SetAllPlayersGameType(GameType *type)
{
	MinecraftServer *server = MinecraftServer::getInstance();
	if(!server) return;

	PlayerList *playerList = server->getPlayers();
	if(!playerList) return;

	for(auto it = playerList->players.begin(); it != playerList->players.end(); it++)
	{
		shared_ptr<ServerPlayer> player = *it;
		if(player)
			player->setGameMode(type);
	}
}
