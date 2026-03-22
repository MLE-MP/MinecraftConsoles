#include "stdafx.h"
#include "UI.h"
#if defined(__PS3__) || defined(__ORBIS__)
#include "Common\Network\Sony\SonyCommerce.h"
#endif
#include "UIScene_DLCMainMenu.h"

#define PLAYER_ONLINE_TIMER_ID 0
#define PLAYER_ONLINE_TIMER_TIME 100

UIScene_DLCMainMenu::UIScene_DLCMainMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();
	// Alert the app the we want to be informed of ethernet connections
	//app.SetLiveLinkRequired( true );

	m_labelOffers.init(UIString("Join Servers"));
	m_buttonListOffers.init(eControl_OffersList);

	m_Timer.setVisible(false); //use this while we query friends from the server, async

	//m_buttonListOffers.addItem(UIString("Add Friend"), 0);
	//m_buttonListOffers.addItem(UIString(""), 1);

	//for (int i = 0; i < 5; i++) {
	//	m_buttonListOffers.addItem(UIString("Custom String " + std::to_string(i)), i);
	//}
	//m_buttonListOffers.addItem(IDS_DLC_MENU_TEXTUREPACKS,e_DLC_TexturePacks);
	//m_buttonListOffers.addItem(IDS_DLC_MENU_MASHUPPACKS, e_DLC_MashupPacks);

	//app.AddDLCRequest(e_Marketplace_Content); // content is skin packs, texture packs and mash-up packs
	// we also need to mount the local DLC so we can tell what's been purchased
	//app.StartInstallDLCProcess(iPad);
	
	TelemetryManager->RecordMenuShown(iPad, eUIScene_DLCMainMenu, 0);
}

UIScene_DLCMainMenu::~UIScene_DLCMainMenu()
{
	// Alert the app the we no longer want to be informed of ethernet connections
	app.SetLiveLinkRequired( false );
	//app.FreeLocalDLCImages();

	//setLanguageOverride(true);
}

wstring UIScene_DLCMainMenu::getMoviePath()
{
	return L"DLCMainMenu";
}

void UIScene_DLCMainMenu::updateTooltips()
{
	ui.SetTooltips( m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK );
}

void UIScene_DLCMainMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	//app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d, down- %s, pressed- %s, released- %s\n", iPad, key, down?"TRUE":"FALSE", pressed?"TRUE":"FALSE", released?"TRUE":"FALSE");
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
			navigateBack();
		}
		break;
	case ACTION_MENU_OK:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
	case ACTION_MENU_LEFT:
	case ACTION_MENU_RIGHT:
	case ACTION_MENU_PAGEUP:
	case ACTION_MENU_PAGEDOWN:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_DLCMainMenu::handlePress(F64 controlId, F64 childId)
{
	switch(static_cast<int>(controlId))
	{
	case eControl_OffersList:
		{
			int iIndex = static_cast<int>(childId);
			DLCOffersParam *param = new DLCOffersParam();
			param->iPad = m_iPad;

			param->iType = iIndex;


			killTimer(PLAYER_ONLINE_TIMER_ID);
			//ui.NavigateToScene(m_iPad, eUIScene_DLCOffersMenu, param);
			break;
		}
	};
}

void UIScene_DLCMainMenu::handleTimerComplete(int id)
{

}

int UIScene_DLCMainMenu::ExitDLCMainMenu(void *pParam,int iPad,C4JStorage::EMessageResult result)
{
	UIScene_DLCMainMenu* pClass = static_cast<UIScene_DLCMainMenu *>(pParam);

	pClass->navigateBack();

	return 0;
}

void UIScene_DLCMainMenu::handleGainFocus(bool navBack)
{
	UIScene::handleGainFocus(navBack);

	updateTooltips();

}

void UIScene_DLCMainMenu::tick()
{
	UIScene::tick();
}

