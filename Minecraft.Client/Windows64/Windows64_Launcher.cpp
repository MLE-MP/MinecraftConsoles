#include "stdafx.h"

#include "Windows64_Launcher.h"
#include "Xbox/resource.h"

#include "Network/WinsockNetLayer.h"

#include <shlobj.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <regex>

#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>
#include <commdlg.h>

#include "discord_rpc.h"

float g_sleepPercentage = 100;
int g_autosaveInterval = 120;

bool g_doBoatBreak = true;

DWORD WINAPI DiscordRPCThreadFunc(LPVOID lpParam) {

	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));

	//dont handle any of these, if rpc disconnects it wont reconnect, but if someone doesnt have discord it wont loop reconnects
	//handlers.ready = handleDiscordReady;
	//handlers.disconnected = handleDiscordDisconnected;
	//handlers.errored = handleDiscordError;

	const char* applicationId = "1424889204110004316";

	Discord_Initialize(applicationId, &handlers, 1, NULL);

	DiscordRichPresence presence;
	memset(&presence, 0, sizeof(presence));
	
	//presence.state = "testing"; //todo: show if in server or single player
	//presence.details = "test2"; //todo: show dimention or some active state like crafting, exploring the overworld
	presence.startTimestamp = time(0);
	presence.largeImageKey = "MCLE";
	presence.largeImageText = "MCLE";

	//todo: maybe show dimention as small image instead of in details?
	//presence.smallImageKey = "MCLE"; 
	//presence.smallImageText = "MCLE";

	Discord_UpdatePresence(&presence);

	while (true) {
		Discord_RunCallbacks();
		Sleep(2000);
	}

	Discord_Shutdown();
	return 0;
}

static  ATOM RegisterLauncherClass(HINSTANCE hInstance);
static BOOL InitWindow(HINSTANCE hInstance);

HWND launcher_HWND = NULL;
bool shouldContinue = true;

HWND hBottomBar;
HWND hStatusText;
HWND hBtnOffline;
HWND hBtnLaunch;

HWND hBtnLogout;

HWND hBtnDiscord;
HWND hBtnImportSkin;

HWND hBtnRegister;
HWND hBtnLogin;

HWND hUsernameLabel;
HWND hUsernameEdit;
HWND hPasswordLabel;
HWND hPasswordEdit;

HWND hLoginUsernameLabel;

std::string username = "";
std::string authenticationToken = "";

bool startedThread = false;

bool offlinemode = false;
bool Windows64Launcher::IsInOfflineMode() { return offlinemode; }

void onSuccessfulLogin();
void onLoginFailed();
void AttemptFullLoginFlow();

std::vector<std::wstring> Windows64Launcher::GetBannedUsersList() {
	wchar_t exePath[MAX_PATH] = {};
	GetModuleFileNameW(NULL, exePath, MAX_PATH);

	wchar_t* lastSlash = wcsrchr(exePath, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}

	wchar_t filePath[MAX_PATH] = {};
	_snwprintf_s(filePath, MAX_PATH, _TRUNCATE, L"%sWindows64\\GameHDD\\banned_players.txt", exePath);

	std::vector<std::wstring> banned;

	std::wifstream file(filePath);

	if (!file.is_open())
		return banned;

	std::wstring line;
	while (std::getline(file, line))
	{
		if (!line.empty())
			banned.push_back(line);
	}

	file.close();

	return banned;
}

void Windows64Launcher::SaveBannedUsersList(std::vector<std::wstring> bannedUsers) {
	wchar_t exePath[MAX_PATH] = {};
	GetModuleFileNameW(NULL, exePath, MAX_PATH);

	wchar_t* lastSlash = wcsrchr(exePath, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}

	wchar_t filePath[MAX_PATH] = {};
	_snwprintf_s(filePath, MAX_PATH, _TRUNCATE, L"%sWindows64\\GameHDD\\banned_players.txt", exePath);

	std::wofstream file(filePath, std::ios::out | std::ios::trunc);

	if (!file.is_open())
		return;

	for (const auto& name : bannedUsers)
	{
		file << name << L"\n";
	}

	file.close();
}

