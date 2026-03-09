#pragma once

#include "UIScene.h"
#include "..\..\..\Minecraft.World\EMiniGameId.h"

typedef struct _MiniGameCreateMenuInitData
{
	int iPad;
	EMiniGameId miniGameId;
	BOOL bOnline;
	BOOL bIsPrivate;

	_MiniGameCreateMenuInitData()
	{
		iPad = 0;
		miniGameId = MINIGAME_BATTLE;
		bOnline = FALSE;
		bIsPrivate = FALSE;
	}
} MiniGameCreateMenuInitData;

class UIScene_MiniGameCreateMenu : public UIScene
{
private:
	enum EControls
	{
		eControl_GameType,
		eControl_MoreOptions,
		eControl_CreateGame,
		eControl_OnlineGame,
	};

	EMiniGameId m_miniGameId;
	int m_iGameType;

	UIControl m_controlMainPanel;
	UIControl_Label m_labelWorldName;
	UIControl_Button m_buttonGameType, m_buttonMoreOptions, m_buttonCreateGame;
	UIControl m_editWorldName;
	UIControl m_texturePackSelector;
	UIControl m_sliderDifficulty;
	UIControl_CheckBox m_checkboxOnline;

	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT(m_controlMainPanel, "MainPanel")
		UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
			UI_MAP_ELEMENT(m_labelWorldName, "WorldName")
			UI_MAP_ELEMENT(m_editWorldName, "EditWorldName")
			UI_MAP_ELEMENT(m_texturePackSelector, "TexturePackSelector")
			UI_MAP_ELEMENT(m_buttonGameType, "GameModeToggle")
			UI_MAP_ELEMENT(m_checkboxOnline, "CheckboxOnline")
			UI_MAP_ELEMENT(m_buttonMoreOptions, "MoreOptions")
			UI_MAP_ELEMENT(m_buttonCreateGame, "NewWorld")
			UI_MAP_ELEMENT(m_sliderDifficulty, "Difficulty")
		UI_END_MAP_CHILD_ELEMENTS()
	UI_END_MAP_ELEMENTS_AND_NAMES()

	LaunchMoreOptionsMenuInitData m_MoreOptionsParams;
	bool m_bIgnoreInput;

public:
	UIScene_MiniGameCreateMenu(int iPad, void *initData, UILayer *parentLayer);
	virtual ~UIScene_MiniGameCreateMenu();

	virtual EUIScene getSceneType() { return eUIScene_MiniGameCreateMenu; }
	virtual wstring getMoviePath();
	virtual void updateTooltips();
	virtual void updateComponents();

	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);

protected:
	void handlePress(F64 controlId, F64 childId);
	void handleGainFocus(bool navBack);

private:
	static const wchar_t *GetMiniGameName(EMiniGameId id);
	static void CreateMiniGame(UIScene_MiniGameCreateMenu *pClass);
};
