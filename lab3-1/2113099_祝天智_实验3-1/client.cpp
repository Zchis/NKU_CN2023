#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#pragma comment(lib,"ws2_32.lib")
#include <stdlib.h>
#include <time.h>
#include <WinSock2.h>
#include <fstream>
#include <iostream>

#include <vector>
#include <string>
#include <ws2tcpip.h>

#define SERVER_IP    "127.0.0.1"  //������IP��ַ
#define SERVER_PORT  6666		//�������˿ںţ��ĳ�·�����˿ڣ�
#define BUFFER sizeof(Packet)  //��������С
#define TIMEOUT 5  //��ʱ����λs������һ���е�����ack����ȷ����

// #define CLIENT_IP    "127.0.0.2"  // �ͻ���IP��ַ


int length = sizeof(SOCKADDR);
int expectSeq = 0;
using namespace std;
SOCKET socketClient;//�ͻ����׽���
SOCKADDR_IN addrServer; //��������ַ
SOCKADDR_IN addrClient; //�ͻ��˵�ַ
enum PakcketType {
	//�������󣬷�������׼�����ͻ�����׼�����Ͽ��������󣬷���������������Ϣ
	ConnectRequest, ServerPrepared, ClientPrepared, LeaveRequest, AcceptLeave = 10
};
enum MessageType {
	//�����ļ�ͷ���ļ����ݣ��ļ�β����Ӧ����Ϣ��Ӧ���ļ�ͷ��Ϣ
	SendHeader = 4, Message = 5, SendTail = 6, replyMessage = 7, replyHeader
};
struct Packet
{
	unsigned char flg;//��Ϣ���ͱ�ʶ
	int seq;//���к�
	int ack;//ȷ��ack��
	unsigned short len;//���ݲ��ֳ���
	unsigned short checksum;//У���
	char data[8192];//���ݳ���
	//���캯��
	Packet() :flg(-1), seq(-1), ack(-1), len(8192), checksum(65535) {
		memset(data, 0, sizeof(data));
	}
	Packet(unsigned char flg) :flg(flg), seq(-1), ack(-1), len(8192), checksum(65535) {
		memset(data, 0, sizeof(data));
	}
	//ת�����ַ�����ʽ���
	void toString() {
		cout << "-------------Packet-----------------" << endl;
		switch (flg) {
		case ConnectRequest: {
			cout << "flg: ConnectRequest" << endl;
			break;
		}
		case ServerPrepared: {
			cout << "flg: ServerPrepared" << endl;
			break;

		}
		case ClientPrepared: {
			cout << "flg: ClientPrepared" << endl;
			break;
		}
		case SendHeader: {
			cout << "flg: SendHeader" << endl;
			break;
		}
		case Message: {
			cout << "flg: Message" << endl;
			break;
		}
		case SendTail: {
			cout << "flg: SendTail" << endl;
			break;
		}
		case replyMessage: {
			cout << "flg: replyMessage" << endl;
			break;
		}
		}
		cout << "seq: " << seq << endl;
		cout << "ack: " << ack << endl;
		cout << "len: " << len << endl;
		cout << "checksum: " << checksum << endl;
		if (flg == SendHeader) {
			cout << "File DIR: ";
			for (int i = 0; i < len; i++) {
				cout << data[i];
			}
			cout << endl;
		}
		cout << "---------------------------------------" << endl;
	}
	//����У���
	unsigned short makeCheckSum() {
		unsigned long sum = 0;
		sum += this->flg;
		sum = (sum >> 16) + (sum & 0xffff);//����λ�ӻص���ʮ��λ
		sum += this->seq;
		sum = (sum >> 16) + (sum & 0xffff);
		sum += this->ack;
		sum = (sum >> 16) + (sum & 0xffff);
		sum += this->len;
		sum = (sum >> 16) + (sum & 0xffff);
		for (int i = 0; i < 8192; i++) {
			sum += this->data[i];
			sum = (sum >> 16) + (sum & 0xffff);
		}
		return ~sum;
	}
	//�������ݰ��Ƿ���ȷ
	bool IsCorrect() {
		unsigned short nowCheckSum = makeCheckSum();//�ټ���һ��У���
		//�����Ⱦ�˵�����ݱ���ȷ����
		if (this->checksum == (nowCheckSum & 0xffff))return true;
		return false;
	}
	//��װ���ݱ�
	void Package(int seq, int ack) {
		this->seq = seq;
		this->ack = ack;
		this->checksum = makeCheckSum();
	}
};
bool shakeHands() {
	//����������һ��������������ݱ�
	Packet* packet = new Packet(ConnectRequest);
	sendto(socketClient, (char*)packet, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
	//���ͳɹ�����ʾ��Ϣ
	cout << "Send Request Successfully!" << endl;
	cout << "Wait for Server..." << endl;
	int state = ConnectRequest;//��¼��ǰ�Ŀͻ���״̬Ϊ��������״̬
	bool serverflg = 0;//�������Ƿ���׼���ı�ʶ

	//����������������Ϣ����֮��������
	while (true) {
		switch (state) {//���ݵ�ǰ״̬���ж�Ӧ����
		case ConnectRequest: {//��������״̬:��ʱ�ͻ��˳�������������������SeverPrepared��Ϣ��
			int recvSize = recvfrom(socketClient, (char*)packet, sizeof(*packet), 0, (SOCKADDR*)&addrServer,
				&length);
			if (recvSize > 0 && packet->flg == ServerPrepared) {//����Ƿ�������׼������Ϣ����
				state = ServerPrepared;//��״̬��Ϊ��������׼��
			}
			break;
		}
		case ServerPrepared: {//��������׼��״̬������������Ϳͻ�����׼�������ݱ������ɹ����أ���ʾ���ֳɹ�
			Packet* clientPreparedPacket = new Packet(ClientPrepared);
			if (!serverflg) {//�����ʾ��Ϣ
				serverflg = true;
				cout << "Server has Prepared.";
			}
			sendto(socketClient, (char*)clientPreparedPacket, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
			return true;
		}
		}
	}
}
bool waveHands() {
	Packet* packet = new Packet(LeaveRequest);// ����LeaveRequest���ݰ�
	packet->Package(-1, -1);
	sendto(socketClient, (char*)packet, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
	while (true) {// ����֮��͵ȴ���������ȷ��
		Packet* serverPacket = new Packet();
		int recvSize = recvfrom(socketClient, (char*)serverPacket, sizeof(*packet), 0, (SOCKADDR*)&addrServer,
			&length);
		if (recvSize > 0 && serverPacket->IsCorrect() && serverPacket->flg == AcceptLeave) {// �������˷���ȷ�϶Ͽ�AcceptLeave�����ݰ�
			cout << "Server accept your Leave Request." << endl;
			cout << "Good Bye." << endl;
			return true;
		}
	}
	return false;
}

int main() {
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	int error = WSAStartup(wVersionRequested, &wsaData);//�����׽��ֿ�
	if (error) {
		cout << "�Ҳ���winsock.dll" << endl;
		return 0;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		cout << "Could not find a usable version of Winsock.dll" << endl;
		WSACleanup();
	}
	else
	{
		cout << "Socket Created Successfully." << endl;
	}
	socketClient = socket(AF_INET, SOCK_DGRAM, 0);
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(6666);
	//server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, "127.0.0.1", &addrServer.sin_addr.S_un.S_addr);

	addrClient.sin_family = AF_INET;
	addrClient.sin_port = htons(2345);
	//server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, "127.0.0.1", &addrClient.sin_addr.S_un.S_addr);

	if (bind(socketClient, (sockaddr*)&addrClient, sizeof(addrServer)) == SOCKET_ERROR) {
		std::cerr << "Failed to bind socket" << std::endl;
	}
	else {
		sockaddr_in localAddr;
		int addrLen = sizeof(localAddr);
		if (getsockname(socketClient, (sockaddr*)&localAddr, &addrLen) == SOCKET_ERROR) {
			std::cerr << "Failed to get local address" << std::endl;
		}
		else {
			// �������IP��ַ
			// localAddr.sin_addr.S_un.S_addr = inet_addr(CLIENT_IP);// ָ���˿�
			char ipStr[INET_ADDRSTRLEN];
			unsigned short port;

			inet_ntop(AF_INET, &(localAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
			port = ntohs(localAddr.sin_port);

			std::cout << "Local IP Address: " << ipStr << std::endl;
			std::cout << "Local Port: " << port << std::endl;

		}
	}


	if (shakeHands()) {
		int recvSize;
		ofstream outfile;
		while (true) {
			Packet* recvPacket = new Packet();
			Packet* replyPacket = new Packet(replyMessage);
			recvSize = recvfrom(socketClient, (char*)recvPacket, sizeof(*recvPacket), 0, (SOCKADDR*)&addrServer,
				&length);
			recvPacket->toString();
			//������ݱ��ɹ�����
			if (recvPacket->IsCorrect()) {
				//������յ���seq����������seq����ô����������
				if (recvPacket->seq == expectSeq) {
					if (recvPacket->flg == SendHeader) {// ������յ�����ͷ���ļ�
						cout << "You got a file from Server: " << recvPacket->data << endl;
						cout << "Please select an directory (include filename and filetyle as D:\\1.txt): ";
						string downloadDir;
						cin >> downloadDir;
						outfile.open(downloadDir, ios::binary);
						replyPacket->flg = replyHeader;
					}
					if (recvPacket->flg == Message) {// �ļ���
						cout << "receive correct Packet,seq = " << recvPacket->seq << endl;
						outfile.write(recvPacket->data, recvPacket->len);
					}
					if (recvPacket->flg == SendTail) {
						cout << "receive correct Packet,seq = " << recvPacket->seq << endl;
						cout << "Finish DownLoading." << endl;
						outfile.write(recvPacket->data, recvPacket->len);
						outfile.close();
						replyPacket->Package(-1, expectSeq++);
						sendto(socketClient, (char*)replyPacket, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
						cout << "Do you wanna leave?(y/n)" << endl;
						char option;
						cin >> option;
						if (option == 'y')break;
						continue;
					}
					replyPacket->Package(-1, expectSeq++);// �ͷ��˲��������ݣ�ֻȷ��
					sendto(socketClient, (char*)replyPacket, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
				}
				else {
					//���������ʾ��Ϣ�����ش���ӦAck���кŵ�Ӧ�����ݱ�
					cout << "----------------------------------------------------------------" << endl;
					cout << "This Packet has been sent before. Maybe your Reply Packet lost." << endl;
					cout << "This Packet has been sent before. Maybe your Reply Packet lost." << endl;
					cout << "This Packet has been sent before. Maybe your Reply Packet lost." << endl;
					cout << "----------------------------------------------------------------" << endl;
					replyPacket->ack = recvPacket->seq;
					replyPacket->Package(-1, recvPacket->seq);// ���·�һ������
					sendto(socketClient, (char*)replyPacket, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
				}
			}
		}
		waveHands();
	}
	return 0;
}
