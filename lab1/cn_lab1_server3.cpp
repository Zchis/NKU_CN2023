#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <stdio.h>
#include <time.h>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

const int port = 6000;

struct param {//�洢�ͻ��˵������Ϣ
	SOCKET s;
	SOCKADDR_IN addr;
	int id;
};

std::vector<param> clients;//�������еĿͻ���

void getTime(char* buff) {//����time�⣬ʵ�ַ�����Ϣʱ��Ļ�ȡ
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

DWORD WINAPI receiveMessage(LPVOID lparam) {//�̺߳��������տͻ��˵���Ϣ�����й㲥
	param* pp = (param*)lparam;
	SOCKET clientSocket = pp->s;//��ȡ�ͻ��˵��׽��ֺ͵�ַ
	int id = pp->id;

	char receiveBuffer[100];//�����������վ������Ϣ
	while (1) {//���Ͻ��տͻ��˵���Ϣ
		/*
		receiveBuffer��
		�����¿ͻ������Ӻ󣬻�㲥��"i:%d", id�������������ͻ��˼�⵽��һλΪ��i��ʱ����ʾ�¿ͻ�������

		���пͻ��˷�����Ϣʱ����һλ����Ϊm�������Ƿ�����Ϣ��
		���пͻ����˳�ʱ����һλ����Ϊe���������˳���

		���Է������˹㲥��ʱ�������ͻ��˼���һλ���ж�����һ���ͻ��˷�����Ϣ�����˳���������ӡ���ʵ���Ϣ��
		*/
		int receiveLen = recv(clientSocket, receiveBuffer, 100, 0);//�ӻ�����������Ϣ
		if (receiveLen < 0) {//�����ֽ���
			printf("fail to receive\n");
			break;
		}
		else {
			if (receiveBuffer[0] == 'e') {//�жϿͻ��˷��͵���Ϣ
				printf("client %d exit\n", id);
				closesocket(clientSocket);

				// ���б��Ƴ�
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
			//�㲥
			for (const param& client : clients) {//����clients
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
	WSADATA wsaData;//��ʼ���׽��ֿ�
	int err = WSAStartup(MAKEWORD(1, 1), &wsaData);
	if (!err) printf("succeed to open server socket\n");
	else printf("wrong \n");
	//�������������׽���
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);//IPv4�����������䣬Ĭ��Э��

	SOCKADDR_IN addr;//����
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//�������п��õı���IP��ַ
	addr.sin_port = htons(port);//ʹ��6000�˿ں�
	addr.sin_family = AF_INET;//ipv4

	int bindRes = bind(serverSocket, (SOCKADDR*)&addr, sizeof(SOCKADDR));//bind�󶨺��������������׽���serverSocket���ַ��Ϣaddr��
	if (bindRes == SOCKET_ERROR) printf("fail to bind\n");
	else printf("succeed to bind\n");

	int listenRes = listen(serverSocket, 5);//listen��������
	if (listenRes < 0) printf("fail to listen\n");
	else printf("succeed to listen\n");

	int id = 0;
	int len = sizeof(SOCKADDR);

	while (1) {//�������߳�
		param newClient;
		newClient.s = accept(serverSocket, (SOCKADDR*)&newClient.addr, &len);//�ȴ��ͻ��˵��������󲢽�������
		if (newClient.s == SOCKET_ERROR) {
			printf("fail to connect\n");
			WSACleanup();
			return 0;
		}
		clients.push_back(newClient);

		char idBuffer[10];
		sprintf(idBuffer, "i:%d", id);//��һλ��Ϊ��i���������¿ͻ��˼��롣����λд��id�����͵���Ӧ�Ŀͻ���
		send(newClient.s, idBuffer, 10, 0);
		newClient.id = id;
		HANDLE h = CreateThread(NULL, NULL, &receiveMessage, &newClient, 0, NULL);//�������̣߳�����ÿ�����ӵ��������Ŀͻ��ˣ����ᴴ��һ�����߳�������ͨ�š�
		id++;
	}

	closesocket(serverSocket);
	WSACleanup();
	return 0;
}
