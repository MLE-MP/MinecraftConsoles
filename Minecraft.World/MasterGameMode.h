#pragma once

#include "CommonMasterGameMode.h"

class BlockDegradeRoutine;
class GameStats;
class ServerPlayer;
class GameType;

class MasterGameMode : public CommonMasterGameMode
{
public:
	enum ETimerID
	{
		GracePeriod = 3,
		ChestRefill = 5
	};

	class CountdownInfo
	{
	public:
		CountdownInfo();

		int m_int0;
		vector<int> m_vector;
		void (*m_onTimerFunc)(MasterGameMode *, void *);
		void (*m_onTimerFunc1)(MasterGameMode *, void *);
	};

	enum EInternalGameModeState
	{
		STATE_INIT           = 0,
		STATE_LOBBY_WAIT     = 1,
		STATE_MAP_SELECT     = 2,
		STATE_LOADING        = 3,
		STATE_PRE_ROUND      = 4,
		STATE_IN_ROUND_A     = 5,
		STATE_IN_ROUND_B     = 6,
		STATE_POST_ROUND     = 7,
		STATE_ROUND_RESULTS  = 8,
		STATE_TRANSITION_A   = 9,
		STATE_INTER_ROUND    = 10,
		STATE_FINAL_RESULTS  = 11,
		STATE_TRANSITION_B   = 12,
		STATE_GAME_OVER      = 13,
		STATE_SHUTDOWN       = 14
	};

	MasterGameMode();
	~MasterGameMode();
	void Tick();
	long long RestartMapGenerator();
	void GetGameModeState() const;
	Level *GeneratorTargetLevel() const;
	void setChosenThemeWordId(int wordId);
	void IsRoundPlaying() const;
	void getScoreboard() const;
	void GetSweptVolumeFromLastKnownPosition(shared_ptr<Player> player);
	void GetLastCheckpointID(int statsUID);
	void OnCheckpointEnter(const shared_ptr<Player> &player, unsigned int checkpointId, CheckpointRuleDefinition *checkpoint);
	void OnPrimaryTargetEnter(const shared_ptr<Player> &player, TargetAreaRuleDefinition *target);
	void OnSecondaryTargetEnter(const shared_ptr<Player> &player, TargetAreaRuleDefinition *target);
	void OnPowerupEnter(const shared_ptr<Player> &player, unsigned int powerupId, PowerupRuleDefinition *powerup);

	void PopulateChests();
	void SetLockPlayerPositions(bool lock);
	void SetState(EInternalGameModeState state);
	void SetPlayersInvulnerable(bool invulnerable);
	void SelectNewGameRules();
	void ChooseNextGameRules(bool forceNew);
	int ChooseItemSet(const MiniGameDef &def, bool useRandom);
	void SetupTeams();
	void GeneratePlaylistSyncInfo();
	void SetAllPlayersGameType(GameType *type);

	static void StaticCtor();
	static void OnGameStart(MasterGameMode *_this, void *data);
	static void OnRefillChestTimer(MasterGameMode *_this, void *data);
	static void OnGracePeriodEnd(MasterGameMode *_this, void *data);

	GameStats *GetGameStats() { return m_gameStats; }
	EInternalGameModeState GetState() const { return m_state; }

	EInternalGameModeState m_state;
	int m_tickCounter;
	int m_roundNumber;
	bool m_playersInvulnerable;
	BlockDegradeRoutine *m_degradeRoutine;
	GameStats *m_gameStats;
};
