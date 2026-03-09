#include "stdafx.h"
#include "UI.h"
#include "UIScene_MiniGameCreateMenu.h"
#include "..\..\Minecraft.h"
#include "..\..\MinecraftServer.h"
#include "..\..\Options.h"
#include "..\..\..\Minecraft.World\MiniGameDef.h"
#include "..\..\..\Minecraft.World\EMiniGameId.h"
#include "..\..\..\Minecraft.World\LevelSettings.h"
#include "..\GameRules\LevelGenerationOptions.h"
#include "..\DLC\DLCManager.h"
#include "..\DLC\DLCPack.h"
#include "..\DLC\DLCGameRulesHeader.h"

UIScene_MiniGameCreateMenu::UIScene_MiniGameCreateMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	initialiseMovie();

	m_bIgnoreInput = false;
	m_iPad = iPad;
	m_iGameType = 0;

	MiniGameCreateMenuInitData *params = (MiniGameCreateMenuInitData *)initData;
	if(params)
	{
		m_miniGameId = params->miniGameId;
		m_MoreOptionsParams.bOnlineGame = params->bOnline;
		m_MoreOptionsParams.bInviteOnly = params->bIsPrivate;
	}
	else
	{
		m_miniGameId = MINIGAME_BATTLE;
		m_MoreOptionsParams.bOnlineGame = FALSE;
		m_MoreOptionsParams.bInviteOnly = FALSE;
	}

	m_MoreOptionsParams.bAllowFriendsOfFriends = TRUE;
	m_MoreOptionsParams.bPVP = TRUE;
	m_MoreOptionsParams.bFireSpreads = FALSE;
	m_MoreOptionsParams.bTNT = FALSE;
	m_MoreOptionsParams.bTrust = FALSE;
	m_MoreOptionsParams.bHostPrivileges = FALSE;
	m_MoreOptionsParams.bStructures = FALSE;
	m_MoreOptionsParams.bFlatWorld = FALSE;
	m_MoreOptionsParams.bBonusChest = FALSE;
	m_MoreOptionsParams.bGenerateOptions = FALSE;
	m_MoreOptionsParams.iPad = iPad;

	m_MoreOptionsParams.bMobGriefing = false;
	m_MoreOptionsParams.bKeepInventory = false;
	m_MoreOptionsParams.bDoMobSpawning = false;
	m_MoreOptionsParams.bDoMobLoot = false;
	m_MoreOptionsParams.bDoTileDrops = false;
	m_MoreOptionsParams.bNaturalRegeneration = false;
	m_MoreOptionsParams.bDoDaylightCycle = false;

	m_labelWorldName.init(GetMiniGameName(m_miniGameId));

	m_editWorldName.setVisible(false);
	m_texturePackSelector.setVisible(false);
	m_sliderDifficulty.setVisible(false);

	m_buttonGameType.init(L"Game Type: Casual", eControl_GameType);
	m_buttonMoreOptions.init(app.GetString(IDS_MORE_OPTIONS), eControl_MoreOptions);
	m_buttonCreateGame.init(L"Create Game", eControl_CreateGame);

	bool bMultiplayerAllowed = ProfileManager.IsSignedInLive(m_iPad) && ProfileManager.AllowedToPlayMultiplayer(m_iPad);
	m_checkboxOnline.SetEnable(bMultiplayerAllowed);
	m_checkboxOnline.init(app.GetString(IDS_ONLINE_GAME), eControl_OnlineGame, m_MoreOptionsParams.bOnlineGame ? true : false);

	if(!ProfileManager.IsSignedInLive(m_iPad))
	{
		m_checkboxOnline.SetEnable(false);
	}

	doHorizontalResizeCheck();
}

UIScene_MiniGameCreateMenu::~UIScene_MiniGameCreateMenu()
{
}

wstring UIScene_MiniGameCreateMenu::getMoviePath()
{
	return L"CreateWorldMenu";
}

void UIScene_MiniGameCreateMenu::updateTooltips()
{
	ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK);
}

void UIScene_MiniGameCreateMenu::updateComponents()
{
	m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, true);
	m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
}

