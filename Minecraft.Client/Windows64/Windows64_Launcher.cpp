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

	hStatusText = CreateWindowW(L"STATIC", L"Version: 0.2", WS_CHILD | WS_VISIBLE, 10, 10, 200, 20, hBottomBar, nullptr, nullptr, nullptr);

	hBtnLaunch = CreateWindowW(L"BUTTON", L"Launch", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 0, 80, 25, hWnd, (HMENU)1, nullptr, nullptr);
	hBtnOffline = CreateWindowW(L"BUTTON", L"Play Offline", WS_CHILD | WS_VISIBLE, 0, 0, 80, 25, hWnd, (HMENU)2, nullptr, nullptr);

	hBtnRegister = CreateWindowW(L"BUTTON", L"Register", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 0, 80, 25, hWnd, (HMENU)3, nullptr, nullptr);
	hBtnLogin = CreateWindowW(L"BUTTON", L"Login", WS_CHILD | WS_VISIBLE, 0, 0, 80, 25, hWnd, (HMENU)4, nullptr, nullptr);

	hBtnLogout = CreateWindowW(L"BUTTON", L"Logout", WS_CHILD | WS_VISIBLE, 0, 0, 80, 25, hWnd, (HMENU)5, nullptr, nullptr);

	hBtnDiscord = CreateWindowW(L"BUTTON", L"Discord", WS_CHILD | WS_VISIBLE, 0, 0, 80, 25, hWnd, (HMENU)6, nullptr, nullptr);

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
	else if (registerResponse == 2222) {
		MessageBoxW(launcher_HWND, L"Invalid Username Characters", L"Registraction Failed", MB_OK);
	}
	else if (registerResponse == 2233) {
		MessageBoxW(launcher_HWND, L"Invalid Password Characters", L"Registraction Failed", MB_OK);
	}
	else if (registerResponse == 3333) {
		MessageBoxW(launcher_HWND, L"Username Taken", L"Registraction Failed", MB_OK);
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