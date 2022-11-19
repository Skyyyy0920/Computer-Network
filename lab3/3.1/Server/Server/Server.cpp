#include "Server.h"

#define SERVER_IP "127.0.0.1"  // ������IP��ַ 
#define SERVER_PORT 920  // �������˿ں�
#define PACKET_LENGTH 1024
#define BUFFER_SIZE sizeof(Packet)  // ��������С

SOCKADDR_IN socketAddr;  // ��������ַ
SOCKADDR_IN addrClient;  // �ͻ��˵�ַ
SOCKET socketServer;  // �������׽���

stringstream ss;
SYSTEMTIME sysTime = { 0 };
void printTime() {  // ��ӡϵͳʱ��
	ss.clear();
	ss.str("");
	GetSystemTime(&sysTime);
	ss << "[" << sysTime.wYear << "/" << sysTime.wMonth << "/" << sysTime.wDay << " " << sysTime.wHour + 8 << ":" << sysTime.wMinute << ":" << sysTime.wSecond << ":" << sysTime.wMilliseconds << "]";
	cout << ss.str();
}

void printPacketMessage(Packet* pkt) {  // ��ӡ���ݰ���Ϣ 
	cout << "Packet size=" << pkt->len << " Bytes!  FLAG=" << pkt->FLAG;
	cout << " seqNumber=" << pkt->seq << " ackNumber=" << pkt->ack;
	cout << " checksum=" << pkt->checksum << " windowLength=" << pkt->window << endl;
}

void printSendPacketMessage(Packet* pkt) {
	cout << "[Send Packet's information]";
	printPacketMessage(pkt);
}

void printReceivePacketMessage(Packet* pkt) {
	cout << "[Receive Packet's information]";
	printPacketMessage(pkt);
}

int fromLen = sizeof(SOCKADDR);
int waitSeq;  // �ȴ������ݰ����к�
int err;  // socket������ʾ
int packetNum;  // �������ݰ�������
int fileSize;  // �ļ���С
unsigned int recvSize;  // �ۻ��յ����ļ�λ��
char* fileName;
char* fileBuffer;

default_random_engine randomEngine;
uniform_real_distribution<float> randomNumber(0.0, 1.0);  // �Լ����ö���

void initSocket() {
	WSADATA wsaData;  // �洢��WSAStartup�������ú󷵻ص�Windows Sockets����  
	err = WSAStartup(MAKEWORD(2, 2), &wsaData);  // ָ���汾 2.2
	if (err != NO_ERROR) {
		cout << "WSAStartup failed with error: " << err << endl;
		return;
	}
	socketServer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // ͨ��Э�飺IPv4 InternetЭ��; �׽���ͨ�����ͣ�TCP����;  Э���ض����ͣ�ĳЩЭ��ֻ��һ�����ͣ���Ϊ0

	u_long mode = 1;
	ioctlsocket(socketServer, FIONBIO, &mode); // ��������ģʽ���ȴ����Կͻ��˵����ݰ�

	socketAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);  // 0.0.0.0, ����������IP
	socketAddr.sin_family = AF_INET;  // ʹ��IPv4��ַ
	socketAddr.sin_port = htons(SERVER_PORT);  // �˿ں�

	/*
	* Ϊʲô��������Ҫ�󶨶��ͻ��˲���Ҫ�󶨣�
	*
	* IP��ַ�Ͷ˿ں���������ʶ����ĳһ̨�����ϵľ���һ�����̵ġ�
	* Ҳ����˵���˿ںſ���������ʶ�����ϵ�ĳһ�����̡�
	* ��ˣ�����ϵͳ��Ҫ�Զ˿ںŽ��й������Ҽ�����еĶ˿ں������޵ġ�
	* ��������а󶨣�����ϵͳ���������һ���˿ںŸ���������
	* �������ϵͳ����������������˿ںŵ�ͬʱ��
	* ����������Ҳ׼��ʹ������˿ںŻ���˵�˿ں��Ѿ���ʹ�ã�
	* ����ܻᵼ�·�����һֱ������������
	* ��Σ����������������Ͳ�����ֹͣ�ˣ����ǽ��������˵Ķ˿ںŰ��������й滮�Ķ������еĶ˿ںŽ���ʹ�á�
	* �ͻ�����Ҫ������������˷���������˿ͻ��˾���Ҫ֪���������˵�IP��ַ�Ͷ˿ںţ�
	* ������󶨣���ϵͳ������ɣ��ͻ��˽��޷�֪���������˵Ķ˿ںţ���ʹ֪��Ҳ��Ҫÿ�ζ�ȥ��ȡ��
	* ���ڿͻ�����˵���������˲�����Ҫ�������ͻ��˷������ݣ�
	* �ͻ����������Ķ����������Ǳ����ġ�
	* �ͻ��˸��������˷�������ʱ���Ὣ�Լ���IP��ַ�Ͷ˿ں�һ���͹�ȥ���������˿��Է�����ҵ��ͻ��ˡ�
	* ͬʱ���ͻ��˲�����һֱ���еģ�ֻ��Ҫÿ��ϵͳ������伴�ɡ�
	* ��ˣ�����������Ҫ�󶨶��ͻ��˲���Ҫ�󶨡�
	*/

	err = bind(socketServer, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));  // �󶨵�socketServer
	if (err == SOCKET_ERROR) {
		err = GetLastError();
		cout << "Could not bind the port " << SERVER_PORT << " for socket. Error message: " << err << endl;
		WSACleanup();
		return;
	}
	else {
		printTime();
		cout << "�����������ɹ����ȴ��ͻ��˽�������" << endl;
	}
}

