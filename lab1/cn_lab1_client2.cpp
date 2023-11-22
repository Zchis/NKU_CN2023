#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<winsock2.h>
#include<stdio.h>
#include<time.h>
#pragma comment(lib,"ws2_32.lib")
const char ip[15] = "127.0.0.1";
const int port = 6000;
int id;
void getTime(char* buff)
{
	time_t mytime;
	time(&mytime);
	mytime = time(NULL);
	time_t PTime = 0;
	time_t time = mytime;
	struct tm* timeP;
	char buffer[128];
	PTime = time;
	timeP = localtime(&PTime);
	sprintf(buff, "%d/%d/%d %d:%d:%d:", 1900 + timeP->tm_year, 1 + timeP->tm_mon, timeP->tm_mday, timeP->tm_hour, timeP->tm_min, timeP->tm_sec);
}
DWORD WINAPI receiveMessage(LPVOID lparam)
{
	SOCKET clientSocket = *((SOCKET*)lparam);
	char receiveBuffer[100];
	while (1)
	{
		/*
		receiveBuffer：
		当有新客户端连接后，会广播（"i:%d", id），所有其他客户端检测到第一位为‘i’时，显示新客户端连接
		
		当有客户端发送消息时，第一位会置为m，代表是发送消息。
		当有客户端退出时，第一位会置为e，代表是退出。

		所以服务器端广播的时候，其他客户端检测第一位，判断是另一个客户端发送消息还是退出，进而打印合适的消息。
		*/
		int receiveLen = recv(clientSocket, receiveBuffer, 100, 0);
		if (receiveLen < 0)
		{
			printf("fail to receive\n");
			break;
		}
		else if (receiveBuffer[0] == 'i')//从
		{
			id = receiveBuffer[2] - '0';//
			printf("welocme! client %d\n", id);
		}
		else if (receiveBuffer[0] == 'm')
		{
			printf("client %d message: %s \n", receiveBuffer[99], receiveBuffer + 2);
		}
		else if (receiveBuffer[0] == 'e')//检测到‘e’退出
		{
			printf("client %d exit\n", receiveBuffer[99]);
		}
	}
	return 0;
}
int main()
{
	WSADATA wsaData;//初始化套接字库
	int err;
	err = WSAStartup(MAKEWORD(1, 1), &wsaData);
	if (!err)printf("succeed to open client socket\n");
	else printf("wrong \n");

	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);//流式套接字

	SOCKADDR_IN clientSocket_in;//服务器信息
	clientSocket_in.sin_addr.S_un.S_addr = inet_addr(ip);
	clientSocket_in.sin_family = AF_INET;
	clientSocket_in.sin_port = htons(port);//字节序


	if (SOCKET_ERROR == connect(clientSocket, (SOCKADDR*)&clientSocket_in, sizeof(SOCKADDR)))
	{
		printf("fail to connect\n");
		WSACleanup();
		return 0;
	}
	else printf("succeed to connect\n");



	HANDLE hThread = CreateThread(NULL, NULL, &receiveMessage, &clientSocket, 0, NULL);//接收从服务器传来的消息
	char scanfBuffer[100];
	char sendBuffer[100];
	char timeBuffer[100];
	while (1)//循环来检测输入，所以要创建一个单独接收来自服务器信息的线程
	{
		scanf("%s", &scanfBuffer);//输入

		if (strcmp(scanfBuffer, "exit"))//如果输入的exit，则将第一位置为‘e’，如果是发送消息。则置为‘m’
		{
			getTime(timeBuffer);
			sprintf(sendBuffer, "m:%s %s", timeBuffer, scanfBuffer);//写入sendBuffer中
		}
		else sprintf(sendBuffer, "e:");


		int sendLen = send(clientSocket, sendBuffer, 100, 0);
		if (sendLen < 0)
		{
			printf("fail to send\n");
			break;
		}
		else if (sendBuffer[0] == 'm')printf("successfully send\n");
		else if (sendBuffer[0] == 'e')break;
	}
	CloseHandle(hThread);
	closesocket(clientSocket);
	WSACleanup();
	return 0;
}