const std::string& Windows64Launcher::GetAuthenticationToken() {
	return authenticationToken;
}

const std::string& Windows64Launcher::GetUsername() {
	return username;
}

void Windows64Launcher::CreateLauncherWindow(HINSTANCE hInstance, std::function<void()> onLaunch) {
	RegisterLauncherClass(hInstance);
	InitWindow(hInstance);

	CheckForUpdates();

	onLoginFailed(); //call this to disable login element during socket connection

	AttemptFullLoginFlow();

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Now launcher is closed -> continue
	if (shouldContinue == false && onLaunch)
		onLaunch();
}

void AttemptFullLoginFlow() {
	if (Windows64Launcher::GetAuthenticationData(authenticationToken, username)) {
		int responseState = Windows64Launcher::API_GetAccountInfo(authenticationToken);
		if (responseState == 0) {
			onSuccessfulLogin();
		} else {
			onLoginFailed();
			MessageBoxW(launcher_HWND, L"Unable To Connect To Saved Account", L"Login Failed", MB_OK);
		}
	}
	else {
		onLoginFailed();
	}
}

void onSuccessfulLogin() {
	SetWindowText(hPasswordEdit, "");
	SetWindowText(hLoginUsernameLabel, std::string("Welcome: " + username).c_str());

	ShowWindow(hBtnLaunch, SW_SHOW);
	ShowWindow(hBtnOffline, SW_SHOW);

	ShowWindow(hBtnLogout, SW_SHOW);
	ShowWindow(hLoginUsernameLabel, SW_SHOW);

	ShowWindow(hUsernameLabel, SW_HIDE);
	ShowWindow(hUsernameEdit, SW_HIDE);
	ShowWindow(hPasswordLabel, SW_HIDE);
	ShowWindow(hPasswordEdit, SW_HIDE);

	ShowWindow(hBtnRegister, SW_HIDE);
	ShowWindow(hBtnLogin, SW_HIDE);

	if (!startedThread) {
		HANDLE hThread = CreateThread(NULL, 0, DiscordRPCThreadFunc, NULL, 0, NULL);
		CloseHandle(hThread);

		startedThread = true;
	}
}

void onLoginFailed() {
	ShowWindow(hBtnLaunch, SW_HIDE);
	ShowWindow(hBtnOffline, SW_HIDE);

	ShowWindow(hBtnLogout, SW_HIDE);
	ShowWindow(hLoginUsernameLabel, SW_HIDE);

	ShowWindow(hUsernameLabel, SW_SHOW);
	ShowWindow(hUsernameEdit, SW_SHOW);
	ShowWindow(hPasswordLabel, SW_SHOW);
	ShowWindow(hPasswordEdit, SW_SHOW);

	ShowWindow(hBtnRegister, SW_SHOW);
	ShowWindow(hBtnLogin, SW_SHOW);
}