void UIScene_MiniGameCreateMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	if(m_bIgnoreInput) return;

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

		{
			bool bOnlineGame = m_checkboxOnline.IsChecked();
			if(m_MoreOptionsParams.bOnlineGame != bOnlineGame)
			{
				m_MoreOptionsParams.bOnlineGame = bOnlineGame;
				if(!m_MoreOptionsParams.bOnlineGame)
				{
					m_MoreOptionsParams.bInviteOnly = false;
					m_MoreOptionsParams.bAllowFriendsOfFriends = false;
				}
			}
		}

		handled = true;
		break;
	}
}

void UIScene_MiniGameCreateMenu::handlePress(F64 controlId, F64 childId)
{
	if(m_bIgnoreInput) return;

	ui.PlayUISFX(eSFX_Press);

	switch((int)controlId)
	{
	case eControl_GameType:
		m_iGameType = (m_iGameType + 1) % 2;
		if(m_iGameType == 0)
			m_buttonGameType.setLabel(L"Game Type: Casual");
		else
			m_buttonGameType.setLabel(L"Game Type: Competitive");
		break;
	case eControl_MoreOptions:
		ui.NavigateToScene(m_iPad, eUIScene_LaunchMoreOptionsMenu, &m_MoreOptionsParams);
		break;
	case eControl_CreateGame:
		m_bIgnoreInput = true;
		CreateMiniGame(this);
		break;
	}
}

void UIScene_MiniGameCreateMenu::handleGainFocus(bool navBack)
{
	UIScene::handleGainFocus(navBack);
	if(navBack)
	{
		m_checkboxOnline.setChecked(m_MoreOptionsParams.bOnlineGame);
	}
	m_bIgnoreInput = false;
}

const wchar_t *UIScene_MiniGameCreateMenu::GetMiniGameName(EMiniGameId id)
{
	switch(id)
	{
	case MINIGAME_BATTLE: return L"Battle";
	case MINIGAME_TUMBLE: return L"Tumble";
	case MINIGAME_GLIDE:  return L"Glide";
	default:              return L"Mini Game";
	}
}

