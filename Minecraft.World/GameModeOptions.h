#pragma once

enum eCustomGameModeHostOption
{
	GLIDE_POWERUPS = 6,
	GLIDE_SOLO = 14
};

class GameModeOptions
{
public:
	void Set(eCustomGameModeHostOption option, unsigned int value);
};
