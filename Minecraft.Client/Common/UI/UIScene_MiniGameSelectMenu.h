#pragma once

#include "UIScene.h"
#include "..\..\..\Minecraft.World\EMiniGameId.h"

class UIScene_MiniGameSelectMenu : public UIScene
{
private:
	enum EControls
	{
		eControl_GameList,
	};

	UIControl_SaveList m_gameList;
	UIControl_SaveList m_joinList;
	UIControl_Label m_labelSavesListTitle, m_labelJoinListTitle, m_labelNoGames;
	UIControl m_controlSavesTimer, m_controlJoinTimer;

	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT(m_gameList, "SavesList")
		UI_MAP_ELEMENT(m_joinList, "JoinList")
		UI_MAP_ELEMENT(m_labelSavesListTitle, "SavesListTitle")
		UI_MAP_ELEMENT(m_labelJoinListTitle, "JoinListTitle")
		UI_MAP_ELEMENT(m_labelNoGames, "NoGames")
		UI_MAP_ELEMENT(m_controlSavesTimer, "SavesTimer")
		UI_MAP_ELEMENT(m_controlJoinTimer, "JoinTimer")
	UI_END_MAP_ELEMENTS_AND_NAMES()

	bool m_bIgnorePress;

public:
	UIScene_MiniGameSelectMenu(int iPad, void *initData, UILayer *parentLayer);
	virtual ~UIScene_MiniGameSelectMenu();

	virtual EUIScene getSceneType() { return eUIScene_MiniGameSelectMenu; }
	virtual wstring getMoviePath();
	virtual void updateTooltips();

	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);

protected:
	void handlePress(F64 controlId, F64 childId);
	void handleGainFocus(bool navBack);

private:
	void LaunchMiniGame(EMiniGameId miniGameId);
};