void UIScene_MiniGameCreateMenu::CreateMiniGame(UIScene_MiniGameCreateMenu *pClass)
{
	bool isClientSide = ProfileManager.IsSignedInLive(ProfileManager.GetPrimaryPad()) && pClass->m_MoreOptionsParams.bOnlineGame;
	bool isPrivate = pClass->m_MoreOptionsParams.bInviteOnly ? true : false;

	app.ClearTerrainFeaturePosition();

	StorageManager.ResetSaveData();
	StorageManager.SetSaveTitle((wchar_t *)GetMiniGameName(pClass->m_miniGameId));

	NetworkGameInitData *param = new NetworkGameInitData();
	param->levelName = GetMiniGameName(pClass->m_miniGameId);
	param->seed = 0;
	param->findSeed = true;
	param->saveData = NULL;
	param->texturePackId = 0;

	MiniGameDef &miniGameDef = MiniGameDef::GetCustomGameModeById(pClass->m_miniGameId, true);
	int difficulty = miniGameDef.getDifficulty();

	Minecraft *mc = Minecraft::GetInstance();
	if(mc)
		mc->SetupMiniGameInstance(miniGameDef, 0);

	// load minigame map pack (PCK + MCS)
	{
		wstring mapFolder;
		switch(pClass->m_miniGameId)
		{
		case MINIGAME_BATTLE:  mapFolder = L"Battle";  break;
		case MINIGAME_TUMBLE:  mapFolder = L"Tumble";  break;
		case MINIGAME_GLIDE:   mapFolder = L"Glide";   break;
		default:               mapFolder = L"Battle";  break;
		}

		wstring basePath = L"Windows64Media\\Minigames\\" + mapFolder + L"\\";

		// find all pack subfolders
		WIN32_FIND_DATAW findDir;
		HANDLE hFindDir = FindFirstFileW((basePath + L"*").c_str(), &findDir);
		vector<wstring> packFolders;
		if(hFindDir != INVALID_HANDLE_VALUE)
		{
			do
			{
				if((findDir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(findDir.cFileName, L".") != 0 && wcscmp(findDir.cFileName, L"..") != 0)
					packFolders.push_back(findDir.cFileName);
			} while(FindNextFileW(hFindDir, &findDir));
			FindClose(hFindDir);
		}

		if(!packFolders.empty())
		{
			// pick a random pack
			int packIdx = rand() % (int)packFolders.size();
			wstring packPath = basePath + packFolders[packIdx] + L"\\";
			wstring pckPath = packPath + L"WorldPack.pck";

			app.DebugPrintf("Loading minigame pack: %ls\n", packPath.c_str());

			// parse the PCK via DLCManager
			DLCPack *pack = new DLCPack(packFolders[packIdx], 0xFFFFFFFF);
			DWORD dwFilesProcessed = 0;
			bool pckOk = app.m_dlcManager.readDLCDataFile(dwFilesProcessed, pckPath, pack);

			if(pckOk && dwFilesProcessed > 0)
			{
				app.m_dlcManager.addPack(pack);

				int grHeaderCount = pack->getDLCItemsCount(DLCManager::e_DLCType_GameRulesHeader);
				if(grHeaderCount > 0)
				{
					int startIdx = rand() % grHeaderCount;
					bool mapLoaded = false;

					for(int attempt = 0; attempt < grHeaderCount && !mapLoaded; ++attempt)
					{
						int mapIdx = (startIdx + attempt) % grHeaderCount;
						DLCGameRulesHeader *grHeader = (DLCGameRulesHeader *)pack->getFile(DLCManager::e_DLCType_GameRulesHeader, mapIdx);

						if(!grHeader || !grHeader->lgo)
							continue;

						LevelGenerationOptions *lgo = grHeader->lgo;
						wstring baseSaveName = lgo->getBaseSavePath();

						if(baseSaveName.empty())
						{
							param->levelGen = lgo;
							mapLoaded = true;
							app.DebugPrintf("Loaded procedural minigame rules from pack: %ls\n", packPath.c_str());
							break;
						}

						wstring mcsPath = packPath + L"Data\\" + baseSaveName;
						HANDLE hFile = CreateFileW(mcsPath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
						if(hFile != INVALID_HANDLE_VALUE)
						{
							DWORD dwFileSize = GetFileSize(hFile, NULL);
							PBYTE pbData = new BYTE[dwFileSize];
							DWORD bytesRead = 0;
							ReadFile(hFile, pbData, dwFileSize, &bytesRead, NULL);
							CloseHandle(hFile);

							lgo->setBaseSaveData(pbData, dwFileSize);
							param->levelGen = lgo;
							param->savePlatform = SAVE_FILE_PLATFORM_X360;
							mapLoaded = true;

							app.DebugPrintf("Loaded minigame map: %ls (%d bytes) with game rules\n", mcsPath.c_str(), dwFileSize);
						}
						else
						{
							app.DebugPrintf("MCS file not found, trying next: %ls\n", mcsPath.c_str());
						}
					}

					if(!mapLoaded)
					{
						char msg[512];
						sprintf_s(msg, "FATAL: No valid MCS map files found in minigame pack.\nPack: %ls\nTried %d header(s), none had a loadable MCS file on disk.", packPath.c_str(), grHeaderCount);
						app.DebugPrintf("%s\n", msg);
						MessageBoxA(NULL, msg, "Minigame Load Failed", MB_OK | MB_ICONERROR);
						__debugbreak();
					}
				}
				else
				{
					char msg[512];
					sprintf_s(msg, "FATAL: No game rules headers found in minigame pack.\nPack: %ls\nThe PCK was parsed but contains 0 GameRulesHeader entries.", packPath.c_str());
					app.DebugPrintf("%s\n", msg);
					MessageBoxA(NULL, msg, "Minigame Load Failed", MB_OK | MB_ICONERROR);
					__debugbreak();
				}
			}
			else
			{
				char msg[512];
				sprintf_s(msg, "FATAL: Failed to parse minigame PCK file.\nPath: %ls\npckOk=%d, dwFilesProcessed=%d", pckPath.c_str(), (int)pckOk, dwFilesProcessed);
				app.DebugPrintf("%s\n", msg);
				MessageBoxA(NULL, msg, "Minigame Load Failed", MB_OK | MB_ICONERROR);
				delete pack;
				__debugbreak();
			}
		}
		else
		{
			char msg[512];
			sprintf_s(msg, "FATAL: No map pack folders found for minigame.\nSearched: %ls\nNo subdirectories containing map data exist.", basePath.c_str());
			app.DebugPrintf("%s\n", msg);
			MessageBoxA(NULL, msg, "Minigame Load Failed", MB_OK | MB_ICONERROR);
			__debugbreak();
		}
	}

	app.SetGameHostOption(eGameHostOption_Difficulty, difficulty);
	app.SetGameHostOption(eGameHostOption_FriendsOfFriends, pClass->m_MoreOptionsParams.bAllowFriendsOfFriends);
	app.SetGameHostOption(eGameHostOption_Gamertags, app.GetGameSettings(pClass->m_iPad, eGameSetting_GamertagsVisible) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_BedrockFog, app.GetGameSettings(pClass->m_iPad, eGameSetting_BedrockFog) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_GameType, GameType::SURVIVAL->getId());
	app.SetGameHostOption(eGameHostOption_LevelType, FALSE);
	app.SetGameHostOption(eGameHostOption_Structures, FALSE);
	app.SetGameHostOption(eGameHostOption_BonusChest, FALSE);
	app.SetGameHostOption(eGameHostOption_PvP, TRUE);
	app.SetGameHostOption(eGameHostOption_TrustPlayers, FALSE);
	app.SetGameHostOption(eGameHostOption_FireSpreads, FALSE);
	app.SetGameHostOption(eGameHostOption_TNT, FALSE);
	app.SetGameHostOption(eGameHostOption_HostCanFly, FALSE);
	app.SetGameHostOption(eGameHostOption_HostCanChangeHunger, FALSE);
	app.SetGameHostOption(eGameHostOption_HostCanBeInvisible, FALSE);

	app.SetGameHostOption(eGameHostOption_MobGriefing, FALSE);
	app.SetGameHostOption(eGameHostOption_KeepInventory, FALSE);
	app.SetGameHostOption(eGameHostOption_DoMobSpawning, FALSE);
	app.SetGameHostOption(eGameHostOption_DoMobLoot, FALSE);
	app.SetGameHostOption(eGameHostOption_DoTileDrops, FALSE);
	app.SetGameHostOption(eGameHostOption_NaturalRegeneration, FALSE);
	app.SetGameHostOption(eGameHostOption_DoDaylightCycle, FALSE);

	app.SetGameHostOption(eGameHostOption_WasntSaveOwner, false);

#ifdef _LARGE_WORLDS
	app.SetGameHostOption(eGameHostOption_WorldSize, 1);
#endif

	g_NetworkManager.HostGame(0, isClientSide, isPrivate, MINECRAFT_NET_MAX_PLAYERS, 0);

	param->settings = app.GetGameHostOption(eGameHostOption_All);

#ifdef _LARGE_WORLDS
	param->xzSize = LEVEL_WIDTH_CLASSIC;
	param->hellScale = HELL_LEVEL_SCALE_CLASSIC;
#else
	param->xzSize = LEVEL_MAX_WIDTH;
	param->hellScale = HELL_LEVEL_MAX_SCALE;
#endif

#ifndef _XBOX
	g_NetworkManager.FakeLocalPlayerJoined();
#endif

	LoadingInputParams *loadingParams = new LoadingInputParams();
	loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
	loadingParams->lpParam = (LPVOID)param;

	app.SetAutosaveTimerTime();

	UIFullscreenProgressCompletionData *completionData = new UIFullscreenProgressCompletionData();
	completionData->bShowBackground = TRUE;
	completionData->bShowLogo = TRUE;
	completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;
	completionData->iPad = DEFAULT_XUI_MENU_USER;
	loadingParams->completionData = completionData;

	ui.NavigateToScene(pClass->m_iPad, eUIScene_FullscreenProgress, loadingParams);
}
