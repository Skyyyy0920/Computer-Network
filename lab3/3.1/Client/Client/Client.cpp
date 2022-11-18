#include "Client.h"

#define SERVER_IP "127.0.0.1"  // ��������IP��ַ
#define SERVER_PORT 920
#define BUFFER_SIZE sizeof(Packet)  // ��������С
#define WINDOW_SIZE 20
#define PACKET_LENGTH 1024
#define MAX_TIME 2 * CLOCKS_PER_SEC  // ��ʱ�ش�

SOCKADDR_IN socketAddr;  // ��������ַ
SOCKET socketClient;  // �ͻ����׽���

stringstream ss;
SYSTEMTIME sysTime = { 0 };  // ϵͳʱ��
void printTime() {  // ��ӡϵͳʱ��
	ss.clear();
	ss.str("");
	GetSystemTime(&sysTime);
	ss << "[" << sysTime.wYear << "/" << sysTime.wMonth << "/" << sysTime.wDay << " " << sysTime.wHour + 8 << ":" << sysTime.wMinute << ":" << sysTime.wSecond << ":" << sysTime.wMilliseconds << "]";
	cout << ss.str();
}

int fromLen = sizeof(SOCKADDR);
int packetNum;  // �������ݰ�������
int fileSize;  // �ļ���С
int sendSeq;  // �������ݰ����к�
int err;  // socket������ʾ
char* filePath;
char* fileBuffer;

void initSocket() {
	WSADATA wsaData;
	err = WSAStartup(MAKEWORD(2, 2), &wsaData);  // �汾 2.2 
	if (err != NO_ERROR)
	{
		cout << "WSAStartup failed with error: " << err << endl;
		return;
	}
	socketClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	socketAddr.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = htons(SERVER_PORT);

	printTime();
	cout << "�ͻ��˳�ʼ���ɹ���׼�����������������" << endl;
}

void connect() {
	int state = 0;  // ��ʶĿǰ���ֵ�״̬
	bool flag = 1;
	Packet* sendPkt = new Packet;
	Packet* recvPkt = new Packet;

	u_long mode = 1;
	ioctlsocket(socketClient, FIONBIO, &mode); // ���ó�����ģʽ�ȴ�ACK��Ӧ

	while (flag) {
		switch (state) {
		case 0:  // ����SYN=1���ݰ�״̬
			printTime();
			cout << "��ʼ��һ�����֣������������SYN=1�����ݰ�..." << endl;

			// ��һ�����֣������������SYN=1�����ݰ����������յ�����ӦSYN, ACK=1�����ݰ�
			sendPkt->setSYN();
			sendto(socketClient, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));
			cout << socketAddr.sin_port << endl;
			state = 1;  // ת״̬1
			break;

		case 1:  // �ȴ��������ظ�
			err = recvfrom(socketClient, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &fromLen);
			if (err >= 0) {
				if ((recvPkt->FLAG & 0x2) && (recvPkt->FLAG & 0x4)) {  // SYN=1, ACK=1
					printTime();
					cout << "��ʼ���������֣������������ACK=1�����ݰ�..." << endl;

					// ���������֣������������ACK=1�����ݰ�����֪�������Լ�������������
					sendPkt->setACK();
					sendto(socketClient, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));
					cout << socketAddr.sin_port << endl;
					state = 2;  // ת״̬2
				}
				else {
					printTime();
					cout << "�ڶ������ֽ׶��յ������ݰ��������¿�ʼ��һ������..." << endl;
					state = 0;  // ת״̬0
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
	ioctlsocket(socketClient, FIONBIO, &mode); // ���ó�����ģʽ�ȴ�ACK��Ӧ

	while (flag) {
		switch (state) {
		case 0:  // ����FIN=1���ݰ�״̬
			printTime();
			cout << "��ʼ��һ�λ��֣������������FIN=1�����ݰ�..." << endl;

			// ��һ�λ��֣������������FIN=1�����ݰ�
			sendPkt->setFIN();
			sendto(socketClient, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));

			state = 1;  // ת״̬1
			break;

		case 1:  // FIN_WAIT_1
			err = recvfrom(socketClient, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &fromLen);
			if (err >= 0) {
				cout << "Recieve Message " << recvPkt->len << " Bytes!";
				cout << " Flag:" << recvPkt->FLAG << " SEQ:" << recvPkt->seq;
				cout << " checksum:" << recvPkt->checksum << endl;
				if (recvPkt->FLAG & 0x4) {  // ACK=1
					printTime();
					cout << "�յ������Է������ڶ��λ���ACK���ݰ�..." << endl;

					state = 2;  // ת״̬2
				}
				else {
					printTime();
					cout << "�ڶ��λ��ֽ׶��յ������ݰ��������¿�ʼ�ڶ��λ���..." << endl;
				}
			}
			break;
		
		case 2:  // FIN_WAIT_2
			err = recvfrom(socketClient, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &fromLen);
			if (err >= 0) {
				cout << "Recieve Message " << recvPkt->len << " Bytes!";
				cout << " Flag:" << recvPkt->FLAG << " SEQ:" << recvPkt->seq;
				cout << " checksum:" << recvPkt->checksum << endl;
				if ((recvPkt->FLAG & 0x1) && (recvPkt->FLAG & 0x4)) {  // ACK=1, FIN=1
					printTime();
					cout << "�յ������Է����������λ���FIN&ACK���ݰ�����ʼ���Ĵλ��֣������������ACK=1�����ݰ�..." << endl;

					// ���Ĵλ��֣������������ACK=1�����ݰ���֪ͨ������ȷ�϶Ͽ�����
					sendPkt->setFINACK();
					sendto(socketClient, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));
					state = 3;  // ת״̬3
				}
				else {
					printTime();
					cout << "�����λ��ֽ׶��յ������ݰ��������¿�ʼ�����λ���..." << endl;
				}
			}
			break;
		
		case 3:  // TIME_WAIT
			// ���ﰴ��TCP�ķ�������Ҫ�ȴ�2MSL�������Ȳ�д�ˣ�ֱ���˳�
			state = 4;
			break;

		case 4:  // �Ĵλ��ֽ���״̬
			printTime();
			cout << "�Ĵλ��ֽ�����ȷ���ѶϿ����ӣ�Bye-bye..." << endl;
			flag = 0;
			break;
		}
	}
}