LRESULT OnWindowCreation(HWND hWnd) {
	hBottomBar = CreateWindowW(L"STATIC", nullptr, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, nullptr, nullptr, nullptr);

	hStatusText = CreateWindowW(L"STATIC", L"Version: " L"" LAUNCHER_VERSION, WS_CHILD | WS_VISIBLE, 10, 10, 200, 20, hBottomBar, nullptr, nullptr, nullptr);

	hBtnLaunch = CreateWindowW(L"BUTTON", L"Launch", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 0, 80, 25, hWnd, (HMENU)1, nullptr, nullptr);
	hBtnOffline = CreateWindowW(L"BUTTON", L"Play Offline", WS_CHILD | WS_VISIBLE, 0, 0, 80, 25, hWnd, (HMENU)2, nullptr, nullptr);

	hBtnRegister = CreateWindowW(L"BUTTON", L"Register", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 0, 80, 25, hWnd, (HMENU)3, nullptr, nullptr);
	hBtnLogin = CreateWindowW(L"BUTTON", L"Login", WS_CHILD | WS_VISIBLE, 0, 0, 80, 25, hWnd, (HMENU)4, nullptr, nullptr);

	hBtnLogout = CreateWindowW(L"BUTTON", L"Logout", WS_CHILD | WS_VISIBLE, 0, 0, 80, 25, hWnd, (HMENU)5, nullptr, nullptr);

	hBtnDiscord = CreateWindowW(L"BUTTON", L"Discord", WS_CHILD | WS_VISIBLE, 0, 0, 80, 25, hWnd, (HMENU)6, nullptr, nullptr);
	hBtnImportSkin = CreateWindowW(L"BUTTON", L"Import Skin", WS_CHILD | WS_VISIBLE, 0, 0, 90, 25, hWnd, (HMENU)7, nullptr, nullptr);

	hUsernameLabel = CreateWindowW(L"STATIC", L"Username:", WS_CHILD | WS_VISIBLE, 0, 0, 80, 20, hWnd, nullptr, nullptr, nullptr);
	hUsernameEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 0, 0, 180, 25, hWnd, nullptr, nullptr, nullptr);
	hPasswordLabel = CreateWindowW(L"STATIC", L"Password:", WS_CHILD | WS_VISIBLE, 0, 0, 80, 20, hWnd, nullptr, nullptr, nullptr);
	hPasswordEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD, 0, 0, 180, 25, hWnd, nullptr, nullptr, nullptr);

	hLoginUsernameLabel = CreateWindowW(L"STATIC", L"Welcome: Placeholder", WS_CHILD | WS_VISIBLE, 0, 0, 300, 20, hWnd, nullptr, nullptr, nullptr);


	return 0;
}

LRESULT OnWindowSize(int width, int height) {
	int inputWidth = 180;
	int inputHeight = 25;
	int labelWidth = 80;

	int centerX = width / 2 - inputWidth / 2;
	int startY = 40;

	MoveWindow(hLoginUsernameLabel, 0, 0, 300, 20, TRUE);
	MoveWindow(hBtnLogout, 0, 30, labelWidth, 20, TRUE);

	MoveWindow(hBtnDiscord, centerX, 125, labelWidth, 20, TRUE);
	MoveWindow(hBtnImportSkin, centerX + labelWidth + 10, 125, 90, 20, TRUE);

	// Username
	MoveWindow(hUsernameLabel, centerX - labelWidth + 50, startY - 5, labelWidth, 20, TRUE);
	MoveWindow(hUsernameEdit, centerX + 50, startY - 8, inputWidth, inputHeight, TRUE);

	// Password
	MoveWindow(hPasswordLabel, centerX - labelWidth + 50, startY + 40, labelWidth, 20, TRUE);
	MoveWindow(hPasswordEdit, centerX + 50, startY + 37, inputWidth, inputHeight, TRUE);

	int barHeight = 50;

	// Position bottom bar
	MoveWindow(hBottomBar, 0, height - barHeight, width, barHeight, TRUE);

	int padding = 10;
	int buttonWidth = 80;
	int buttonHeight = 25;

	int barTop = height - barHeight;
	int buttonY = barTop + (barHeight - buttonHeight) / 2;

	MoveWindow(hBtnLaunch, width - padding - buttonWidth, buttonY, buttonWidth, buttonHeight, TRUE);
	MoveWindow(hBtnLogin, width - padding - buttonWidth, buttonY, buttonWidth, buttonHeight, TRUE);

	MoveWindow(hBtnRegister, width - padding * 2 - buttonWidth * 2, buttonY, buttonWidth, buttonHeight, TRUE);
	MoveWindow(hBtnOffline, width - padding * 2 - buttonWidth * 2, buttonY, buttonWidth, buttonHeight, TRUE);

	MoveWindow(hStatusText, padding, (barHeight - 20) / 2, 300, 20, TRUE);

	return 0;
}

bool isValidUsername(const std::string& str) {
	std::regex pattern("^[A-Za-z0-9]+$");
	return std::regex_match(str, pattern);
}

bool IsValidPassword(const std::string& password) {
	std::regex pattern("^[A-Za-z0-9%$#@&]+$");
	return std::regex_match(password, pattern);
}

