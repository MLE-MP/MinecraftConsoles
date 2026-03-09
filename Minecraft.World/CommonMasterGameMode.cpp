#include "stdafx.h"
#include "CommonMasterGameMode.h"
#include "MiniGameDef.h"
#include "EMiniGameId.h"
#include "TumbleGenerator.h"
#include "WallGenerator.h"
#include "..\Minecraft.Client\Common\GameRules\LevelGenerationOptions.h"
#include "..\Minecraft.Client\Common\GameRules\LevelRuleset.h"
#include "..\Minecraft.Client\Common\GameRules\LayerGenerationRuleDefinition.h"
#include "..\Minecraft.Client\Common\GameRules\CheckpointRuleDefinition.h"
#include "..\Minecraft.Client\Common\GameRules\TargetAreaRuleDefinition.h"
#include "..\Minecraft.Client\Common\GameRules\PowerupRuleDefinition.h"
#include "..\Minecraft.Client\Minecraft.h"
#include "..\Minecraft.Client\MinecraftServer.h"

static const MiniGameDef *s_currentMiniGame = NULL;

CommonMasterGameMode::~CommonMasterGameMode()
{
	delete m_mapGenerator;
	m_mapGenerator = NULL;
}

void CommonMasterGameMode::Setup(EMiniGameId id)
{
	m_mapGeneratorSeed = -1;
	m_genUnkFloat = 1.0f;
	m_mapGenerator = NULL;
	m_voteables = NULL;
}

void CommonMasterGameMode::Tick()
{
}

void CommonMasterGameMode::OnExitedGame()
{
	delete m_mapGenerator;
	m_mapGenerator = NULL;
	s_currentMiniGame = NULL;
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
		LevelRuleset *rules = NULL; // TODO: get from LevelGenerationOptions
		if(rules)
		{
			LayerGenerationRuleDefinition *layerGenRule = NULL;
			vector<GameRuleDefinition *> children;
			rules->getChildren(&children);
			for(auto it = children.begin(); it != children.end(); it++)
			{
				if((*it)->getActionType() == ConsoleGameRules::eGameRuleType_LayerGeneration)
				{
					layerGenRule = (LayerGenerationRuleDefinition *)(*it);
					break;
				}
			}

			AABB *levelGenArea = rules->getNamedArea(L"LevelGeneration");

			if(layerGenRule && levelGenArea)
			{
				delete m_mapGenerator;

				Random layerRandom(oldSeed);
				vector<LayerGenerator *> *layers = layerGenRule->CreateLayers(
					layerRandom, levelGenArea, miniGame->GetLayerCount(), m_genUnkFloat);

				if(layers)
				{
					m_mapGenerator = new TumbleGenerator(layerRandom, *layers);
					m_mapGenerator->StartGeneration((Level *)GeneratorTargetLevel());
					delete layers;
				}
			}
		}
	}
	else if(minigameId == MINIGAME_BUILD_OFF)
	{
		LevelRuleset *rules = NULL; // TODO: get from LevelGenerationOptions
		if(rules)
		{
			vector<AABB *> walls1;
			vector<AABB *> walls2;

			vector<GameRuleDefinition *> children;
			rules->getChildren(&children);
			for(auto it = children.begin(); it != children.end(); it++)
			{
				if((*it)->getActionType() == ConsoleGameRules::eGameRuleType_NamedArea)
				{
					NamedAreaRuleDefinition *namedArea = (NamedAreaRuleDefinition *)(*it);
					AABB *area = namedArea->getArea();
					if(area)
					{
						wstring name = namedArea->getName();
						if(name.find(L"Wall1") != wstring::npos)
							walls1.push_back(area);
						else if(name.find(L"Wall2") != wstring::npos)
							walls2.push_back(area);
					}
				}
			}

			delete m_mapGenerator;

			m_mapGenerator = new WallGenerator(random, &walls1, &walls2);
			m_mapGenerator->StartGeneration((Level *)GeneratorTargetLevel());
		}
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
	if(!GetMiniGame() || !GetMiniGame()->ArePowerupsActive())
		return;

	for(auto it = players.begin(); it != players.end(); it++)
	{
		// TODO: check if player is in a powerup area
	}
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
	s_currentMiniGame = &def;
	Minecraft::GetInstance()->SetupMiniGameInstance(const_cast<MiniGameDef &>(def), 0);
	MinecraftServer *server = MinecraftServer::getInstance();
	if(server)
		server->setPvpAllowed(def.AllowPvp());
}

void CommonMasterGameMode::clearBuildOffVotes()
{
	if(m_voteables)
	{
		Voteable *buildVoteable = m_voteables->getVoteable(3);
		(void)buildVoteable;
	}
}

const MiniGameDef *CommonMasterGameMode::GetMiniGame()
{
	return s_currentMiniGame;
}
