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

#define SERVER_IP    "127.0.0.1"  //服务器IP地址
#define SERVER_PORT  6666		//服务器端口号（改成路由器端口）
#define BUFFER sizeof(Packet)  //缓冲区大小
#define TIMEOUT 5  //超时，单位s，代表一组中的所有ack都正确接收

// #define CLIENT_IP    "127.0.0.2"  // 客户端IP地址


int length = sizeof(SOCKADDR);
int expectSeq = 0;
using namespace std;
SOCKET socketClient;//客户端套接字
SOCKADDR_IN addrServer; //服务器地址
SOCKADDR_IN addrClient; //客户端地址
enum PakcketType {
	//连接请求，服务器已准备，客户端已准备，断开连接请求，服务器接收连接信息
	ConnectRequest, ServerPrepared, ClientPrepared, LeaveRequest, AcceptLeave = 10
};
enum MessageType {
	//发送文件头，文件内容，文件尾部，应答消息，应答文件头消息
	SendHeader = 4, Message = 5, SendTail = 6, replyMessage = 7, replyHeader
};
struct Packet
{
	unsigned char flg;//信息类型标识
	int seq;//序列号
	int ack;//确认ack号
	unsigned short len;//数据部分长度
	unsigned short checksum;//校验和
	char data[8192];//数据长度
	//构造函数
	Packet() :flg(-1), seq(-1), ack(-1), len(8192), checksum(65535) {
		memset(data, 0, sizeof(data));
	}
	Packet(unsigned char flg) :flg(flg), seq(-1), ack(-1), len(8192), checksum(65535) {
		memset(data, 0, sizeof(data));
	}
	//转换成字符串形式输出
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
	//计算校验和
	unsigned short makeCheckSum() {
		unsigned long sum = 0;
		sum += this->flg;
		sum = (sum >> 16) + (sum & 0xffff);//将进位加回到低十六位
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
	//检验数据包是否正确
	bool IsCorrect() {
		unsigned short nowCheckSum = makeCheckSum();//再计算一遍校验和
		//如果相等就说明数据报正确接收
		if (this->checksum == (nowCheckSum & 0xffff))return true;
		return false;
	}
	//包装数据报
	void Package(int seq, int ack) {
		this->seq = seq;
		this->ack = ack;
		this->checksum = makeCheckSum();
	}
};
bool shakeHands() {
	//创建并发送一个连接请求的数据报
	Packet* packet = new Packet(ConnectRequest);
	sendto(socketClient, (char*)packet, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
	//发送成功的提示信息
	cout << "Send Request Successfully!" << endl;
	cout << "Wait for Server..." << endl;
	int state = ConnectRequest;//记录当前的客户端状态为连接请求状态
	bool serverflg = 0;//服务器是否已准备的标识

	//持续监听服务器信息并与之进行握手
	while (true) {
		switch (state) {//根据当前状态进行对应操作
		case ConnectRequest: {//连接请求状态:此时客户端持续监听服务器发来的SeverPrepared消息。
			int recvSize = recvfrom(socketClient, (char*)packet, sizeof(*packet), 0, (SOCKADDR*)&addrServer,
				&length);
			if (recvSize > 0 && packet->flg == ServerPrepared) {//如果是服务器已准备的信息类型
				state = ServerPrepared;//将状态置为服务器已准备
			}
			break;
		}
		case ServerPrepared: {//服务器已准备状态：向服务器发送客户端已准备的数据报，并成功返回，表示握手成功
			Packet* clientPreparedPacket = new Packet(ClientPrepared);
			if (!serverflg) {//输出提示信息
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
	Packet* packet = new Packet(LeaveRequest);// 发送LeaveRequest数据包
	packet->Package(-1, -1);
	sendto(socketClient, (char*)packet, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
	while (true) {// 发送之后就等待服务器端确认
		Packet* serverPacket = new Packet();
		int recvSize = recvfrom(socketClient, (char*)serverPacket, sizeof(*packet), 0, (SOCKADDR*)&addrServer,
			&length);
		if (recvSize > 0 && serverPacket->IsCorrect() && serverPacket->flg == AcceptLeave) {// 服务器端发送确认断开AcceptLeave的数据包
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
	int error = WSAStartup(wVersionRequested, &wsaData);//加载套接字库
	if (error) {
		cout << "找不到winsock.dll" << endl;
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
			// 输出本地IP地址
			// localAddr.sin_addr.S_un.S_addr = inet_addr(CLIENT_IP);// 指定端口
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
			//如果数据报成功接收
			if (recvPacket->IsCorrect()) {
				//如果接收到的seq就是期望的seq，那么就正常处理
				if (recvPacket->seq == expectSeq) {
					if (recvPacket->flg == SendHeader) {// 如果接收到的是头部文件
						cout << "You got a file from Server: " << recvPacket->data << endl;
						cout << "Please select an directory (include filename and filetyle as D:\\1.txt): ";
						string downloadDir;
						cin >> downloadDir;
						outfile.open(downloadDir, ios::binary);
						replyPacket->flg = replyHeader;
					}
					if (recvPacket->flg == Message) {// 文件体
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
					replyPacket->Package(-1, expectSeq++);// 客服端不发生数据，只确认
					sendto(socketClient, (char*)replyPacket, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
				}
				else {
					//否则输出提示信息，并重传对应Ack序列号的应答数据报
					cout << "----------------------------------------------------------------" << endl;
					cout << "This Packet has been sent before. Maybe your Reply Packet lost." << endl;
					cout << "This Packet has been sent before. Maybe your Reply Packet lost." << endl;
					cout << "This Packet has been sent before. Maybe your Reply Packet lost." << endl;
					cout << "----------------------------------------------------------------" << endl;
					replyPacket->ack = recvPacket->seq;
					replyPacket->Package(-1, recvPacket->seq);// 重新发一遍请求
					sendto(socketClient, (char*)replyPacket, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
				}
			}
		}
		waveHands();
	}
	return 0;
}