void connect() {
	int state = 0;  // ��ʶĿǰ���ֵ�״̬
	bool flag = 1;
	Packet* sendPkt = new Packet;
	Packet* recvPkt = new Packet;

	while (flag) {
		switch (state) {
		case 0:  // �ȴ��ͻ��˷������ݰ�״̬
			err = recvfrom(socketServer, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &fromLen);
			if (err > 0) {
				if (recvPkt->FLAG & 0x2) {  // SYN=1
					printTime();
					cout << "�յ����Կͻ��˵Ľ������󣬿�ʼ�ڶ������֣���ͻ��˷���ACK,SYN=1�����ݰ�..." << endl;

					// �ڶ������֣���ͻ��˷���ACK,SYN=1�����ݰ�
					sendPkt->setSYNACK();
					sendto(socketServer, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));
					state = 1;  // ת״̬1
				}
				else {
					printTime();
					cout << "��һ�����ֽ׶��յ������ݰ��������¿�ʼ��һ������..." << endl;
				}
			}
			break;

		case 1:  // ���տͻ��˵�ACK=1���ݰ�
			err = recvfrom(socketServer, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &fromLen);
			if (err > 0) {
				if (recvPkt->FLAG & 0x4) {  // ACK=1
					printTime();
					cout << "�յ����Կͻ��˵���������ACK���ݰ�..." << endl;

					state = 2;  // ת״̬2
				}
				else {
					printTime();
					cout << "���������ֽ׶��յ������ݰ��������¿�ʼ����������..." << endl;
					// ��ʵ�������е�����ģ������ߵ�sendto��û�е���һ��״̬��������ʵ�����ظ����ͣ�����������д��
				}
			}
			break;

		case 2: // �������ֽ���״̬
			printTime();
			cout << "�������ֽ�����ȷ���ѽ������ӣ���ʼ�ļ�����..." << endl;
			cout << endl << "**************************************************************************************************" << endl << endl;
			flag = 0;
			break;
		}
	}
}

void disconnect() {  // �ο� <https://blog.csdn.net/LOOKTOMMER/article/details/121307137>
	int state = 0;  // ��ʶĿǰ���ֵ�״̬
	bool flag = 1;
	Packet* sendPkt = new Packet;
	Packet* recvPkt = new Packet;

	u_long mode = 1;
	ioctlsocket(socketServer, FIONBIO, &mode); // ���ó�����ģʽ�ȴ�ACK��Ӧ

	while (flag) {
		switch (state) {
		case 0:  // CLOSE_WAIT_1
			printTime();
			cout << "���յ��ͻ��˵ĶϿ��������󣬿�ʼ�ڶ��λ��֣���ͻ��˷���ACK=1�����ݰ�..." << endl;

			// �ڶ��λ��֣���ͻ��˷���ACK=1�����ݰ�
			sendPkt->setACK();
			sendto(socketServer, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));

			state = 1;  // ת״̬1
			break;

		case 1:  // CLOSE_WAIT_2
			printTime();
			cout << "��ʼ�����λ��֣���ͻ��˷���FIN,ACK=1�����ݰ�..." << endl;

			// �����λ��֣���ͻ��˷���FIN,ACK=1�����ݰ�
			sendPkt->setFINACK();
			sendto(socketServer, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));

			state = 2;  // ת״̬2
			break;

		case 2:  // LAST_ACK
			err = recvfrom(socketServer, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &fromLen);
			if (err > 0) {
				if (recvPkt->FLAG & 0x4) {  // ACK=1
					printTime();
					cout << "�յ������Կͻ��˵��Ĵλ��ֵ�ACK���ݰ�..." << endl;

					state = 3;  // ת״̬3
				}
				else {
					printTime();
					cout << "���Ĵλ��ֽ׶��յ������ݰ��������¿�ʼ���Ĵλ���..." << endl;
				}
			}
			break;

		case 3:  // CLOSE
			printTime();
			cout << "�Ĵλ��ֽ�����ȷ���ѶϿ����ӣ�Bye-bye..." << endl;
			flag = 0;
			break;
		}
	}
}