LRESULT OnAccountRegister() {
	char username_buf[16];
	char password_buf[32];

	GetWindowText(hUsernameEdit, username_buf, 16);
	GetWindowText(hPasswordEdit, password_buf, 32);

	if (!isValidUsername(username_buf)) {
		MessageBoxW(launcher_HWND, L"A Username Can Only Consist Of Letters And Numbers", L"Registraction Failed", MB_OK);
		return 0;
	}

	if (!IsValidPassword(password_buf)) {
		MessageBoxW(launcher_HWND, L"A Password Can Only Consist Of Letters And Numbers And %$#@&", L"Registraction Failed", MB_OK);
		return 0;
	}

	int registerResponse = Windows64Launcher::API_AttemptAccountRegister(username_buf, password_buf, authenticationToken);

	if (registerResponse == 0) { //success
		Windows64Launcher::SaveAuthenticationData(authenticationToken, username);
		AttemptFullLoginFlow();
	}
	else if (registerResponse == 20000) {
		MessageBoxW(launcher_HWND, L"Failed to Connect To Server", L"Registraction Failed", MB_OK);
	}
	else if (registerResponse == 20500) {
		MessageBoxW(launcher_HWND, L"Failed to Connect To Server", L"Registraction Failed", MB_OK);
	}
	else if (registerResponse == 2222) {
		MessageBoxW(launcher_HWND, L"Invalid Username Characters", L"Registraction Failed", MB_OK);
	}
	else if (registerResponse == 2233) {
		MessageBoxW(launcher_HWND, L"Invalid Password Characters", L"Registraction Failed", MB_OK);
	}
	else if (registerResponse == 3333) {
		MessageBoxW(launcher_HWND, L"Username Taken", L"Registraction Failed", MB_OK);
	}
	else if (registerResponse == 5555) {
		MessageBoxW(launcher_HWND, L"VPN's Are Not Allowed", L"Registraction Failed", MB_OK);
	}
	else if (registerResponse == 6666) {
		MessageBoxW(launcher_HWND, L"Too many accounts from this IP", L"Registraction Failed", MB_OK);
	}
	else if (registerResponse == 7777) {
		MessageBoxW(launcher_HWND, L"Username Contains Banned Word", L"Registraction Failed", MB_OK);
	}
	else if (registerResponse == 8888) {
		MessageBoxW(launcher_HWND, L"Username Is Banned", L"Registraction Failed", MB_OK);
	}
	else { //unknown error, we will setup internal codes and have them logged here
		MessageBoxW(launcher_HWND, std::wstring(L"Unknown Error: " + std::to_wstring(registerResponse)).c_str(), L"Registraction Failed", MB_OK);
	}

	return 0;
}

LRESULT OnAccountLogin() {
	char username_buf[16];
	char password_buf[32];

	GetWindowText(hUsernameEdit, username_buf, 16);
	GetWindowText(hPasswordEdit, password_buf, 32);

	if (!isValidUsername(username_buf)) {
		MessageBoxW(launcher_HWND, L"A Username Can Only Consist Of Letters And Numbers", L"Login Failed", MB_OK);
		return 0;
	}

	if (!IsValidPassword(password_buf)) {
		MessageBoxW(launcher_HWND, L"A Password Can Only Consist Of Letters And Numbers And %$#@&", L"Login Failed", MB_OK);
		return 0;
	}

	int registerResponse = Windows64Launcher::API_AttemptAccountLogin(username_buf, password_buf, authenticationToken);

	if (registerResponse == 0) { //success
		Windows64Launcher::SaveAuthenticationData(authenticationToken, username);
		AttemptFullLoginFlow();
	}
	else if (registerResponse == 1111 || registerResponse == 2222) { //invalid details
		MessageBoxW(launcher_HWND, L"Invalid Username / Password", L"Login Failed", MB_OK);
	}
	else if (registerResponse == 2) { //Banned
		MessageBoxW(launcher_HWND, L"You Have Been Banned", L"Login Failed", MB_OK);
	}
	else { //unknown error, we will setup internal codes and have them logged here
		MessageBoxW(launcher_HWND, std::wstring(L"Unknown Error: " + std::to_wstring(registerResponse)).c_str(), L"Registraction Failed", MB_OK);
	}

	return 0;
}

