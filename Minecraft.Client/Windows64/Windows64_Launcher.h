#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <functional>

extern float g_sleepPercentage;
extern int g_autosaveInterval;

extern bool g_doBoatBreak;

class Windows64Launcher {
public:

	static void CreateLauncherWindow(HINSTANCE hInstance, std::function<void()> onLaunch);

	static void SaveAuthenticationData(const std::string& token, const std::string& refreshToken);
	static bool GetAuthenticationData(std::string& tokenOut, std::string& refreshTokenOut, bool dedicated = false);
	static bool GetAuthenticationDataAndLoad(bool dedicated = false);

	static int API_GetAccountInfo(const std::string token);
	static int API_AttemptAccountRegister(const std::string username, const std::string password, std::string& tokenOut);
	static int API_AttemptAccountLogin(const std::string username, const std::string password, std::string& tokenOut);

	static bool IsInOfflineMode();

	static std::vector<std::wstring> GetBannedUsersList();
	static void SaveBannedUsersList(std::vector<std::wstring> bannedUsers);

	static const std::string& GetAuthenticationToken();
	static const std::string& GetUsername();


};