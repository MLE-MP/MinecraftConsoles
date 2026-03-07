#pragma once

#include "EMiniGameId.h"
#include "MapGenerator.h"
#include "Voteables.h"
#include "AABB.h"
#include <vector>
#include <memory>

class CheckpointRuleDefinition;
class TargetAreaRuleDefinition;
class PowerupRuleDefinition;
class Player;
class MiniGameDef;
class Team;
class Level;

class CommonMasterGameMode
{
public:
	virtual ~CommonMasterGameMode();
	virtual void Setup(EMiniGameId id);
	virtual void Tick();
	virtual void OnExitedGame();
	virtual long long RestartMapGenerator();
	virtual void GetGameModeState() const = 0;
	virtual Level *GeneratorTargetLevel() const = 0;
	virtual void setChosenThemeWordId(int wordId);
	virtual void IsRoundPlaying() const = 0;
	virtual void getScoreboard() const = 0;
	virtual void ResetMapDistances();
	virtual void CacheMapDistance();
	virtual void GetSweptVolumeFromLastKnownPosition(shared_ptr<Player> player) = 0;
	virtual void GetLastCheckpointID(int statsUID) = 0;
	virtual void OnProgressMade(const shared_ptr<Player> &player, double progress);
	virtual void OnLapCompleted(const shared_ptr<Player> &player);
	virtual void OnCheckpointEnter(const shared_ptr<Player> &player, unsigned int checkpointId, CheckpointRuleDefinition *checkpoint);
	virtual void OnTargetEnter(const shared_ptr<Player> &player, unsigned int targetId, TargetAreaRuleDefinition *target);
	virtual void OnPrimaryTargetEnter(const shared_ptr<Player> &player, TargetAreaRuleDefinition *target);
	virtual void OnSecondaryTargetEnter(const shared_ptr<Player> &player, TargetAreaRuleDefinition *target);
	virtual void OnPowerupEnter(const shared_ptr<Player> &player, unsigned int powerupId, PowerupRuleDefinition *powerup);

	void CheckPowerups(const vector<shared_ptr<Player>> &players);
	void setCheckpoints(vector<CheckpointRuleDefinition *> checkpoints);
	const AABB *GetTeamArea(Team *team) const;
	void SetupMiniGameInstance(const MiniGameDef &def, int param);
	void clearBuildOffVotes();

	static const MiniGameDef *GetMiniGame();

	vector<int> *GetPossibleVotes() { return &m_possibleVotes; }

	long long m_mapGeneratorSeed;
	float m_genUnkFloat;
	MapGenerator *m_mapGenerator;
	vector<int> m_possibleVotes;
	Voteables *m_voteables;
	vector<AABB *> m_glideTargets;
	vector<CheckpointRuleDefinition *> m_glideCheckpoints;
};