LRESULT OnCommandReceived(HWND hWnd, int type) {
	switch (type) {
	case 1: // Launch
		shouldContinue = false;
		offlinemode = false;
		DestroyWindow(hWnd);
		break;
	case 2: // Offline
		shouldContinue = false;
		offlinemode = true;
		DestroyWindow(hWnd);
		break;
	case 3: // Register
		OnAccountRegister();
		break;
	case 4: // Login
		OnAccountLogin();
		break;
	case 5: //Logout
		Windows64Launcher::SaveAuthenticationData("", username);
		onLoginFailed();
		break;
	case 6: //Discord
	{
		ShellExecute(0, 0, "https://discord.gg/xjc9JW4Bfp", 0, 0 , SW_SHOW );
	}
	break;
	case 7: //Import Skin
	{
		wchar_t filePath[MAX_PATH] = { 0 };

		OPENFILENAMEW ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFilter = L"PNG Files\0*.png\0";
		ofn.lpstrFile = filePath;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = L"Select a Skin";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		if (!GetOpenFileNameW(&ofn))
			break;

		FILE* imgFile = _wfopen(filePath, L"rb");
		if (!imgFile) {
			MessageBoxW(hWnd, L"Failed to open image file.", L"Import Error", MB_OK | MB_ICONERROR);
			break;
		}

		unsigned char pngHeader[24];
		bool validSize = false;
		if (fread(pngHeader, 1, 24, imgFile) == 24) {
			int w = (pngHeader[16] << 24) | (pngHeader[17] << 16) | (pngHeader[18] << 8) | pngHeader[19];
			int h = (pngHeader[20] << 24) | (pngHeader[21] << 16) | (pngHeader[22] << 8) | pngHeader[23];
			if (w == 64 && h == 32)
				validSize = true;
		}
		fclose(imgFile);

		if (!validSize) {
			MessageBoxW(hWnd, L"Skin must be 64x32.", L"Invalid Skin", MB_OK | MB_ICONERROR);
			break;
		}

		CreateDirectoryW(L"Windows64\\CustomSkins", NULL);

		std::wstring fileName = filePath;
		size_t lastSlash = fileName.find_last_of(L"\\//");
		if (lastSlash != std::wstring::npos)
			fileName = fileName.substr(lastSlash + 1);

		std::wstring destPath = L"Windows64\\CustomSkins\\" + fileName;

		if (!CopyFileW(filePath, destPath.c_str(), FALSE)) {
			MessageBoxW(hWnd, L"Failed to copy skin file.", L"Import Error", MB_OK | MB_ICONERROR);
			break;
		}

		MessageBoxW(hWnd, L"Skin imported! It will be available next time you launch the game.", L"Import Skin", MB_OK | MB_ICONINFORMATION);
	}
	break;
	}
	return 0;
}

LRESULT CALLBACK WndProc_Launcher(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_COMMAND:
		return OnCommandReceived(hWnd, LOWORD(wParam));
	case WM_SIZE:
		return OnWindowSize(LOWORD(lParam), HIWORD(lParam));
	case WM_CREATE:
		return OnWindowCreation(hWnd);
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

static ATOM RegisterLauncherClass(HINSTANCE hInstance) {
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc_Launcher;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, "Minecraft");
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = "Legacy Launcher";
	wcex.lpszClassName = "LCELauncherClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MINECRAFTWINDOWS));

	return RegisterClassEx(&wcex);
}

static BOOL InitWindow(HINSTANCE hInstance) {

	RECT wr = { 0, 0, 300, 200 };    // set the size, but not the position
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);    // adjust the size

	launcher_HWND = CreateWindow("LCELauncherClass",
		"Legacy Launcher",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		wr.right - wr.left,    // width of the window
		wr.bottom - wr.top,    // height of the window
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!launcher_HWND) {
		return FALSE;
	}

	ShowWindow(launcher_HWND, true);
	UpdateWindow(launcher_HWND);

	return TRUE;
}


