// Code implemented by LCEMP, credit if used on other repos
// https://github.com/LCEMP/LCEMP

#include "stdafx.h"

#ifdef _WINDOWS64

#include "WinsockNetLayer.h"
#include "..\..\Common\Network\PlatformNetworkManagerStub.h"
#include "..\..\..\Minecraft.World\Socket.h"
#include "..\..\..\Minecraft.World\DisconnectPacket.h"
#include "..\..\Minecraft.h"
#include "..\4JLibs\inc\4J_Profile.h"

#include <string>

#include <thread>
#include <Windows64/Windows64_Minecraft.h>

#include "json.hpp"

static bool RecvExact(SOCKET sock, BYTE* buf, int len);

SOCKET WinsockNetLayer::s_listenSocket = INVALID_SOCKET;
SOCKET WinsockNetLayer::s_hostConnectionSocket = INVALID_SOCKET;
HANDLE WinsockNetLayer::s_acceptThread = nullptr;
HANDLE WinsockNetLayer::s_clientRecvThread = nullptr;

bool WinsockNetLayer::s_isHost = false;
bool WinsockNetLayer::s_connected = false;
bool WinsockNetLayer::s_active = false;
bool WinsockNetLayer::s_initialized = false;

BYTE WinsockNetLayer::s_localSmallId = 0;
BYTE WinsockNetLayer::s_hostSmallId = 0;
unsigned int WinsockNetLayer::s_nextSmallId = XUSER_MAX_COUNT;

CRITICAL_SECTION WinsockNetLayer::s_sendLock;
CRITICAL_SECTION WinsockNetLayer::s_connectionsLock;

std::vector<Win64RemoteConnection> WinsockNetLayer::s_connections;

SOCKET WinsockNetLayer::s_advertiseSock = INVALID_SOCKET;
HANDLE WinsockNetLayer::s_advertiseThread = nullptr;
volatile bool WinsockNetLayer::s_advertising = false;
Win64LANBroadcast WinsockNetLayer::s_advertiseData = {};
CRITICAL_SECTION WinsockNetLayer::s_advertiseLock;
int WinsockNetLayer::s_hostGamePort = WIN64_NET_DEFAULT_PORT;

SOCKET WinsockNetLayer::s_discoverySock = INVALID_SOCKET;
HANDLE WinsockNetLayer::s_discoveryThread = nullptr;
volatile bool WinsockNetLayer::s_discovering = false;
CRITICAL_SECTION WinsockNetLayer::s_discoveryLock;
std::vector<Win64LANSession> WinsockNetLayer::s_discoveredSessions;

CRITICAL_SECTION WinsockNetLayer::s_disconnectLock;
std::vector<BYTE> WinsockNetLayer::s_disconnectedSmallIds;

CRITICAL_SECTION WinsockNetLayer::s_freeSmallIdLock;
std::vector<BYTE> WinsockNetLayer::s_freeSmallIds;
SOCKET WinsockNetLayer::s_smallIdToSocket[256];
CRITICAL_SECTION WinsockNetLayer::s_smallIdToSocketLock;

SOCKET WinsockNetLayer::s_splitScreenSocket[XUSER_MAX_COUNT] = { INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET };
BYTE WinsockNetLayer::s_splitScreenSmallId[XUSER_MAX_COUNT] = { 0xFF, 0xFF, 0xFF, 0xFF };
HANDLE WinsockNetLayer::s_splitScreenRecvThread[XUSER_MAX_COUNT] = {nullptr, nullptr, nullptr, nullptr};

char g_Win64RelayServerIP[256] = "38.49.215.81";
int g_Win64RelayServerPort = 3501;

wchar_t g_Win64AuthIP_Wide[256] = L"38.49.215.81";
int g_Win64AuthServerPort = 3502;

char g_Win64MultiplayerIP[256] = "";
int g_Win64MultiplayerPort = 0;

bool WinsockNetLayer::Initialize()
{
	if (s_initialized) return true;

	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		app.DebugPrintf("WSAStartup failed: %d\n", result);
		return false;
	}

	InitializeCriticalSection(&s_sendLock);
	InitializeCriticalSection(&s_connectionsLock);
	InitializeCriticalSection(&s_advertiseLock);
	InitializeCriticalSection(&s_discoveryLock);
	InitializeCriticalSection(&s_disconnectLock);
	InitializeCriticalSection(&s_freeSmallIdLock);
	InitializeCriticalSection(&s_smallIdToSocketLock);
	for (int i = 0; i < 256; i++)
		s_smallIdToSocket[i] = INVALID_SOCKET;

	s_initialized = true;

	StartDiscovery();

	return true;
}

void WinsockNetLayer::Shutdown()
{
	StopAdvertising();
	StopDiscovery();

	s_active = false;
	s_connected = false;

	if (s_listenSocket != INVALID_SOCKET)
	{
		closesocket(s_listenSocket);
		s_listenSocket = INVALID_SOCKET;
	}

	if (s_hostConnectionSocket != INVALID_SOCKET)
	{
		closesocket(s_hostConnectionSocket);
		s_hostConnectionSocket = INVALID_SOCKET;
	}

	// Stop accept loop first so no new RecvThread can be created while shutting down.
	if (s_acceptThread != nullptr)
	{
		WaitForSingleObject(s_acceptThread, 2000);
		CloseHandle(s_acceptThread);
		s_acceptThread = nullptr;
	}

	std::vector<HANDLE> recvThreads;
	EnterCriticalSection(&s_connectionsLock);
	for (size_t i = 0; i < s_connections.size(); i++)
	{
		s_connections[i].active = false;
		if (s_connections[i].tcpSocket != INVALID_SOCKET)
		{
			closesocket(s_connections[i].tcpSocket);
			s_connections[i].tcpSocket = INVALID_SOCKET;
		}
		if (s_connections[i].recvThread != nullptr)
		{
			recvThreads.push_back(s_connections[i].recvThread);
			s_connections[i].recvThread = nullptr;
		}
	}
	LeaveCriticalSection(&s_connectionsLock);

	// Wait for all host-side receive threads to exit before destroying state.
	for (size_t i = 0; i < recvThreads.size(); i++)
	{
		WaitForSingleObject(recvThreads[i], 2000);
		CloseHandle(recvThreads[i]);
	}

	EnterCriticalSection(&s_connectionsLock);
	s_connections.clear();
	LeaveCriticalSection(&s_connectionsLock);

	if (s_clientRecvThread != nullptr)
	{
		WaitForSingleObject(s_clientRecvThread, 2000);
		CloseHandle(s_clientRecvThread);
		s_clientRecvThread = nullptr;
	}

	for (int i = 0; i < XUSER_MAX_COUNT; i++)
	{
		if (s_splitScreenSocket[i] != INVALID_SOCKET)
		{
			closesocket(s_splitScreenSocket[i]);
			s_splitScreenSocket[i] = INVALID_SOCKET;
		}
		if (s_splitScreenRecvThread[i] != nullptr)
		{
			WaitForSingleObject(s_splitScreenRecvThread[i], 2000);
			CloseHandle(s_splitScreenRecvThread[i]);
			s_splitScreenRecvThread[i] = nullptr;
		}
		s_splitScreenSmallId[i] = 0xFF;
	}

	if (s_initialized)
	{
		EnterCriticalSection(&s_disconnectLock);
		s_disconnectedSmallIds.clear();
		LeaveCriticalSection(&s_disconnectLock);

		EnterCriticalSection(&s_freeSmallIdLock);
		s_freeSmallIds.clear();
		LeaveCriticalSection(&s_freeSmallIdLock);

		DeleteCriticalSection(&s_sendLock);
		DeleteCriticalSection(&s_connectionsLock);
		DeleteCriticalSection(&s_advertiseLock);
		DeleteCriticalSection(&s_discoveryLock);
		DeleteCriticalSection(&s_disconnectLock);
		DeleteCriticalSection(&s_freeSmallIdLock);
		DeleteCriticalSection(&s_smallIdToSocketLock);
		WSACleanup();
		s_initialized = false;
	}
}

