#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include "SocketLog.h"
#include <locale.h>
#include <WS2tcpip.h>
#include <vector>
#include <stdio.h>
#include <time.h>
#define PORT 3000
#define SERVER_ADDR L"127.0.0.1"
#define BUFFER_SIZE 16

#define DUMMY_CNT 200
#define MAX_WIDTH 80
#define MAX_HEIGHT 23
int MovePattern1[10] = { 1,1,0,1,1,2,2,2,2,-2 };


struct Session
{
	SOCKET socket;
	int32_t ID;
	int32_t x;
	int32_t y;
	bool bConnected;
	bool bGetID;
};

std::vector <Session> g_VectorSession;


int32_t g_PrevX = 0;
int32_t g_PrevY = 0;
int32_t g_MyID;

char g_Buffer[BUFFER_SIZE];


Session g_DummySession[DUMMY_CNT] = { 0, };
void Disconnect(Session* session);
void Update();
void Network();
void PacketProcess(Session* session);
void SendToPosition(Session *session);

int main()
{
	srand((unsigned int)time(NULL));
	WSAData wsaData;
	setlocale(LC_ALL, "Korean");

	SOCKADDR_IN serverAddr;
	WCHAR addrBuffer[128];
	
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		ERROR_LOG(L"WsaStartUp() Error");
	}

	wprintf(L"접속할 주소를 입력하시오:");
	wscanf_s(L"%s", addrBuffer, (unsigned)_countof(addrBuffer));

	for (size_t i = 0; i < DUMMY_CNT; i++)
	{
		g_DummySession[i].socket = socket(AF_INET, SOCK_STREAM, 0);
		if (g_DummySession[i].socket == SOCKET_ERROR)
		{
			ERROR_LOG(L"socket() Error");
		}
	}	

	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	//InetPton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr.S_un.S_addr);
	InetPton(AF_INET, addrBuffer, &serverAddr.sin_addr.S_un.S_addr);

	for (size_t i = 0; i < DUMMY_CNT; i++)
	{
		if (0 != connect(g_DummySession[i].socket, (sockaddr*)&serverAddr, sizeof(serverAddr)))
		{
			ERROR_LOG(L"connect() error");
		}
		else
		{ 
			wprintf(L"connect 성공\n");
			g_DummySession[i].bConnected = true;
		}
	}

	while (true)
	{
		Network();
		Update();
		Sleep(100);
	}
	
	for (size_t i = 0; i < DUMMY_CNT; i++)
	{
		closesocket(g_DummySession[i].socket);
	}
	
	WSACleanup();

}

void Disconnect(Session* session)
{
	session->bConnected = false;
	session->bGetID = false;
	session->ID = 0;
	closesocket(session->socket);
	session->x = 0;
	session->y = 0;
}

void Update()
{
	for (size_t i = 0; i < DUMMY_CNT; i++)
	{
		if (g_DummySession[i].bConnected == false || g_DummySession[i].bGetID== false)
		{
			continue;
		}
		int num = rand() % 4; 
		switch (num)
		{
		case 0:
			g_DummySession[i].x--;
			break;
		case 1:
			g_DummySession[i].y--;
			break;

		case 2:
			g_DummySession[i].x++;
			break;
		case 3:
			g_DummySession[i].y++;
			break;
		}
		SendToPosition(&g_DummySession[i]);
	}
	
}



void Network()
{
	fd_set readSet;
	fd_set exceptSet;

	FD_ZERO(&readSet);
	FD_ZERO(&exceptSet);

	for (size_t i = 0; i < DUMMY_CNT; i++)
	{
		if (g_DummySession[i].bConnected)
		{
			FD_SET(g_DummySession[i].socket, &readSet);
			FD_SET(g_DummySession[i].socket, &exceptSet);
		}
	}

	timeval time;
	time.tv_sec = 0;
	time.tv_sec = 0;

	
	while (true)
	{
		int rtn = select(0, &readSet, NULL, &exceptSet, &time);
		if (rtn <=0)
		{
			return;
		}
		for (size_t i = 0; i < DUMMY_CNT; i++)
		{
			if (g_DummySession[i].bConnected == false)
			{
				continue;
			}
			if (FD_ISSET(g_DummySession[i].socket, &exceptSet))
			{
				ERROR_LOG(L"Connect Fail");
			}
			if (FD_ISSET(g_DummySession[i].socket, &readSet))
			{
				int recvCnt = recv(g_DummySession[i].socket, g_Buffer, BUFFER_SIZE, 0);

				if (recvCnt <= 0)
				{
					ERROR_LOG(L"recv Error");
					Disconnect(&g_DummySession[i]);
				}
				else
				{
					PacketProcess(&g_DummySession[i]);
				}
			}
		}
		
	

	}
}

void PacketProcess(Session* session)
{
	int* header = (int*)g_Buffer;

	switch (*header)
	{
		case 0:
		{
			session->ID = *((int*)(g_Buffer + 4));
			break;
		}
		case 1:
		{
			int* id = (int*)(g_Buffer + 4);
			int* x = (int*)(g_Buffer + 8);
			int* y = (int*)(g_Buffer + 12);

			//----------------------------------------
			// 생성메시지가왔는데, 현재 받은 세션의 ID 와 메시지의 ID가 다르면 무시한다.
			//----------------------------------------
			if (session->ID != *id)
			{
				break;
			}
			session->bGetID = true;
			session->x = *x;
			session->y = *y;
			break;

		}
		case 2:
		{
			//int* id = (int*)(g_Buffer + 4);
			//
			////----------------------------------------
			//// 생성메시지가왔는데, 현재 받은 세션의 ID 와 메시지의 ID가 다르면 무시한다.
			////----------------------------------------
			//if (session->ID != *id)
			//{
			//	break;
			//}
			//
		
			break;
		}
		case 3:
		{
			int* id = (int*)(g_Buffer + 4);
			int* x = (int*)(g_Buffer + 8);
			int* y = (int*)(g_Buffer + 12);

			//----------------------------------------
			// 생성메시지가왔는데, 현재 받은 세션의 ID 와 메시지의 ID가 다르면 무시한다.
			//----------------------------------------
			if (session->ID != *id)
			{
				break;
			}
			session->x = *x;
			session->y = *y;
			break;

		}
	}
}

void SendToPosition(Session* session)
{
	int packet[4];
	packet[0] = 3;
	packet[1] = session->ID;
	packet[2] = session->x;
	packet[3] = session->y;
	int sendRtn = send(session->socket, (char*)packet, sizeof(packet), 0);
	if (sendRtn < 0)
	{
		ERROR_LOG(L"send Error");
	}
}

