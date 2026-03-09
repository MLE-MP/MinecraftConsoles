#include "stdafx.h"
#include "UI.h"
#include "UIScene_MiniGameSelectMenu.h"
#include "UIScene_MiniGameCreateMenu.h"
#include "..\..\Minecraft.h"
#include "..\..\..\Minecraft.World\MiniGameDef.h"
#include "..\..\..\Minecraft.World\EMiniGameId.h"

UIScene_MiniGameSelectMenu::UIScene_MiniGameSelectMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	initialiseMovie();

	m_bIgnorePress = false;

	m_gameList.init(eControl_GameList);

	m_gameList.addItem(L"Battle");
	m_gameList.addItem(L"Tumble");
	m_gameList.addItem(L"Glide");

	m_labelSavesListTitle.init(L"Mini Games");
	m_labelJoinListTitle.init(L"");
	m_labelNoGames.init(L"");

	m_controlJoinTimer.setVisible(false);
	m_controlSavesTimer.setVisible(false);

	doHorizontalResizeCheck();
}

UIScene_MiniGameSelectMenu::~UIScene_MiniGameSelectMenu()
{
}

wstring UIScene_MiniGameSelectMenu::getMoviePath()
{
	return L"LoadOrJoinMenu";
}

void UIScene_MiniGameSelectMenu::updateTooltips()
{
	ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK, -1, -1);
}

void UIScene_MiniGameSelectMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	if(m_bIgnorePress) return;

	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
			navigateBack();
			handled = true;
		}
		break;
	case ACTION_MENU_OK:
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
	case ACTION_MENU_LEFT:
	case ACTION_MENU_RIGHT:
		sendInputToMovie(key, repeat, pressed, released);
		handled = true;
		break;
	}
}

void UIScene_MiniGameSelectMenu::handlePress(F64 controlId, F64 childId)
{
	if(m_bIgnorePress) return;

	ui.PlayUISFX(eSFX_Press);

	switch((int)controlId)
	{
	case eControl_GameList:
		switch((int)childId)
		{
		case 0:
			LaunchMiniGame(MINIGAME_BATTLE);
			break;
		case 1:
			LaunchMiniGame(MINIGAME_TUMBLE);
			break;
		case 2:
			LaunchMiniGame(MINIGAME_GLIDE);
			break;
		}
		break;
	}
}

void UIScene_MiniGameSelectMenu::handleGainFocus(bool navBack)
{
	UIScene::handleGainFocus(navBack);
	m_bIgnorePress = false;
}

void UIScene_MiniGameSelectMenu::LaunchMiniGame(EMiniGameId miniGameId)
{
	Minecraft *pMinecraft = Minecraft::GetInstance();
	if(!pMinecraft)
		return;

	m_bIgnorePress = true;

	pMinecraft->SetupMiniGameInstance(MiniGameDef::GetCustomGameModeById(miniGameId, true), 0);

	MiniGameCreateMenuInitData *params = new MiniGameCreateMenuInitData();
	params->iPad = m_iPad;
	params->miniGameId = miniGameId;
	params->bOnline = FALSE;
	params->bIsPrivate = FALSE;

	ui.NavigateToScene(m_iPad, eUIScene_MiniGameCreateMenu, params);
}
