#pragma once

#ifdef _WINDOWS64

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <Windows.h>

namespace Win64Xuid
{
	inline bool IsPersistedUidValid(PlayerUID xuid)
	{
		return xuid != INVALID_XUID;
	}

	inline uint64_t Mix64(uint64_t x)
	{
		x += 0x9E3779B97F4A7C15ULL;
		x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
		x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
		return x ^ (x >> 31);
	}

	inline PlayerUID DeriveXuidForPad(PlayerUID baseXuid, int iPad)
	{
		if (iPad == 0)
			return baseXuid;

		// Deterministic per-pad XUID: hash the base XUID with the pad number.
		// Produces a fully unique 64-bit value with no risk of overlap.
		// Suggested by rtm516 to avoid adjacent-integer collisions from the old "+ iPad" approach.
		uint64_t raw = Mix64((uint64_t)baseXuid + (uint64_t)iPad);
		raw |= 0x8000000000000000ULL; // keep high bit set like all our XUIDs

		PlayerUID xuid = (PlayerUID)raw;
		if (!IsPersistedUidValid(xuid))
		{
			raw ^= 0x0100000000000001ULL;
			xuid = (PlayerUID)raw;
		}
		if (!IsPersistedUidValid(xuid))
			xuid = (PlayerUID)(0xD15EA5E000000001ULL + iPad);

		return xuid;
	}

	inline PlayerUID ResolvePersistentXuidFromName(const std::wstring& playerName)
	{
		const unsigned __int64 fnvOffset = 14695981039346656037ULL;
		const unsigned __int64 fnvPrime = 1099511628211ULL;
		unsigned __int64 hash = fnvOffset;

		for (size_t i = 0; i < playerName.length(); ++i)
		{
			unsigned short codeUnit = (unsigned short)playerName[i];
			hash ^= (unsigned __int64)(codeUnit & 0xFF);
			hash *= fnvPrime;
			hash ^= (unsigned __int64)((codeUnit >> 8) & 0xFF);
			hash *= fnvPrime;
		}

		// Namespace the hash away from legacy smallId-based values.
		hash ^= 0x9E3779B97F4A7C15ULL;
		hash |= 0x8000000000000000ULL;

		if (hash == (unsigned __int64)INVALID_XUID)
		{
			hash ^= 0x0100000000000001ULL;
		}

		return (PlayerUID)hash;
	}
}

#endif