void sendPacket(Packet* sendPkt) {
	Packet* recvPkt = new Packet;

	// ���һ���ļ��ĸ�������
	cout << "Send Message " << sendPkt->len << " Bytes!";
	cout << " Flag:" << sendPkt->FLAG << " SEQ:" << sendPkt->seq;
	cout << " checksum:" << sendPkt->checksum << endl;
	err = sendto(socketClient, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));
	if (err == SOCKET_ERROR) {
		cout << "sendto failed with error: " << WSAGetLastError() << endl;
	}
	clock_t start = clock();  // ��¼����ʱ�䣬��ʱ�ش�

	// �ȴ�����ACK��Ϣ����֤acknowledge number
	while (true) {
		while (recvfrom(socketClient, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &fromLen) <= 0) {
			if (clock() - start > MAX_TIME) {
				printTime();
				cout << "TIME OUT! Resend this data packet" << endl;

				err = sendto(socketClient, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));
				if (err == SOCKET_ERROR) {
					cout << "sendto failed with error: " << WSAGetLastError() << endl;
				}
				start = clock(); // ���迪ʼʱ��
			}
		}
		// cout << "Recieve Message " << recvPkt->len << " Bytes!";
		// cout << " Flag:" << recvPkt->FLAG << " SEQ:" << recvPkt->seq << " ACK:" << recvPkt->ack;
		// cout << " checksum:" << recvPkt->checksum << endl;

		// ��������Ҫͬʱ���㣬�������ʱ�ж���packet��seq
		if ((recvPkt->FLAG & 0x4) && (recvPkt->ack == sendPkt->seq)) {
			sendSeq++;
			break;
		}
	}
	
}

void readFile() {
	ifstream f(filePath, ifstream::in | ios::binary);  // �Զ����Ʒ�ʽ���ļ�
	if (!f.is_open()) {
		cout << "�ļ��޷��򿪣�" << endl;
		return;
	}
	f.seekg(0, std::ios_base::end);  // ���ļ���ָ�붨λ������ĩβ
	fileSize = f.tellg();
	packetNum = fileSize / PACKET_LENGTH + 1;
	cout << "�ļ���СΪ " << fileSize << " Bytes���ܹ�Ҫ���� " << packetNum << " �����ݰ�" << endl;
	f.seekg(0, std::ios_base::beg);  // ���ļ���ָ�����¶�λ�����Ŀ�ʼ
	fileBuffer = new char[fileSize];
	f.read(fileBuffer, fileSize);
	f.close();
}

void sendFile() {
	sendSeq = 0;
	clock_t start = clock();
	
	// �ȷ�һ����¼�ļ��������ݰ���������HEAD��־λΪ1����ʾ��ʼ�ļ�����
	Packet* headPkt = new Packet;
	// headPkt->fillData(filePath, strlen(filePath));
	headPkt->setHEAD(sendSeq, fileSize, filePath);
	headPkt->checksum = checksum((uint32_t*)headPkt);
	sendPacket(headPkt);

	// ��ʼ����װ���ļ������ݰ�
	Packet* sendPkt = new Packet;
	for (int i = 0; i < packetNum; i++) {
		if (i == packetNum - 1) {  // ���һ����
			sendPkt->setTAIL();
			sendPkt->fillData(sendSeq, fileSize - i * PACKET_LENGTH, fileBuffer + i * PACKET_LENGTH);
			sendPkt->checksum = checksum((uint32_t*)sendPkt);
		}
		else {
			sendPkt->fillData(sendSeq, PACKET_LENGTH, fileBuffer + i * PACKET_LENGTH);
			sendPkt->checksum = checksum((uint32_t*)sendPkt);	
		}
		sendPacket(sendPkt);
		Sleep(20);
	}

	clock_t end = clock();
	printTime();
	cout << "�ļ�������ϣ�����ʱ��Ϊ��" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
	cout << "������Ϊ��" << ((float)fileSize) / ((end - start) / CLOCKS_PER_SEC) << " Bytes/s " << endl << endl;
	cout << endl << "**************************************************************************************************" << endl << endl;
}

int main() {
	// ��ʼ���ͻ���
	initSocket();

	// ��������˽�������
	connect();

	// ��ȡ�ļ�
	cout << "��������Ҫ������ļ�����";  
	// C:\Users\new\Desktop\Tree-Hole\readme.md
	// C:\Users\new\Desktop\GRE��ջ���1100���Ѷȷּ��棨�ڶ��棩.pdf
	// C:\Users\new\Desktop\helloword.txt

	filePath = new char[128];
	cin >> filePath;
	readFile();

	// ��ʼ�����ļ�
	sendFile();
	
	// �����Ͽ�����
	disconnect();

	// ������ɺ󣬹ر�socket
	err = closesocket(socketClient);
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