void saveFile() {
	string filePath = "C:/Users/new/Desktop/";  // ����·��
	for (int i = 0; fileName[i]; i++)filePath += fileName[i];
	ofstream fout(filePath, ios::binary | ios::out);

	fout.write(fileBuffer, fileSize); // ���ﻹ��size,���ʹ��string.data��c_str�Ļ�ͼƬ����ʾ�������������
	fout.close();
}

void receiveFile() {
	int state = 0;
	bool flag = 1;
	waitSeq = 0;
	recvSize = 0;
	Packet* recvPkt = new Packet;
	Packet* sendPkt = new Packet;

	// ѭ�������ļ����ݰ�
	while (flag) {
		switch (state) {
		case 0:  // �ȴ��ļ�ͷ״̬
			err = recvfrom(socketServer, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &fromLen);
			if (err > 0) {
				printReceivePacketMessage(sendPkt);
				if (err > 0) {
					if (isCorrupt(recvPkt)) {  // �������ݰ���
						printTime();
						cout << "�յ�һ���𻵵����ݰ������Ͷ˷��� ack=" << waitSeq << " ���ݰ�" << endl;

						state = 2;
						break;
					}
					if (recvPkt->FLAG & 0x8) {  // HEAD=1
						fileSize = recvPkt->len;
						fileBuffer = new char[fileSize];
						fileName = new char[128];
						memcpy(fileName, recvPkt->data, strlen(recvPkt->data) + 1);
						packetNum = fileSize % PACKET_LENGTH ? fileSize / PACKET_LENGTH + 1 : fileSize / PACKET_LENGTH;

						printTime();
						cout << "�յ����Է��Ͷ˵��ļ�ͷ���ݰ����ļ���Ϊ: " << fileName;
						cout << "���ļ���СΪ: " << fileSize << " Bytes���ܹ���Ҫ���� " << packetNum << " �����ݰ�";
						cout << "���ȴ������ļ����ݰ�..." << endl << endl;

						waitSeq++;
						state = 2;
					}
					else {
						printTime();
						cout << "�յ������ݰ������ļ�ͷ���ȴ����Ͷ��ش�..." << endl;
					}
				}
			}
			break;

		case 1:  // �ȴ��ļ����ݰ�״̬
			err = recvfrom(socketServer, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &fromLen);
			if (err > 0) {
				printReceivePacketMessage(recvPkt);
				if (isCorrupt(recvPkt)) {  // �������ݰ���
					printTime();
					cout << "�յ�һ���𻵵����ݰ������Ͷ˷��� ack=" << waitSeq << " ���ݰ�" << endl << endl;
					state = 2;
					break;
				}
				if (recvPkt->seq == waitSeq) {  // �յ���Ŀǰ�ȴ��İ�
					if (randomNumber(randomEngine) >= 0.02) {
						memcpy(fileBuffer + recvSize, recvPkt->data, recvPkt->len);
						recvSize += recvPkt->len;

						printTime();
						cout << "�յ��� " << recvPkt->seq << " �����ݰ������Ͷ˷��� ack=" << waitSeq << endl << endl;

						waitSeq++;
						state = 2;
					}
					else {
						cout << "��������" << endl;
					}
				}
				else {  // ����Ŀǰ�ȴ������ݰ�
					printTime();
					cout << "�յ��� " << recvPkt->seq << " �����ݰ�������������Ҫ�����ݰ������Ͷ˷��� ack=" << waitSeq << endl << endl;
				}
			}

		case 2:  // ����ACK
			sendPkt->ack = waitSeq - 1;
			sendPkt->setACK();
			sendto(socketServer, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));
			state = 1;
			if (recvSize == fileSize) {  // �ļ��������
				printTime();
				cout << "�ļ��������..." << endl;
				cout << endl << "**************************************************************************************************" << endl << endl;

				// �����ļ�
				saveFile();

				state = 3;
			}
			break;

		case 3:  // �ļ��ѽ�����ϣ��ȴ����Ͷ˷��Ͷ�������
			err = recvfrom(socketServer, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &fromLen);
			if (err > 0) {
				if (recvPkt->FLAG & 0x1) {  // FIN=1
					flag = 0;
					disconnect();
				}
			}
		}

	}
}

int main() {
	// ��ʼ��������socket
	initSocket();

	// ��������
	connect();

	// ��ʼ�����ļ�
	receiveFile();

	// ������ɺ󣬹ر�socket
	err = closesocket(socketServer);
	if (err == SOCKET_ERROR) {
		cout << "Close socket failed with error: " << WSAGetLastError() << endl;
		return 1;
	}

	// �����˳�
	printTime();
	cout << "�����˳�..." << endl;
	WSACleanup();
	return 0;
}