std::string GetOrCreateLauncherFolder() {
	char path[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path);
	std::string folder = path;
	folder += "\\LegacyLauncher";

	CreateDirectoryA(folder.c_str(), NULL);

	return folder;
}

bool loadedDedicated = false;

void Windows64Launcher::SaveAuthenticationData(const std::string& token, const std::string& username) {
	if (loadedDedicated) return;

	std::string folder = GetOrCreateLauncherFolder();
	std::string file = folder + "\\AccountToken_DoNotShare.dat";

	std::string data = token + "\n" + username;

	std::ofstream out(file, std::ios::trunc);
	out << data;
	out.close();
}

bool Windows64Launcher::GetAuthenticationData(std::string& tokenOut, std::string& usernameOut, bool dedicated)
{
	std::ifstream in;

	if (dedicated) {
		wchar_t exePath[MAX_PATH] = {};
		GetModuleFileNameW(NULL, exePath, MAX_PATH);

		wchar_t* lastSlash = wcsrchr(exePath, L'\\');
		if (lastSlash)
			*(lastSlash + 1) = L'\0';

		wchar_t filePath[MAX_PATH] = {};
		_snwprintf_s(filePath, MAX_PATH, _TRUNCATE, L"%sWindows64\\AccountToken_DoNotShare.dat", exePath);

		in.open(filePath);

		loadedDedicated = true;
	}

	if (!in.is_open()) {
		std::string folder = GetOrCreateLauncherFolder();
		std::string file = folder + "\\AccountToken_DoNotShare.dat";

		in.open(file);
	}

	if (!in.is_open())
		return false;

	std::getline(in, tokenOut);
	std::getline(in, usernameOut);
	in.close();

	if (tokenOut.empty())
		return false;

	return true;
}

bool Windows64Launcher::GetAuthenticationDataAndLoad(bool dedicated) {
	return Windows64Launcher::GetAuthenticationData(authenticationToken, username, dedicated);
}

std::vector<std::string> split(const std::string& str, char delimiter) {
	std::vector<std::string> result;
	size_t start = 0;
	size_t end = 0;

	while ((end = str.find(delimiter, start)) != std::string::npos) {
		result.push_back(str.substr(start, end - start));
		start = end + 1;
	}

	// Add the last substring
	result.push_back(str.substr(start));
	return result;
}

int Windows64Launcher::API_GetAccountInfo(const std::string token) {
	std::vector<std::wstring> headers;
	headers.push_back(L"Content-Type: text/plain");

	HttpResponse response = WinsockNetLayer::DoWinHttpRequest(L"/getAccountInfo", L"POST", token, headers);

	if (response.status == 0) return -1;

	if (response.status != 200) return (20000 + response.status);

	if (response.body.find('-') == std::string::npos) return stoi(response.body);

	username = response.body.erase(0, 1);

	return 0;
}

int Windows64Launcher::API_AttemptAccountRegister(const std::string _username, const std::string password, std::string& tokenOut) {
	std::vector<std::wstring> headers;
	headers.push_back(L"Content-Type: text/plain");

	std::string data = _username + ":" + password;

	HttpResponse response = WinsockNetLayer::DoWinHttpRequest(L"/accountRegistration", L"POST", data, headers);

	if (response.status != 200) return (20000 + response.status);

	if (response.body.find('-') == std::string::npos) return stoi(response.body);

	std::vector<std::string> splitData = split(response.body.erase(0, 1), ':');

	username = splitData[0];
	authenticationToken = splitData[1];

	return 0;
}

int Windows64Launcher::API_AttemptAccountLogin(const std::string _username, const std::string password, std::string& tokenOut)
{
	std::vector<std::wstring> headers;
	headers.push_back(L"Content-Type: text/plain");

	std::string data = _username + ":" + password;

	HttpResponse response = WinsockNetLayer::DoWinHttpRequest(L"/accountLogin", L"POST", data, headers);

	if (response.status != 200) return (20000 + response.status);

	if (response.body.find('-') == std::string::npos) return stoi(response.body);

	std::vector<std::string> splitData = split(response.body.erase(0, 1), ':');

	username = splitData[0];
	authenticationToken = splitData[1];

	return 0;
}

