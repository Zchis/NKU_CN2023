#pragma comment(lib,"ws2_32.lib")
#include <stdlib.h>
#include <time.h>
#include <WinSock2.h>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <string> 
#include <ws2tcpip.h>

#define SERVER_IP    "127.0.0.1"  // ������IP��ַ 
#define SERVER_PORT  6666		// �������˿ں�
#define BUFFER sizeof(Packet)  //��������С
#define TIMEOUT 5  //��ʱ����λs������һ���е�����ack����ȷ����

using namespace std;

SOCKADDR_IN addrServer;   //��������ַ
SOCKADDR_IN addrClient;   //�ͻ��˵�ַ

SOCKET sockServer;//�������׽���
SOCKET sockClient;//�ͻ����׽���
int recvSize;
int length = sizeof(SOCKADDR);
int nowSeq = 0;
int nowAck = 0;

enum ConnectionState {// ö�٣����ݰ�����
	ConnectRequest, ServerPrepared, ClientPrepared, LeaveRequest, AcceptLeave = 10
};
enum MessageType {
	SendHeader = 4, Message = 5, SendTail = 6, replyMessage = 7, replyHeader = 8
};
struct Packet
{
	unsigned char flg;//���ӽ������Ͽ���ʶ����ʾ���ݰ�����
	int seq;//���к�
	int ack;//ȷ�Ϻ�
	unsigned short len;//���ݲ��ֳ���
	unsigned short checksum;//У���
	char data[8192];//���ݳ���
	Packet() :flg(-1), seq(-1), ack(-1), len(8192), checksum(65535) {// ���캯����ʼ��
		memset(data, 0, sizeof(data));
	}
	Packet(unsigned char flg) :flg(flg), seq(-1), ack(-1), len(8192), checksum(65535) {// �в����Ĺ��캯��
		memset(data, 0, sizeof(data));
	}

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
	}
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
		return ~sum;//��λȡ����������ն˱ȶ�
	}
	void Package(int seq, int ack) {
		this->seq = seq;
		this->ack = ack;
		this->checksum = makeCheckSum();
	}
	bool IsCorrect() {// ����
		unsigned short nowCheckSum = makeCheckSum();
		if (this->checksum == (nowCheckSum & 0xffff))return true;
		return false;
	}
};
//���ֽ�������
bool shakeHands() {
	Packet* packet = new Packet();// �������ݱ�׼����������
	int state = -1;// ��ʼ״̬Ϊ�ȴ��ͻ�������״̬
	while (true) {// ���������ȴ��ͻ�������
		recvSize = recvfrom(sockServer, (char *)packet, BUFFER, 0, ((SOCKADDR *)&addrClient), &length);
		if (recvSize > 0 && packet->flg == ConnectRequest) {
			state = ConnectRequest;// ����յ����󣬿�ʼ����

			while (true) {
				switch (state) {
				case ConnectRequest: {// �յ��ͻ�������󣬷��ͷ�������׼�����ݱ�
					cout << "Get Connection Request...PleaseWait" << endl;
					Packet* serverPacket = new Packet(ServerPrepared);// ����һ��flgΪServerPrepared�����ݰ�����ʾ���������Ѿ�׼������
					sendto(sockServer, (char*)serverPacket, BUFFER, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
					cout << "send successfully" << endl;
					state = ServerPrepared;// ����״̬
					break;
				}
				case ServerPrepared: {//��������׼�������������ͻ�����׼������Ϣ
					recvSize = recvfrom(sockServer, (char *)packet, BUFFER, 0, ((SOCKADDR *)&addrClient), &length);
					if (recvSize > 0 && packet->flg == ClientPrepared) {// ������յ������ݰ���flgΪClientPrepared�����ͷ�����������˳ɹ���������
						cout << "Client Connected Successfully." << endl;
						return true;
					}
				}
				}
			}
		}
		else {//ÿ2s���һ���Ƿ��пͻ�������
			cout << "No Client Request." << endl;
			Sleep(2000);
			continue;
		}
	}
	return false;
}
//���ֶϿ�����
bool waveHands() {
	Packet* packet = new Packet();
	while (true) {
		recvSize = recvfrom(sockServer, (char *)packet, BUFFER, 0, ((SOCKADDR *)&addrClient), &length);
		if (recvSize > 0 && packet->flg == LeaveRequest && packet->IsCorrect()) {// ������յ�flg����LeaveRequest�����ݰ������ͷ�������Ͽ�����
			cout << "The Client is about to leave." << endl;
			Packet* serverPacket = new Packet(AcceptLeave);// ����һ��ȷ�϶Ͽ������ݰ��Ͽ�����
			serverPacket->Package(-1, -1);
			sendto(sockServer, (char*)serverPacket, BUFFER, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
			return true;
		}
	}
	return false;
}
int main()
{
	WORD wVersionRequested = MAKEWORD(2, 2); //winSocket2.2
	WSADATA wsaData;
	int error = WSAStartup(wVersionRequested, &wsaData);//������ʾ
	if (error) {
		cout << "�Ҳ���winsock.dll" << endl;
		return 0;
	}
	sockServer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);// �������ݱ��׽��ֿ�
	int iMode = 1; 
	ioctlsocket(sockServer, FIONBIO, (u_long FAR*) & iMode);// �����׽���Ϊ������ģʽ��ioctlsocket(sockServer, FIONBIO, &iMode);
	// addrServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);// �����κο��õ�����ӿ�
	// addrServer.sin_family = AF_INET;// ipv4
	// addrServer.sin_port = htons(SERVER_PORT);// �������ض��˿�
	// inet_pton(AF_INET, "127.0.0.1", &(addrServer.sin_addr)); // �󶨵��̶���IP��ַ

	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(12345);
	//server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, "127.0.0.1", &addrServer.sin_addr.S_un.S_addr);

	addrClient.sin_family = AF_INET;
	addrClient.sin_port = htons(6666);
	//server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, "127.0.0.1", &addrClient.sin_addr.S_un.S_addr);


	error = bind(sockServer, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));// �׽��ֺ�ָ���� IP ��ַ�Ͷ˿ڽ��а�
	if (error) {
		cout << "�˿ڰ�ʧ��: " << SERVER_PORT << endl;
		WSACleanup();
		return 0;
	}
	else
	{
		cout << "�����������ɹ�" << endl;
	}
	sockaddr_in localAddr;
	int addrLen = sizeof(localAddr);
	if (getsockname(sockServer, (sockaddr*)&localAddr, &addrLen) == SOCKET_ERROR) {
		std::cerr << "Failed to get local address" << std::endl;
	}
	else {
		char ipStr[INET_ADDRSTRLEN];
		unsigned short port;

		inet_ntop(AF_INET, &(localAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
		port = ntohs(localAddr.sin_port);

		std::cout << "Local IP Address: " << ipStr << std::endl;
		std::cout << "Local Port: " << port << std::endl;

	}


	while (true) {
		if (shakeHands()) {
			string filepath;

			while (true) {
				cout << "Input your file path: ";
				cin >> filepath;
				ifstream infile(filepath, ifstream::in | ios::binary);//�Զ����Ʒ�ʽ���ļ�
				if (!infile.is_open()) {
					cout << "Cannot open the file" << endl;
					continue;
				}
				infile.seekg(0, std::ios_base::end);  //���ļ�ָ�붨λ���ļ�β
				int fileLength = infile.tellg();
				int packetnum = (fileLength + 8191) / 8192;// �������ݰ�����
				cout << "The File " << filepath << " contains " << fileLength << " Bytes, " << packetnum
					<< "packets in total" << endl;
				infile.seekg(0, std::ios_base::beg);  //���ļ�ָ��ָ��ͷ��
				//��װ�ļ�ͷ���ݱ�
				Packet *HeaderPackage = new Packet(SendHeader);// flg����ΪSendHeader
				for (int i = 0; i < filepath.length(); i++) { HeaderPackage->data[i] = filepath[i]; }// ����װ�����ļ�·��
				HeaderPackage->len = filepath.length();//���ݳ���
				HeaderPackage->Package(nowSeq, -1);//����seq,ack,checksum
				sendto(sockServer, (char *)HeaderPackage, BUFFER, 0, (SOCKADDR *)&addrClient, sizeof(SOCKADDR));// �����ļ�ͷ������֪�ļ���Ϣ
				cout << "Send FileHeader successfully. seq = " << nowSeq++ << endl;

				Packet *recvHeader = new Packet();
				while (true) {
					recvSize = recvfrom(sockServer, (char *)recvHeader, BUFFER, 0, ((SOCKADDR *)&addrClient),
						&length);
					if (recvSize < 0) {
						Sleep(200);
						continue;
					}
					if (recvHeader->flg == replyHeader && recvHeader->IsCorrect() && recvHeader->ack == nowAck) {// ������յ�flg��replyHeader
						cout << "Client Prepared to receive File." << endl;
						nowAck++;
						break;
					}
				}

				clock_t begin = clock();  // ��¼��ʼʱ��
				//��ʼ˳����
				for (int i = 1; i <= packetnum; i++) {
					Packet *MessagePacket = new Packet(Message);// flg = Message
					Packet *recvPacket = new Packet();
					int dataLength = min(8192, fileLength);
					fileLength -= 8192;// ��¼ʣ���ļ�����
					infile.read(MessagePacket->data, dataLength);
					MessagePacket->len = dataLength;
					if (i == packetnum)MessagePacket->flg = SendTail;// ������ļ�ĩβ��Ҫ�������SendTail��ʶ
					MessagePacket->Package(nowSeq, -1);// ����ֻ������ȷ��
					sendto(sockServer, (char *)MessagePacket, BUFFER, 0, (SOCKADDR *)&addrClient, sizeof(SOCKADDR));
					cout << "Send File section��seq = " << nowSeq++ << endl;// ����һ�����ݰ���seq��һ
					int waittime = 0;// ��¼�ȴ�ʱ�䣬ʵ�ֳ�ʱ�ش�����
					while (true) {
						recvSize = recvfrom(sockServer, (char *)recvPacket, BUFFER, 0, ((SOCKADDR *)&addrClient),
							&length);
						// ��ʱ�ش�
						if (recvSize < 0) {
							Sleep(200);// ÿ200������һ��Ӧ����Ϣ
							waittime++;
							if (waittime > 10) {// ����10�μ�黹û���յ�����ΪӦ��ʱ
								cout << "Reply Time Out" << endl;
								sendto(sockServer, (char *)MessagePacket, BUFFER, 0, (SOCKADDR *)&addrClient,// ���·������ݰ�
									sizeof(SOCKADDR));
								waittime = 0;// �ȴ�ʱ������
							}
							continue;
						}
						// ������ݱ�����ΪreplyMessageӦ����Ϣ���������ݱ���ȷ���ͣ�Ӧ����ϢAck��������Ack�����
						if (recvPacket->flg == replyMessage && recvPacket->IsCorrect() && recvPacket->ack == nowAck) {
							//�����ʾ��Ϣ
							cout << "get receive Message from Client. ack = " << nowAck << endl;
							nowAck++;
							break;
						}
						else {//�������Ҫ���´��䵱ǰ���ݱ�
							cout << "--------------------------------------------get error reply.----------------------------------------" << endl;
							cout << "--------------------------------------------get error reply.----------------------------------------" << endl;
							cout << "--------------------------------------------get error reply.----------------------------------------" << endl;
							sendto(sockServer, (char *)MessagePacket, BUFFER, 0, (SOCKADDR *)&addrClient,
								sizeof(SOCKADDR));
							waittime = 0;//ˢ�µȴ�ʱ��
						}
					}
				}

				clock_t end = clock();  // ��¼����ʱ��
				cout << "Finish uploading." << endl;

				double transferTime = double(end - begin) / CLOCKS_PER_SEC;
				cout << "����ʱ��: " << transferTime * 1000 << "ms" << endl;  // ת��Ϊ����
				cout << "������: " << -(double(fileLength) / transferTime) << "bytes/s" << endl;
				cout << "Do you want to upload again?(y/n)" << endl;// �˳�����
				char option;
				cin >> option;
				if (option == 'n')break;
			}
			waveHands();// ���֣��Ͽ�����
		}
	}
	return 0;
}
