#pragma once

class Windows64Minecraft {
public:
	static bool IsOfflineMode();
	static std::string GetAuthenticationTicket();

	static PlayerUID GetMainXUID();
};