static std::string GitHubGet(const wchar_t* path) {
	HINTERNET hSession = WinHttpOpen(L"MCLegacy-Updater/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) return "";

	HINTERNET hConnect = WinHttpConnect(hSession, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

	BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0);
	if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

	std::string body;
	if (bResults) {
		DWORD dwSize = 0;
		do {
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
			if (dwSize == 0) break;
			char* buf = new char[dwSize + 1];
			DWORD dwRead = 0;
			ZeroMemory(buf, dwSize + 1);
			if (WinHttpReadData(hRequest, buf, dwSize, &dwRead))
				body.append(buf, dwRead);
			delete[] buf;
		} while (dwSize > 0);
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return body;
}

static bool DownloadFile(const wchar_t* host, const wchar_t* path, const std::string& destPath) {
	HINTERNET hSession = WinHttpOpen(L"MCLegacy-Updater/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) return false;

	HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

	BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0);
	if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

	if (!bResults) {
		WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
		return false;
	}

	DWORD dwStatusCode = 0;
	DWORD dwSz = sizeof(dwStatusCode);
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSz, WINHTTP_NO_HEADER_INDEX);

	if (dwStatusCode == 301 || dwStatusCode == 302) {
		DWORD dwBufLen = 0;
		WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwBufLen, WINHTTP_NO_HEADER_INDEX);
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && dwBufLen > 0) {
			wchar_t* locBuf = new wchar_t[dwBufLen / sizeof(wchar_t) + 1];
			if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX, locBuf, &dwBufLen, WINHTTP_NO_HEADER_INDEX)) {
				std::wstring location(locBuf);
				delete[] locBuf;

				WinHttpCloseHandle(hRequest);
				WinHttpCloseHandle(hConnect);
				WinHttpCloseHandle(hSession);

				URL_COMPONENTS urlComp = {};
				urlComp.dwStructSize = sizeof(urlComp);
				wchar_t hostBuf[256] = {};
				wchar_t pathBuf[2048] = {};
				urlComp.lpszHostName = hostBuf;
				urlComp.dwHostNameLength = 256;
				urlComp.lpszUrlPath = pathBuf;
				urlComp.dwUrlPathLength = 2048;

				if (WinHttpCrackUrl(location.c_str(), (DWORD)location.length(), 0, &urlComp))
					return DownloadFile(hostBuf, pathBuf, destPath);
				return false;
			}
			delete[] locBuf;
		}
		WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
		return false;
	}

	if (dwStatusCode != 200) {
		WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
		return false;
	}

	FILE* f = fopen(destPath.c_str(), "wb");
	if (!f) {
		WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
		return false;
	}

	DWORD dwSize = 0;
	do {
		dwSize = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
		if (dwSize == 0) break;
		char* buf = new char[dwSize];
		DWORD dwRead = 0;
		if (WinHttpReadData(hRequest, buf, dwSize, &dwRead))
			fwrite(buf, 1, dwRead, f);
		delete[] buf;
	} while (dwSize > 0);

	fclose(f);
	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return true;
}

static std::string ExtractJsonString(const std::string& json, const std::string& key) {
	std::string search = "\"" + key + "\"";
	size_t pos = json.find(search);
	if (pos == std::string::npos) return "";
	pos = json.find("\"", pos + search.length() + 1);
	if (pos == std::string::npos) return "";
	pos++;
	size_t end = json.find("\"", pos);
	if (end == std::string::npos) return "";
	return json.substr(pos, end - pos);
}

