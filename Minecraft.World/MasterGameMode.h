#pragma once

#include "CommonMasterGameMode.h"

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

	enum EInternalGameModeState {};

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

	static void StaticCtor();
	static void OnGameStart(MasterGameMode *_this, void *data);
	static void OnRefillChestTimer(MasterGameMode *_this, void *data);
	static void OnGracePeriodEnd(MasterGameMode *_this, void *data);

	bool m_playersInvulnerable;
};
