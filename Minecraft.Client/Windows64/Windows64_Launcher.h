#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <functional>

extern int g_autosaveInterval;

class Windows64Launcher {
public:
	static int GetViewDistance();


	static void CreateLauncherWindow(HINSTANCE hInstance, std::function<void()> onLaunch);

	static void SaveAuthenticationData(const std::string& token, const std::string& refreshToken);
	static bool GetAuthenticationData(std::string& tokenOut, std::string& refreshTokenOut);
	static bool GetAuthenticationDataAndSave();

	static int API_GetAccountInfo(const std::string token);
	static int API_AttemptAccountRegister(const std::string username, const std::string password, std::string& tokenOut);
	static int API_AttemptAccountLogin(const std::string username, const std::string password, std::string& tokenOut);

	static const std::string& GetAuthenticationToken();
	static const std::string& GetUsername();


};