void Windows64Launcher::CheckForUpdates() {
	SetWindowTextW(hStatusText, L"Checking for updates...");

	std::string response = GitHubGet(L"/repos/mclemp/MinecraftConsoles/releases/latest");
	if (response.empty()) {
		SetWindowTextW(hStatusText, L"Version: " L"" LAUNCHER_VERSION L" (update check failed)");
		return;
	}

	std::string latestTag = ExtractJsonString(response, "tag_name");
	if (latestTag.empty()) {
		SetWindowTextW(hStatusText, L"Version: " L"" LAUNCHER_VERSION);
		return;
	}

	if (latestTag == LAUNCHER_VERSION) {
		SetWindowTextW(hStatusText, L"Version: " L"" LAUNCHER_VERSION L" (up to date)");
		return;
	}

	SetWindowTextW(hStatusText, L"Downloading update...");

	std::string downloadUrl;
	size_t assetsPos = response.find("\"assets\"");
	if (assetsPos != std::string::npos) {
		size_t searchFrom = assetsPos;
		size_t urlPos = response.find("\"browser_download_url\"", searchFrom);
		if (urlPos != std::string::npos) {
			size_t valStart = response.find("\"", urlPos + 22);
			if (valStart != std::string::npos) {
				valStart++;
				size_t valEnd = response.find("\"", valStart);
				if (valEnd != std::string::npos)
					downloadUrl = response.substr(valStart, valEnd - valStart);
			}
		}
	}

	if (downloadUrl.empty()) {
		MessageBoxW(launcher_HWND, L"No downloadable asset found in the release.", L"Update Error", MB_OK | MB_ICONERROR);
		SetWindowTextW(hStatusText, L"Version: " L"" LAUNCHER_VERSION);
		return;
	}

	std::wstring wUrl(downloadUrl.begin(), downloadUrl.end());
	URL_COMPONENTS urlComp = {};
	urlComp.dwStructSize = sizeof(urlComp);
	wchar_t hostBuf[256] = {};
	wchar_t pathBuf[2048] = {};
	urlComp.lpszHostName = hostBuf;
	urlComp.dwHostNameLength = 256;
	urlComp.lpszUrlPath = pathBuf;
	urlComp.dwUrlPathLength = 2048;

	if (!WinHttpCrackUrl(wUrl.c_str(), (DWORD)wUrl.length(), 0, &urlComp)) {
		MessageBoxW(launcher_HWND, L"Failed to parse download URL.", L"Update Error", MB_OK | MB_ICONERROR);
		SetWindowTextW(hStatusText, L"Version: " L"" LAUNCHER_VERSION);
		return;
	}

	char tempPath[MAX_PATH];
	GetTempPathA(MAX_PATH, tempPath);
	std::string zipPath = std::string(tempPath) + "mclegacy_update.zip";

	if (!DownloadFile(hostBuf, pathBuf, zipPath)) {
		MessageBoxW(launcher_HWND, L"Failed to download update.", L"Update Error", MB_OK | MB_ICONERROR);
		SetWindowTextW(hStatusText, L"Version: " L"" LAUNCHER_VERSION);
		return;
	}

	SetWindowTextW(hStatusText, L"Installing update...");

	char exePath[MAX_PATH] = {};
	GetModuleFileNameA(NULL, exePath, MAX_PATH);

	char exeDir[MAX_PATH] = {};
	strcpy_s(exeDir, exePath);
	char* pSlash = strrchr(exeDir, '\\');
	if (pSlash) *(pSlash + 1) = '\0';

	char batPath[MAX_PATH];
	sprintf_s(batPath, "%smclegacy_update.bat", tempPath);

	FILE* bat = fopen(batPath, "w");
	if (!bat) {
		MessageBoxW(launcher_HWND, L"Failed to create update script.", L"Update Error", MB_OK | MB_ICONERROR);
		SetWindowTextW(hStatusText, L"Version: " L"" LAUNCHER_VERSION);
		return;
	}

	fprintf(bat, "@echo off\n");
	fprintf(bat, "echo Waiting for application to close...\n");
	fprintf(bat, "timeout /t 2 /nobreak >nul\n");
	fprintf(bat, "echo Extracting update...\n");
	fprintf(bat, "powershell -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"\n", zipPath.c_str(), exeDir);
	fprintf(bat, "echo Update complete. Restarting...\n");
	fprintf(bat, "start \"\" \"%s\"\n", exePath);
	fprintf(bat, "del \"%%~f0\"\n");
	fclose(bat);

	ShellExecuteA(NULL, "open", batPath, NULL, tempPath, SW_SHOW);

	shouldContinue = true;
	DestroyWindow(launcher_HWND);
	ExitProcess(0);
}