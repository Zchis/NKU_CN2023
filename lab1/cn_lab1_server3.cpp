#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <stdio.h>
#include <time.h>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

const int port = 6000;

struct param {//存储客户端的相关信息
	SOCKET s;
	SOCKADDR_IN addr;
	int id;
};

std::vector<param> clients;//管理所有的客户端

void getTime(char* buff) {//调用time库，实现发送信息时间的获取
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

DWORD WINAPI receiveMessage(LPVOID lparam) {//线程函数，接收客户端的信息并进行广播
	param* pp = (param*)lparam;
	SOCKET clientSocket = pp->s;//获取客户端的套接字和地址
	int id = pp->id;

	char receiveBuffer[100];//缓冲区，接收具体的消息
	while (1) {//不断接收客户端的消息
		/*
		receiveBuffer：
		当有新客户端连接后，会广播（"i:%d", id），所有其他客户端检测到第一位为‘i’时，显示新客户端连接

		当有客户端发送消息时，第一位会置为m，代表是发送消息。
		当有客户端退出时，第一位会置为e，代表是退出。

		所以服务器端广播的时候，其他客户端检测第一位，判断是另一个客户端发送消息还是退出，进而打印合适的消息。
		*/
		int receiveLen = recv(clientSocket, receiveBuffer, 100, 0);//从缓冲区接收消息
		if (receiveLen < 0) {//返回字节数
			printf("fail to receive\n");
			break;
		}
		else {
			if (receiveBuffer[0] == 'e') {//判断客户端发送的消息
				printf("client %d exit\n", id);
				closesocket(clientSocket);

				// 从列表移除
				for (auto it = clients.begin(); it != clients.end(); ++it) {
					if (it->s == clientSocket) {
						clients.erase(it);
						break;
					}
				}
				for (const param& client : clients) {//
					if (client.s != clientSocket) {
						sprintf(receiveBuffer, "m: bye!(He left the chat room)\n", id);
						receiveBuffer[99] = id;
						int sendLen = send(client.s, receiveBuffer, 100, 0);
						if (sendLen < 0) {
							printf("server to client %d send error", client.id);
						}
					}
				}
				break;
			}

			printf("client %d: %s \n", id, receiveBuffer);
			//广播
			for (const param& client : clients) {//遍历clients
				if (client.s != clientSocket) {
					receiveBuffer[99] = id;
					int sendLen = send(client.s, receiveBuffer, 100, 0);
					if (sendLen < 0) {
						printf("server to client %d send error", client.id);
					}
				}
			}
		}
	}

	return 0;
}

int main() {
	WSADATA wsaData;//初始化套接字库
	int err = WSAStartup(MAKEWORD(1, 1), &wsaData);
	if (!err) printf("succeed to open server socket\n");
	else printf("wrong \n");
	//创建服务器的套接字
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);//IPv4，数据流传输，默认协议

	SOCKADDR_IN addr;//设置
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//监听所有可用的本地IP地址
	addr.sin_port = htons(port);//使用6000端口号
	addr.sin_family = AF_INET;//ipv4

	int bindRes = bind(serverSocket, (SOCKADDR*)&addr, sizeof(SOCKADDR));//bind绑定函数，将服务器套接字serverSocket与地址信息addr绑定
	if (bindRes == SOCKET_ERROR) printf("fail to bind\n");
	else printf("succeed to bind\n");

	int listenRes = listen(serverSocket, 5);//listen监听函数
	if (listenRes < 0) printf("fail to listen\n");
	else printf("succeed to listen\n");

	int id = 0;
	int len = sizeof(SOCKADDR);

	while (1) {//创建多线程
		param newClient;
		newClient.s = accept(serverSocket, (SOCKADDR*)&newClient.addr, &len);//等待客户端的连接请求并接受连接
		if (newClient.s == SOCKET_ERROR) {
			printf("fail to connect\n");
			WSACleanup();
			return 0;
		}
		clients.push_back(newClient);

		char idBuffer[10];
		sprintf(idBuffer, "i:%d", id);//第一位置为‘i’，代表新客户端加入。第三位写入id，发送到对应的客户端
		send(newClient.s, idBuffer, 10, 0);
		newClient.id = id;
		HANDLE h = CreateThread(NULL, NULL, &receiveMessage, &newClient, 0, NULL);//创建多线程，对于每个连接到服务器的客户端，都会创建一个新线程来处理通信。
		id++;
	}

	closesocket(serverSocket);
	WSACleanup();
	return 0;
}