bool WinsockNetLayer::HostGame(int port, const char* bindIp)
{
	if (!s_initialized && !Initialize()) return false;

	s_isHost = true;
	s_localSmallId = 0;
	s_hostSmallId = 0;
	s_nextSmallId = 1;
	s_hostGamePort = port;

	EnterCriticalSection(&s_freeSmallIdLock);
	s_freeSmallIds.clear();
	LeaveCriticalSection(&s_freeSmallIdLock);
	EnterCriticalSection(&s_smallIdToSocketLock);
	for (int i = 0; i < 256; i++)
		s_smallIdToSocket[i] = INVALID_SOCKET;
	LeaveCriticalSection(&s_smallIdToSocketLock);

	struct addrinfo hints = {};
	struct addrinfo* result = NULL;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	//hints.ai_flags = (bindIp == NULL || bindIp[0] == 0) ? AI_PASSIVE : 0;

	char portStr[16];
	sprintf_s(portStr, "%d", g_Win64RelayServerPort);

	//const char* resolvedBindIp = (bindIp != NULL && bindIp[0] != 0) ? bindIp : NULL;
	int iResult = getaddrinfo(g_Win64RelayServerIP, portStr, &hints, &result);
	if (iResult != 0)
	{
		app.DebugPrintf("getaddrinfo failed for relay server %s:%d - %d\n",
			g_Win64RelayServerIP != NULL ? g_Win64RelayServerIP : "*",
			g_Win64RelayServerPort,
			iResult);
		return false;
	}

	s_listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (s_listenSocket == INVALID_SOCKET)
	{
		app.DebugPrintf("socket() failed: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		return false;
	}

	int opt = 1;
	setsockopt(s_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

	iResult = connect(s_listenSocket, result->ai_addr, (int)result->ai_addrlen);
	freeaddrinfo(result);
	if (iResult == SOCKET_ERROR)
	{
		app.DebugPrintf("connect() to Relay Server %s:%d failed: %d\n", g_Win64RelayServerIP, g_Win64RelayServerPort, WSAGetLastError());
		closesocket(s_listenSocket);
		s_listenSocket = INVALID_SOCKET;
		return false;
	}

	std::string req = "HOST " + Windows64Minecraft::GetAuthenticationTicket() + " 0\n";
	send(s_listenSocket, req.c_str(), (int)req.length(), 0);

	s_active = true;
	s_connected = true;

	s_acceptThread = CreateThread(NULL, 0, AcceptThreadProc, NULL, 0, NULL);

	app.DebugPrintf("Win64 LAN: Hosting on Relay Server %s:%d\n",
		g_Win64RelayServerIP != NULL ? g_Win64RelayServerIP : "*",
		g_Win64RelayServerPort);
	return true;
}

bool WinsockNetLayer::JoinGame(const char* ip, int port)
{
	if (!s_initialized && !Initialize()) return false;

	s_isHost = false;
	s_hostSmallId = 0;
	s_connected = false;
	s_active = false;

	if (s_hostConnectionSocket != INVALID_SOCKET)
	{
		closesocket(s_hostConnectionSocket);
		s_hostConnectionSocket = INVALID_SOCKET;
	}

	// Wait for old client recv thread to fully exit before starting a new connection.
	// Without this, the old thread can read from the new socket (s_hostConnectionSocket
	// is a global) and steal bytes from the new connection's TCP stream, causing
	// packet stream misalignment on reconnect.
	if (s_clientRecvThread != nullptr)
	{
		WaitForSingleObject(s_clientRecvThread, 5000);
		CloseHandle(s_clientRecvThread);
		s_clientRecvThread = nullptr;
	}

	struct addrinfo hints = {};
	struct addrinfo* result = nullptr;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char portStr[16];
	sprintf_s(portStr, "%d", g_Win64RelayServerPort);

	int iResult = getaddrinfo(g_Win64RelayServerIP, portStr, &hints, &result);
	if (iResult != 0)
	{
		app.DebugPrintf("getaddrinfo failed for Relay Server - %d\n", iResult);
		return false;
	}

	bool connected = false;
	BYTE assignedSmallId = 0;
	const int maxAttempts = 3;
	DWORD timeout = 2000; // 2 seconds
	

	for (int attempt = 0; attempt < maxAttempts; ++attempt)
	{
		s_hostConnectionSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (s_hostConnectionSocket == INVALID_SOCKET)
		{
			app.DebugPrintf("socket() failed: %d\n", WSAGetLastError());
			break;
		}

		int noDelay = 1;
		setsockopt(s_hostConnectionSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

		setsockopt(s_hostConnectionSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		setsockopt(s_hostConnectionSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

		iResult = connect(s_hostConnectionSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
		if (iResult == SOCKET_ERROR)
		{
			app.DebugPrintf("connect() to Relay Server %s:%d failed: %d\n", g_Win64RelayServerIP, g_Win64RelayServerPort, WSAGetLastError());
			closesocket(s_hostConnectionSocket);
			s_hostConnectionSocket = INVALID_SOCKET;
			return false;
		}

		std::string req = "JOIN " + Windows64Minecraft::GetAuthenticationTicket() + " 0 " + std::string(ip) + "\n";
		send(s_hostConnectionSocket, req.c_str(), (int)req.length(), 0);

		BYTE assignBuf[1];
		int bytesRecv = recv(s_hostConnectionSocket, (char*)assignBuf, 1, 0);
		if (bytesRecv != 1)
		{
			app.DebugPrintf("Failed to receive small ID assignment from host (attempt %d/%d)\n", attempt + 1, maxAttempts);
			closesocket(s_hostConnectionSocket);
			s_hostConnectionSocket = INVALID_SOCKET;
			Sleep(200);
			continue;
		}

		if (assignBuf[0] == WIN64_SMALLID_REJECT)
		{
			BYTE rejectBuf[5];
			if (!RecvExact(s_hostConnectionSocket, rejectBuf, 5))
			{
				app.DebugPrintf("Failed to receive reject reason from host\n");
				closesocket(s_hostConnectionSocket);
				s_hostConnectionSocket = INVALID_SOCKET;
				Sleep(200);
				continue;
			}
			// rejectBuf[0] = packet id (255), rejectBuf[1..4] = 4-byte big-endian reason
			int reason = ((rejectBuf[1] & 0xff) << 24) | ((rejectBuf[2] & 0xff) << 16) |
				((rejectBuf[3] & 0xff) << 8) | (rejectBuf[4] & 0xff);
			Minecraft::GetInstance()->connectionDisconnected(ProfileManager.GetPrimaryPad(), (DisconnectPacket::eDisconnectReason)reason);
			closesocket(s_hostConnectionSocket);
			s_hostConnectionSocket = INVALID_SOCKET;
			freeaddrinfo(result);
			return false;
		}

		assignedSmallId = assignBuf[0];
		connected = true;
		break;
	}
	freeaddrinfo(result);

	if (!connected) {
		return false;
	}
	s_localSmallId = assignedSmallId;

	// Save the host IP and port so JoinSplitScreen can connect to the same host
	// regardless of how the connection was initiated (UI vs command line).
	strncpy_s(g_Win64MultiplayerIP, sizeof(g_Win64MultiplayerIP), ip, _TRUNCATE);
	g_Win64MultiplayerPort = port;

	app.DebugPrintf("Win64 LAN: Connected via Relay to Session %s, assigned smallId=%d\n", ip, s_localSmallId);

	s_active = true;
	s_connected = true;

	s_clientRecvThread = CreateThread(nullptr, 0, ClientRecvThreadProc, nullptr, 0, nullptr);

	return true;
}

bool WinsockNetLayer::SendOnSocket(SOCKET sock, const void* data, int dataSize)
{
	if (sock == INVALID_SOCKET || dataSize <= 0 || dataSize > WIN64_NET_MAX_PACKET_SIZE) return false;

	// TODO: s_sendLock is a single global lock for ALL sockets. If one client's
	// send() blocks (TCP window full, slow WiFi), every other write thread stalls
	// waiting for this lock — no data flows to any player until the slow send
	// completes. This scales badly with player count (8+ players = noticeable).
	// Fix: replace with per-socket locks indexed by smallId (s_perSocketSendLock[256]).
	// The lock only needs to prevent interleaving of header+payload on the SAME socket;
	// sends to different sockets are independent and should never block each other.
	EnterCriticalSection(&s_sendLock);

	BYTE header[4];
	header[0] = static_cast<BYTE>((dataSize >> 24) & 0xFF);
	header[1] = static_cast<BYTE>((dataSize >> 16) & 0xFF);
	header[2] = static_cast<BYTE>((dataSize >> 8) & 0xFF);
	header[3] = static_cast<BYTE>(dataSize & 0xFF);

	int totalSent = 0;
	int toSend = 4;
	while (totalSent < toSend)
	{
		int sent = send(sock, (const char*)header + totalSent, toSend - totalSent, 0);
		if (sent == SOCKET_ERROR || sent == 0)
		{
			LeaveCriticalSection(&s_sendLock);
			return false;
		}
		totalSent += sent;
	}

	totalSent = 0;
	while (totalSent < dataSize)
	{
		int sent = send(sock, static_cast<const char *>(data) + totalSent, dataSize - totalSent, 0);
		if (sent == SOCKET_ERROR || sent == 0)
		{
			LeaveCriticalSection(&s_sendLock);
			return false;
		}
		totalSent += sent;
	}

	LeaveCriticalSection(&s_sendLock);
	return true;
}

bool WinsockNetLayer::SendToSmallId(BYTE targetSmallId, const void* data, int dataSize)
{
	if (!s_active) return false;

	if (s_isHost)
	{
		SOCKET sock = GetSocketForSmallId(targetSmallId);
		if (sock == INVALID_SOCKET) return false;
		return SendOnSocket(sock, data, dataSize);
	}
	else
	{
		return SendOnSocket(s_hostConnectionSocket, data, dataSize);
	}
}

SOCKET WinsockNetLayer::GetSocketForSmallId(BYTE smallId)
{
	EnterCriticalSection(&s_smallIdToSocketLock);
	SOCKET sock = s_smallIdToSocket[smallId];
	LeaveCriticalSection(&s_smallIdToSocketLock);
	return sock;
}

void WinsockNetLayer::ClearSocketForSmallId(BYTE smallId)
{
	EnterCriticalSection(&s_smallIdToSocketLock);
	s_smallIdToSocket[smallId] = INVALID_SOCKET;
	LeaveCriticalSection(&s_smallIdToSocketLock);
}

// Send reject handshake: sentinel 0xFF + DisconnectPacket wire format (1 byte id 255 + 4 byte big-endian reason). Then caller closes socket.
static void SendRejectWithReason(SOCKET clientSocket, DisconnectPacket::eDisconnectReason reason)
{
	BYTE buf[6];
	buf[0] = WIN64_SMALLID_REJECT;
	buf[1] = (BYTE)255; // DisconnectPacket packet id
	int r = (int)reason;
	buf[2] = (BYTE)((r >> 24) & 0xff);
	buf[3] = (BYTE)((r >> 16) & 0xff);
	buf[4] = (BYTE)((r >> 8) & 0xff);
	buf[5] = (BYTE)(r & 0xff);
	send(clientSocket, (const char*)buf, sizeof(buf), 0);
}

static bool RecvExact(SOCKET sock, BYTE* buf, int len)
{
	int totalRecv = 0;
	while (totalRecv < len)
	{
		int r = recv(sock, (char*)buf + totalRecv, len - totalRecv, 0);
		if (r <= 0) return false;
		totalRecv += r;
	}
	return true;
}

#if defined(MINECRAFT_SERVER_BUILD)
static bool TryGetNumericRemoteIp(const sockaddr_in &remoteAddress, std::string *outIp)
{
	if (outIp == nullptr)
	{
		return false;
	}

	outIp->clear();
	char ipBuffer[64] = {};
	const char *ip = inet_ntop(AF_INET, (void *)&remoteAddress.sin_addr, ipBuffer, sizeof(ipBuffer));
	if (ip == nullptr || ip[0] == 0)
	{
		return false;
	}

	*outIp = ip;
	return true;
}
#endif

void WinsockNetLayer::HandleDataReceived(BYTE fromSmallId, BYTE toSmallId, unsigned char* data, unsigned int dataSize)
{
	INetworkPlayer* pPlayerFrom = g_NetworkManager.GetPlayerBySmallId(fromSmallId);
	INetworkPlayer* pPlayerTo = g_NetworkManager.GetPlayerBySmallId(toSmallId);

	if (pPlayerFrom == nullptr || pPlayerTo == nullptr)
	{
		app.DebugPrintf("NET RECV: DROPPED %u bytes from=%d to=%d (player NULL: from=%p to=%p)\n",
			dataSize, fromSmallId, toSmallId, pPlayerFrom, pPlayerTo);
		return;
	}

	if (s_isHost)
	{
		::Socket* pSocket = pPlayerFrom->GetSocket();
		if (pSocket != nullptr)
			pSocket->pushDataToQueue(data, dataSize, false);
		else
			app.DebugPrintf("NET RECV: DROPPED %u bytes, host pSocket NULL for from=%d\n", dataSize, fromSmallId);
	}
	else
	{
		::Socket* pSocket = pPlayerTo->GetSocket();
		if (pSocket != nullptr)
			pSocket->pushDataToQueue(data, dataSize, true);
		else
			app.DebugPrintf("NET RECV: DROPPED %u bytes, client pSocket NULL for to=%d\n", dataSize, toSmallId);
	}
}

DWORD WINAPI WinsockNetLayer::AcceptThreadProc(LPVOID param)
{
	char buf[256];
	while (s_active)
	{
		int ret = recv(s_listenSocket, buf, sizeof(buf) - 1, 0);
		if (ret <= 0)
		{
			if (s_active)
				app.DebugPrintf("Relay Server control socket disconnected.\n");
			break;
		}
		buf[ret] = 0;

		std::string str(buf);
		size_t pos = 0;
		while ((pos = str.find('\n')) != std::string::npos)
		{
			std::string line = str.substr(0, pos);
			str.erase(0, pos + 1);

			if (line.find("CLIENT ") == 0)
			{
				std::string clientId = line.substr(7);

				struct addrinfo hints = {};
				struct addrinfo* result = NULL;

				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_TCP;

				char portStr[16];
				sprintf_s(portStr, "%d", g_Win64RelayServerPort);

				int iResult = getaddrinfo(g_Win64RelayServerIP, portStr, &hints, &result);
				if (iResult != 0) continue;

				SOCKET clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
				if (clientSocket == INVALID_SOCKET) { freeaddrinfo(result); continue; }

				int noDelay = 1;
				setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

				iResult = connect(clientSocket, result->ai_addr, (int)result->ai_addrlen);
				freeaddrinfo(result);
				if (iResult == SOCKET_ERROR)
				{
					closesocket(clientSocket);
					continue;
				}

				std::string acc = "ACCEPT " + Windows64Minecraft::GetAuthenticationTicket() + " 0 " + clientId + "\n";

				send(clientSocket, acc.c_str(), (int)acc.length(), 0);

				extern QNET_STATE _iQNetStubState;
				if (_iQNetStubState != QNET_STATE_GAME_PLAY && _iQNetStubState != QNET_STATE_SESSION_HOSTING && _iQNetStubState != QNET_STATE_SESSION_STARTING)
				{
					app.DebugPrintf("Win64 LAN: Rejecting connection, game not ready\n");
					closesocket(clientSocket);
					continue;
				}

				extern CPlatformNetworkManagerStub* g_pPlatformNetworkManager;
				if (g_pPlatformNetworkManager != NULL && !g_pPlatformNetworkManager->CanAcceptMoreConnections())
				{
					app.DebugPrintf("Win64 LAN: Rejecting connection, server at max players\n");
					SendRejectWithReason(clientSocket, DisconnectPacket::eDisconnect_ServerFull);
					closesocket(clientSocket);
					continue;
				}

				BYTE assignedSmallId;
				EnterCriticalSection(&s_freeSmallIdLock);
				if (!s_freeSmallIds.empty())
				{
					assignedSmallId = s_freeSmallIds.back();
					s_freeSmallIds.pop_back();
				}
				else if (s_nextSmallId < (unsigned int)MINECRAFT_NET_MAX_PLAYERS)
				{
					assignedSmallId = (BYTE)s_nextSmallId++;
				}
				else
				{
					LeaveCriticalSection(&s_freeSmallIdLock);
					app.DebugPrintf("Win64 LAN: Server full, rejecting connection\n");
					SendRejectWithReason(clientSocket, DisconnectPacket::eDisconnect_ServerFull);
					closesocket(clientSocket);
					continue;
				}
				LeaveCriticalSection(&s_freeSmallIdLock);

				BYTE assignBuf[1] = { assignedSmallId };
				int sent = send(clientSocket, (const char*)assignBuf, 1, 0);
				if (sent != 1)
				{
					app.DebugPrintf("Failed to send small ID to client\n");
					closesocket(clientSocket);
					PushFreeSmallId(assignedSmallId);
					continue;
				}

				// If an old slot for this smallId somehow remains, retire it before reusing the id.
				EnterCriticalSection(&s_connectionsLock);
				for (size_t i = 0; i < s_connections.size(); i++)
				{
					if (s_connections[i].smallId == assignedSmallId)
					{
						s_connections[i].active = false;
						if (s_connections[i].tcpSocket != INVALID_SOCKET)
						{
							closesocket(s_connections[i].tcpSocket);
							s_connections[i].tcpSocket = INVALID_SOCKET;
						}
					}
				}
				LeaveCriticalSection(&s_connectionsLock);

				Win64RemoteConnection conn;
				conn.tcpSocket = clientSocket;
				conn.smallId = assignedSmallId;
				conn.active = true;
				conn.recvThread = NULL;

				EnterCriticalSection(&s_connectionsLock);
				s_connections.push_back(conn);
				int connIdx = (int)s_connections.size() - 1;
				LeaveCriticalSection(&s_connectionsLock);

				app.DebugPrintf("Win64 LAN: Client connected via Relay, assigned smallId=%d\n", assignedSmallId);

				EnterCriticalSection(&s_smallIdToSocketLock);
				s_smallIdToSocket[assignedSmallId] = clientSocket;
				LeaveCriticalSection(&s_smallIdToSocketLock);

				IQNetPlayer* qnetPlayer = &IQNet::m_player[assignedSmallId];

				extern void Win64_SetupRemoteQNetPlayer(IQNetPlayer * player, BYTE smallId, bool isHost, bool isLocal);
				Win64_SetupRemoteQNetPlayer(qnetPlayer, assignedSmallId, false, false);

				extern CPlatformNetworkManagerStub* g_pPlatformNetworkManager;
				g_pPlatformNetworkManager->NotifyPlayerJoined(qnetPlayer);

				DWORD* threadParam = new DWORD;
				*threadParam = connIdx;
				HANDLE hThread = CreateThread(NULL, 0, RecvThreadProc, threadParam, 0, NULL);

				EnterCriticalSection(&s_connectionsLock);
				if (connIdx < (int)s_connections.size())
					s_connections[connIdx].recvThread = hThread;
				LeaveCriticalSection(&s_connectionsLock);
			}
		}
	}
	return 0;
}

DWORD WINAPI WinsockNetLayer::RecvThreadProc(LPVOID param)
{
	DWORD connIdx = *static_cast<DWORD *>(param);
	delete static_cast<DWORD *>(param);

	EnterCriticalSection(&s_connectionsLock);
	if (connIdx >= static_cast<DWORD>(s_connections.size()))
	{
		LeaveCriticalSection(&s_connectionsLock);
		return 0;
	}
	SOCKET sock = s_connections[connIdx].tcpSocket;
	BYTE clientSmallId = s_connections[connIdx].smallId;
	LeaveCriticalSection(&s_connectionsLock);

	std::vector<BYTE> recvBuf;
	recvBuf.resize(WIN64_NET_RECV_BUFFER_SIZE);

	while (s_active)
	{
		BYTE header[4];
		if (!RecvExact(sock, header, 4))
		{
			app.DebugPrintf("Win64 LAN: Client smallId=%d disconnected (header)\n", clientSmallId);
			break;
		}

		int packetSize =
			(static_cast<uint32_t>(header[0]) << 24) |
			(static_cast<uint32_t>(header[1]) << 16) |
			(static_cast<uint32_t>(header[2]) << 8) |
			static_cast<uint32_t>(header[3]);

		if (packetSize <= 0 || packetSize > WIN64_NET_MAX_PACKET_SIZE)
		{
			app.DebugPrintf("Win64 LAN: Invalid packet size %d from client smallId=%d (max=%d)\n",
				packetSize,
				clientSmallId,
				(int)WIN64_NET_MAX_PACKET_SIZE);
			break;
		}

		if (static_cast<int>(recvBuf.size()) < packetSize)
		{
			recvBuf.resize(packetSize);
			app.DebugPrintf("Win64 LAN: Resized host recv buffer to %d bytes for client smallId=%d\n", packetSize, clientSmallId);
		}

		if (!RecvExact(sock, &recvBuf[0], packetSize))
		{
			app.DebugPrintf("Win64 LAN: Client smallId=%d disconnected (body)\n", clientSmallId);
			break;
		}

		HandleDataReceived(clientSmallId, s_hostSmallId, &recvBuf[0], packetSize);
	}

	EnterCriticalSection(&s_connectionsLock);
	for (size_t i = 0; i < s_connections.size(); i++)
	{
		if (s_connections[i].smallId == clientSmallId)
		{
			s_connections[i].active = false;
			if (s_connections[i].tcpSocket != INVALID_SOCKET)
			{
				closesocket(s_connections[i].tcpSocket);
				s_connections[i].tcpSocket = INVALID_SOCKET;
			}
			break;
		}
	}
	LeaveCriticalSection(&s_connectionsLock);

	EnterCriticalSection(&s_disconnectLock);
	s_disconnectedSmallIds.push_back(clientSmallId);
	LeaveCriticalSection(&s_disconnectLock);

	return 0;
}

bool WinsockNetLayer::PopDisconnectedSmallId(BYTE* outSmallId)
{
	bool found = false;
	EnterCriticalSection(&s_disconnectLock);
	if (!s_disconnectedSmallIds.empty())
	{
		*outSmallId = s_disconnectedSmallIds.back();
		s_disconnectedSmallIds.pop_back();
		found = true;
	}
	LeaveCriticalSection(&s_disconnectLock);
	return found;
}

void WinsockNetLayer::PushFreeSmallId(BYTE smallId)
{
	// SmallIds 0..(XUSER_MAX_COUNT-1) are permanently reserved for the host's
	// local pads and must never be recycled to remote clients.
	if (smallId < (BYTE)XUSER_MAX_COUNT)
		return;

	EnterCriticalSection(&s_freeSmallIdLock);
	// Guard against double-recycle: the reconnect path (queueSmallIdForRecycle) and
	// the DoWork disconnect path can both push the same smallId. If we allow duplicates,
	// AcceptThread will hand out the same smallId to two different connections.
	bool alreadyFree = false;
	for (size_t i = 0; i < s_freeSmallIds.size(); i++)
	{
		if (s_freeSmallIds[i] == smallId) { alreadyFree = true; break; }
	}
	if (!alreadyFree)
		s_freeSmallIds.push_back(smallId);
	LeaveCriticalSection(&s_freeSmallIdLock);
}

void WinsockNetLayer::CloseConnectionBySmallId(BYTE smallId)
{
	EnterCriticalSection(&s_connectionsLock);
	for (size_t i = 0; i < s_connections.size(); i++)
	{
		if (s_connections[i].smallId == smallId && s_connections[i].active && s_connections[i].tcpSocket != INVALID_SOCKET)
		{
			closesocket(s_connections[i].tcpSocket);
			s_connections[i].tcpSocket = INVALID_SOCKET;
			app.DebugPrintf("Win64 LAN: Force-closed TCP connection for smallId=%d\n", smallId);
			break;
		}
	}
	LeaveCriticalSection(&s_connectionsLock);
}

BYTE WinsockNetLayer::GetSplitScreenSmallId(int padIndex)
{
	if (padIndex <= 0 || padIndex >= XUSER_MAX_COUNT) return 0xFF;
	return s_splitScreenSmallId[padIndex];
}

SOCKET WinsockNetLayer::GetLocalSocket(BYTE senderSmallId)
{
	if (senderSmallId == s_localSmallId)
		return s_hostConnectionSocket;
	for (int i = 1; i < XUSER_MAX_COUNT; i++)
	{
		if (s_splitScreenSmallId[i] == senderSmallId && s_splitScreenSocket[i] != INVALID_SOCKET)
			return s_splitScreenSocket[i];
	}
	return INVALID_SOCKET;
}

bool WinsockNetLayer::JoinSplitScreen(int padIndex, BYTE* outSmallId)//todo: update this to the new networking apis
{
	if (!s_active || s_isHost || padIndex <= 0 || padIndex >= XUSER_MAX_COUNT)
		return false;

	return false;
/*
	if (s_splitScreenSocket[padIndex] != INVALID_SOCKET)
	{
		return false;
	}

	struct addrinfo hints = {};
	struct addrinfo* result = nullptr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char portStr[16];
	sprintf_s(portStr, "%d", g_Win64MultiplayerPort);
	if (getaddrinfo(g_Win64MultiplayerIP, portStr, &hints, &result) != 0 || result == nullptr)
	{
		app.DebugPrintf("Win64 LAN: Split-screen getaddrinfo failed for %s:%d\n", g_Win64MultiplayerIP, g_Win64MultiplayerPort);
		return false;
	}

	SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock == INVALID_SOCKET)
	{
		freeaddrinfo(result);
		return false;
	}

	int noDelay = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

	if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
	{
		app.DebugPrintf("Win64 LAN: Split-screen connect() failed: %d\n", WSAGetLastError());
		closesocket(sock);
		freeaddrinfo(result);
		return false;
	}
	freeaddrinfo(result);

	BYTE assignBuf[1];
	if (!RecvExact(sock, assignBuf, 1))
	{
		app.DebugPrintf("Win64 LAN: Split-screen failed to receive smallId\n");
		closesocket(sock);
		return false;
	}

	if (assignBuf[0] == WIN64_SMALLID_REJECT)
	{
		BYTE rejectBuf[5];
		RecvExact(sock, rejectBuf, 5);
		app.DebugPrintf("Win64 LAN: Split-screen connection rejected\n");
		closesocket(sock);
		return false;
	}

	BYTE assignedSmallId = assignBuf[0];
	s_splitScreenSocket[padIndex] = sock;
	s_splitScreenSmallId[padIndex] = assignedSmallId;
	*outSmallId = assignedSmallId;

	app.DebugPrintf("Win64 LAN: Split-screen pad %d connected, assigned smallId=%d\n", padIndex, assignedSmallId);

	int* threadParam = new int;
	*threadParam = padIndex;
	s_splitScreenRecvThread[padIndex] = CreateThread(nullptr, 0, SplitScreenRecvThreadProc, threadParam, 0, nullptr);
	if (s_splitScreenRecvThread[padIndex] == nullptr)
	{
		delete threadParam;
		closesocket(sock);
		s_splitScreenSocket[padIndex] = INVALID_SOCKET;
		s_splitScreenSmallId[padIndex] = 0xFF;
		app.DebugPrintf("Win64 LAN: CreateThread failed for split-screen pad %d\n", padIndex);
		return false;
	}

	return true;*/
}

void WinsockNetLayer::CloseSplitScreenConnection(int padIndex)
{
	if (padIndex <= 0 || padIndex >= XUSER_MAX_COUNT) return;

	if (s_splitScreenSocket[padIndex] != INVALID_SOCKET)
	{
		closesocket(s_splitScreenSocket[padIndex]);
		s_splitScreenSocket[padIndex] = INVALID_SOCKET;
	}
	s_splitScreenSmallId[padIndex] = 0xFF;
	if (s_splitScreenRecvThread[padIndex] != nullptr)
	{
		WaitForSingleObject(s_splitScreenRecvThread[padIndex], 2000);
		CloseHandle(s_splitScreenRecvThread[padIndex]);
		s_splitScreenRecvThread[padIndex] = nullptr;
	}
}

DWORD WINAPI WinsockNetLayer::SplitScreenRecvThreadProc(LPVOID param)
{
	int padIndex = *(int*)param;
	delete (int*)param;

	SOCKET sock = s_splitScreenSocket[padIndex];
	BYTE localSmallId = s_splitScreenSmallId[padIndex];
	std::vector<BYTE> recvBuf;
	recvBuf.resize(WIN64_NET_RECV_BUFFER_SIZE);

	while (s_active && s_splitScreenSocket[padIndex] != INVALID_SOCKET)
	{
		BYTE header[4];
		if (!RecvExact(sock, header, 4))
		{
			app.DebugPrintf("Win64 LAN: Split-screen pad %d disconnected from host\n", padIndex);
			break;
		}

		int packetSize = ((uint32_t)header[0] << 24) | ((uint32_t)header[1] << 16) |
			((uint32_t)header[2] << 8) | ((uint32_t)header[3]);
		if (packetSize <= 0 || packetSize > WIN64_NET_MAX_PACKET_SIZE)
		{
			app.DebugPrintf("Win64 LAN: Split-screen pad %d invalid packet size %d\n", padIndex, packetSize);
			break;
		}

		if ((int)recvBuf.size() < packetSize)
			recvBuf.resize(packetSize);

		if (!RecvExact(sock, &recvBuf[0], packetSize))
		{
			app.DebugPrintf("Win64 LAN: Split-screen pad %d disconnected from host (body)\n", padIndex);
			break;
		}

		HandleDataReceived(s_hostSmallId, localSmallId, &recvBuf[0], packetSize);
	}

	EnterCriticalSection(&s_disconnectLock);
	s_disconnectedSmallIds.push_back(localSmallId);
	LeaveCriticalSection(&s_disconnectLock);

	return 0;
}

DWORD WINAPI WinsockNetLayer::ClientRecvThreadProc(LPVOID param)
{
	std::vector<BYTE> recvBuf;
	recvBuf.resize(WIN64_NET_RECV_BUFFER_SIZE);

	while (s_active && s_hostConnectionSocket != INVALID_SOCKET)
	{
		BYTE header[4];
		if (!RecvExact(s_hostConnectionSocket, header, 4))
		{
			app.DebugPrintf("Win64 LAN: Disconnected from host (header)\n");
			break;
		}

		int packetSize = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];

		if (packetSize <= 0 || packetSize > WIN64_NET_MAX_PACKET_SIZE)
		{
			app.DebugPrintf("Win64 LAN: Invalid packet size %d from host (max=%d)\n",
				packetSize,
				(int)WIN64_NET_MAX_PACKET_SIZE);
			break;
		}

		if (static_cast<int>(recvBuf.size()) < packetSize)
		{
			recvBuf.resize(packetSize);
			app.DebugPrintf("Win64 LAN: Resized client recv buffer to %d bytes\n", packetSize);
		}

		if (!RecvExact(s_hostConnectionSocket, &recvBuf[0], packetSize))
		{
			app.DebugPrintf("Win64 LAN: Disconnected from host (body)\n");
			break;
		}

		HandleDataReceived(s_hostSmallId, s_localSmallId, &recvBuf[0], packetSize);
	}

	s_connected = false;
	return 0;
}

bool WinsockNetLayer::StartAdvertising(int gamePort, const wchar_t* hostName, unsigned int gameSettings, unsigned int texPackId, unsigned char subTexId, unsigned short netVer)
{
	if (s_advertising) return true;
	if (!s_initialized) return false;

	EnterCriticalSection(&s_advertiseLock);
	memset(&s_advertiseData, 0, sizeof(s_advertiseData));
	s_advertiseData.magic = WIN64_LAN_BROADCAST_MAGIC;
	s_advertiseData.netVersion = netVer;
	s_advertiseData.gamePort = static_cast<WORD>(gamePort);
	wcsncpy_s(s_advertiseData.hostName, 32, hostName, _TRUNCATE);
	s_advertiseData.playerCount = 1;
	s_advertiseData.maxPlayers = MINECRAFT_NET_MAX_PLAYERS;
	s_advertiseData.gameHostSettings = gameSettings;
	s_advertiseData.texturePackParentId = texPackId;
	s_advertiseData.subTexturePackId = subTexId;
	s_advertiseData.isJoinable = 0;
	s_hostGamePort = gamePort;
	LeaveCriticalSection(&s_advertiseLock);

	s_advertiseSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s_advertiseSock == INVALID_SOCKET)
	{
		app.DebugPrintf("Win64 LAN: Failed to create advertise socket: %d\n", WSAGetLastError());
		return false;
	}

	BOOL broadcast = TRUE;
	setsockopt(s_advertiseSock, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));

	s_advertising = true;
	s_advertiseThread = CreateThread(nullptr, 0, AdvertiseThreadProc, nullptr, 0, nullptr);

	app.DebugPrintf("Win64 LAN: Started advertising on UDP port %d\n", WIN64_LAN_DISCOVERY_PORT);
	return true;
}

void WinsockNetLayer::StopAdvertising()
{
	s_advertising = false;

	if (s_advertiseSock != INVALID_SOCKET)
	{
		closesocket(s_advertiseSock);
		s_advertiseSock = INVALID_SOCKET;
	}

	if (s_advertiseThread != nullptr)
	{
		WaitForSingleObject(s_advertiseThread, 2000);
		CloseHandle(s_advertiseThread);
		s_advertiseThread = nullptr;
	}
}

void WinsockNetLayer::UpdateAdvertisePlayerCount(BYTE count)
{
	EnterCriticalSection(&s_advertiseLock);
	s_advertiseData.playerCount = count;
	LeaveCriticalSection(&s_advertiseLock);
}

void WinsockNetLayer::UpdateAdvertiseMaxPlayers(BYTE maxPlayers)
{
	EnterCriticalSection(&s_advertiseLock);
	s_advertiseData.maxPlayers = maxPlayers;
	LeaveCriticalSection(&s_advertiseLock);
}

void WinsockNetLayer::UpdateAdvertiseJoinable(bool joinable)
{
	EnterCriticalSection(&s_advertiseLock);
	s_advertiseData.isJoinable = joinable ? 1 : 0;
	LeaveCriticalSection(&s_advertiseLock);
}

DWORD WINAPI WinsockNetLayer::AdvertiseThreadProc(LPVOID param)
{
	struct addrinfo hints = {};
	struct addrinfo* result = NULL;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	//hints.ai_flags = (bindIp == NULL || bindIp[0] == 0) ? AI_PASSIVE : 0;

	char portStr[16];
	sprintf_s(portStr, "%d", g_Win64RelayServerPort);

	//const char* resolvedBindIp = (bindIp != NULL && bindIp[0] != 0) ? bindIp : NULL;
	int iResult = getaddrinfo(g_Win64RelayServerIP, portStr, &hints, &result);
	if (iResult != 0)
	{
		app.DebugPrintf("getaddrinfo failed for relay server %s:%d - %d\n",
			g_Win64RelayServerIP != NULL ? g_Win64RelayServerIP : "*",
			g_Win64RelayServerPort,
			iResult);
		return false;
	}

	while (s_advertising)
	{
		EnterCriticalSection(&s_advertiseLock);
		Win64LANBroadcast data = s_advertiseData;
		LeaveCriticalSection(&s_advertiseLock);


		SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			app.DebugPrintf("socket() failed: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			return false;
		}

		int opt = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

		iResult = connect(sock, result->ai_addr, (int)result->ai_addrlen);
		//freeaddrinfo(result);
		if (iResult == SOCKET_ERROR)
		{
			app.DebugPrintf("connect() to Relay Server %s:%d failed: %d\n", g_Win64RelayServerIP, g_Win64RelayServerPort, WSAGetLastError());
			closesocket(sock);
			sock = INVALID_SOCKET;
			return false;
		}

		std::string req = "ADVERTISE " + Windows64Minecraft::GetAuthenticationTicket() + " 0";
		req += " " + std::to_string(data.playerCount);
		req += " " + std::to_string(data.maxPlayers);
		req += " " + std::to_string(data.gameHostSettings);
		req += " " + std::to_string(data.texturePackParentId);
		req += " " + std::to_string(data.subTexturePackId);
		req += " " + std::to_string(data.isJoinable);
		req += "\n";

		send(sock, req.c_str(), (int)req.length(), 0);

		Sleep(1500);
	}

	return 0;
}

bool WinsockNetLayer::StartDiscovery()
{
	if (s_discovering) return true;
	if (!s_initialized) return false;

	s_discovering = true;
	s_discoveryThread = CreateThread(nullptr, 0, DiscoveryThreadProc, nullptr, 0, nullptr);

	app.DebugPrintf("Win64 LAN: Listening for LAN games on UDP port %d\n", WIN64_LAN_DISCOVERY_PORT);
	return true;
}

void WinsockNetLayer::StopDiscovery()
{
	s_discovering = false;

	if (s_discoverySock != INVALID_SOCKET)
	{
		closesocket(s_discoverySock);
		s_discoverySock = INVALID_SOCKET;
	}

	if (s_discoveryThread != nullptr)
	{
		WaitForSingleObject(s_discoveryThread, 2000);
		CloseHandle(s_discoveryThread);
		s_discoveryThread = nullptr;
	}

	EnterCriticalSection(&s_discoveryLock);
	s_discoveredSessions.clear();
	LeaveCriticalSection(&s_discoveryLock);
}

std::vector<Win64LANSession> WinsockNetLayer::GetDiscoveredSessions()
{
	std::vector<Win64LANSession> result;
	EnterCriticalSection(&s_discoveryLock);
	result = s_discoveredSessions;
	LeaveCriticalSection(&s_discoveryLock);
	return result;
}

DWORD WINAPI WinsockNetLayer::DiscoveryThreadProc(LPVOID param)
{
	struct addrinfo hints = {};
	struct addrinfo* result = NULL;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	//hints.ai_flags = (bindIp == NULL || bindIp[0] == 0) ? AI_PASSIVE : 0;

	char portStr[16];
	sprintf_s(portStr, "%d", g_Win64RelayServerPort);

	//const char* resolvedBindIp = (bindIp != NULL && bindIp[0] != 0) ? bindIp : NULL;
	int iResult = getaddrinfo(g_Win64RelayServerIP, portStr, &hints, &result);
	if (iResult != 0)
	{
		app.DebugPrintf("getaddrinfo failed for relay server %s:%d - %d\n",
			g_Win64RelayServerIP != NULL ? g_Win64RelayServerIP : "*",
			g_Win64RelayServerPort,
			iResult);
		return false;
	}

	while (s_discovering)
	{
		SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			app.DebugPrintf("socket() failed: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			return false;
		}

		int opt = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

		iResult = connect(sock, result->ai_addr, (int)result->ai_addrlen);
		//freeaddrinfo(result);
		if (iResult == SOCKET_ERROR)
		{
			app.DebugPrintf("connect() to Relay Server %s:%d failed: %d\n", g_Win64RelayServerIP, g_Win64RelayServerPort, WSAGetLastError());
			closesocket(sock);
			sock = INVALID_SOCKET;
			return false;
		}

		std::string req = "LIST " + Windows64Minecraft::GetAuthenticationTicket() + " 0";
		req += " friends";
		req += "\n";

		send(sock, req.c_str(), (int)req.length(), 0);

		char recvBuf[8192];
		std::string jsonStr;
		while (true)
		{
			int ret = recv(sock, recvBuf, sizeof(recvBuf) - 1, 0);
			if (ret <= 0) break;
			recvBuf[ret] = 0;
			jsonStr += recvBuf;
			if (jsonStr.find('\n') != std::string::npos) break;
		}
		closesocket(sock);

		// Trim trailing newline
		while (!jsonStr.empty() && (jsonStr.back() == '\n' || jsonStr.back() == '\r'))
			jsonStr.pop_back();

		if (!jsonStr.empty())
		{
			try
			{
				auto json_array = nlohmann::json::parse(jsonStr);
				DWORD now = GetTickCount();

				EnterCriticalSection(&s_discoveryLock);
				s_discoveredSessions.clear();

				for (auto& el : json_array)
				{
					std::string sessionId = el["sessionId"];
					std::string hostName = el["sessionId"];
					int currPlayers = el["playerCount"];
					int maxPlayers = el["maxPlayers"];
					unsigned int gameHostSettings = el["gameHostSettings"];
					unsigned int texturePackParentId = el["texturePackParentId"];
					unsigned int subTexturePackId = el["subTexturePackId"];
					bool isJoinable = el["isJoinable"];

					Win64LANSession session = {};
					strncpy_s(session.hostIP, sizeof(session.hostIP), sessionId.c_str(), _TRUNCATE);
					session.hostPort = g_Win64RelayServerPort;

					MultiByteToWideChar(CP_UTF8, 0, hostName.c_str(), -1, session.hostName, 32);

					session.playerCount = currPlayers;
					session.maxPlayers = maxPlayers;
					session.isJoinable = isJoinable;
					session.lastSeenTick = now;
					session.netVersion = MINECRAFT_NET_VERSION;
					session.gameHostSettings = gameHostSettings;
					session.texturePackParentId = texturePackParentId;
					session.subTexturePackId = subTexturePackId;

					s_discoveredSessions.push_back(session);

					app.DebugPrintf("Win64 ONLINE: Discovered game \"%s\" (session: %s, %d/%d players)\n",
						hostName.c_str(), sessionId.c_str(), currPlayers, maxPlayers);
				}

				LeaveCriticalSection(&s_discoveryLock);
			}
			catch (const std::exception& e)
			{
				app.DebugPrintf("Win64 ONLINE: Failed to parse discovery JSON: %s\n", e.what());
			}
		}

		Sleep(8000);
	}

	return 0;
}

HttpResponse WinsockNetLayer::DoWinHttpRequest(const std::wstring& path, const wchar_t* method, const std::string& requestData, const std::vector<std::wstring>& headers)
{
	HttpResponse response;
	response.status = 0;

	HINTERNET hSession = WinHttpOpen(L"Minecraft Client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) return response;

	HINTERNET hConnect = WinHttpConnect(hSession, g_Win64AuthIP_Wide, g_Win64AuthServerPort, 0);
	if (!hConnect) { WinHttpCloseHandle(hSession); return response; }

	int flags = 0; //WINHTTP_FLAG_SECURE


	HINTERNET hRequest = WinHttpOpenRequest(hConnect, method, path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return response; }

	for (const auto& header : headers) {
		WinHttpAddRequestHeaders(hRequest, header.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
	}

	BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)requestData.c_str(), (DWORD)requestData.size(), (DWORD)requestData.size(), 0);
	if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

	if (bResults) {
		DWORD dwStatusCode = 0;
		DWORD dwSize = sizeof(dwStatusCode);
		WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
		response.status = dwStatusCode;

		DWORD dwDownloaded = 0;
		do {
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
			if (dwSize == 0) break;
			char* pszOutBuffer = new char[dwSize + 1];
			if (!pszOutBuffer) break;
			ZeroMemory(pszOutBuffer, dwSize + 1);
			if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
				response.body.append(pszOutBuffer, dwDownloaded);
			}
			delete[] pszOutBuffer;
		} while (dwSize > 0);
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return response;
}

#endif
