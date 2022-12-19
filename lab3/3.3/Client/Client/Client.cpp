#include "Client.h"

#define SERVER_IP "127.0.0.1"  // ��������IP��ַ
#define SERVER_PORT 920
#define DATA_AREA_SIZE 1024
#define BUFFER_SIZE sizeof(Packet)  // ��������С
#define TIME_OUT 0.5 * CLOCKS_PER_SEC  // ��ʱ�ش���������ʱ��Ϊ2s
#define WINDOW_SIZE 16  // �������ڴ�С

SOCKADDR_IN socketAddr;  // ��������ַ
SOCKET socketClient;  // �ͻ����׽���

int err;  // socket������ʾ
int socketAddrLen = sizeof(SOCKADDR);
unsigned int fileSize;  // �ļ���С
unsigned int packetNum;  // �������ݰ�������
unsigned int sendBase;  // ���ڻ���ţ�ָ���ѷ��ͻ�δ��ȷ�ϵ���С�������
unsigned int nextSeqNum;  // ָ����һ�����õ���δ���͵ķ������

char* filePath;  // �ļ�(����)·��
char* fileName;  // �ļ���
char* fileBuffer;  // ��������ʵ���ã�һ���԰��ļ�ȫ�����뻺����������ļ�̫������ڴ泬��
char** selectiveRepeatBuffer;  // ѡ���ش�������

HANDLE hThread[2];  // �շ�ʹ�ò�ͬ�߳�
mutex mutexLock;  // ������

/*
* ��ʱ��SetTimer�ķ���ֵ����Timer��ID��
* �����ھ��ΪNULLʱ��ϵͳ���������ID��������������ĳ��Ԫ�ز�Ϊ0��
* �ʹ���ǰ��Ӧ�ķ�����Timer�ڼ�ʱ
* �������������������Ҫ�����ã�
* (1)����Ԫ�ص�ֵ��Ӧ��ʱ����ID
* (2)��ʶһ��������û�ж�Ӧ�ļ�ʱ���������(������Ԫ�ز�Ϊ0)��˵���÷���δ��ack
*/
int timerID[WINDOW_SIZE];
MSG msg;  // timer��Ϣ

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
	cout << " checksum=" << pkt->checksum << " windowLength=" << pkt->window;
}

void printSendPacketMessage(Packet* pkt) {
	cout << "[Send]";
	printPacketMessage(pkt);
}

void printReceivePacketMessage(Packet* pkt) {
	cout << "[Receive]";
	printPacketMessage(pkt);
	cout << endl;
}

void printWindow() {
	cout << "  ��ǰ���ʹ���: [" << sendBase << ", " << sendBase + WINDOW_SIZE - 1 << "]";
	cout << endl;
}

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
			state = 1;  // ת״̬1
			break;

		case 1:  // �ȴ��������ظ�
			err = recvfrom(socketClient, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &socketAddrLen);
			if (err >= 0) {
				if ((recvPkt->FLAG & 0x2) && (recvPkt->FLAG & 0x4)) {  // SYN=1, ACK=1
					printTime();
					cout << "��ʼ���������֣������������ACK=1�����ݰ�..." << endl;

					// ���������֣������������ACK=1�����ݰ�����֪�������Լ�������������
					sendPkt->setACK();
					sendto(socketClient, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));
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
			err = recvfrom(socketClient, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &socketAddrLen);
			if (err >= 0) {
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
			err = recvfrom(socketClient, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &socketAddrLen);
			if (err >= 0) {
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

void sendPacket(Packet* sendPkt) {  // ��װ��һ�·������ݰ��Ĺ���
	cout << "���͵� " << sendPkt->seq << " �����ݰ�";
	printWindow();
	printSendPacketMessage(sendPkt);  // ���һ���ļ��ĸ�������
	cout << endl;
	err = sendto(socketClient, (char*)sendPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));
	if (err == SOCKET_ERROR) {
		cout << "Send Packet failed with ERROR: " << WSAGetLastError() << endl;
	}
}

void resendPacket(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime) {  // �ش�����
	// cout << endl << "resend" << " Timer ID " << idEvent << endl << endl;
	unsigned int seq = 0;
	for (int i = 0; i < WINDOW_SIZE; i++) {  // �ҵ����ĸ�Timer��ʱ��
		if (timerID[i] == idEvent && timerID[i] != 0) {
			seq = i + sendBase;
			break;
		}
	}
	cout << "�� " << seq << " �����ݰ���Ӧ�ļ�ʱ����ʱ�����·���" << endl;

	Packet* resendPkt = new Packet;
	memcpy(resendPkt, selectiveRepeatBuffer[seq - sendBase], sizeof(Packet));  // �ӻ�����ֱ��ȡ����
	sendPacket(resendPkt);
	printSendPacketMessage(resendPkt);
	cout << endl;
}

void ackHandler(unsigned int ack) { // �������Խ��ն˵�ACK
	// cout << endl << "ack " << ack << endl << endl;
	if (ack >= sendBase && ack < sendBase + WINDOW_SIZE) {  // ���ack��������ڴ�����
		mutexLock.lock();  // ����
		// cout << "KillTimer " << timerID[ack - sendBase] << ack << sendBase << endl << endl;
		KillTimer(NULL, timerID[ack - sendBase]);
		timerID[ack - sendBase] = 0;  // timerID����

		if (ack == sendBase) {  // ���ǡ��=sendBase����ôsendBase�ƶ���������С��ŵ�δȷ�Ϸ��鴦
			for (int i = 0; i < WINDOW_SIZE; i++) {
				if (timerID[i]) break;  // ����һ���м�ʱ����ͣ����(�����ǰtimerID����Ԫ�ز�Ϊ0��˵���м�ʱ����ʹ��)
				sendBase++;  // sendBase����
			}
			int offset = sendBase - ack;
			for (int i = 0; i < WINDOW_SIZE - offset; i++) {
				timerID[i] = timerID[i + offset];  // timerIDҲ��ƽ�ƣ���Ȼ�Բ�����
				timerID[i + offset] = 0;
				memcpy(selectiveRepeatBuffer[i], selectiveRepeatBuffer[i + offset], sizeof(Packet));  // ������ҲҪƽ��
			}
			for (int i = WINDOW_SIZE - offset; i < WINDOW_SIZE; i++) {
				timerID[i] = -1;  // ״̬-1��ʾĿǰ��Ӧλ��û�����ݰ�������, ״̬0��ʾ��Ӧλ�����ݰ��ѷ��Ͳ���ACK
			}
		}
		mutexLock.unlock();  // ����
	}
}

void readFile() {  // ��������ʵ���ã�һ���԰��ļ�ȫ�����뻺����������ļ�̫������ڴ泬��(���Ǽ�ʮ�׼����׻����ܳ��ܵ��˵�)�����ڸĽ��ռ�
	ifstream f(filePath, ifstream::in | ios::binary);  // �Զ����Ʒ�ʽ���ļ�
	if (!f.is_open()) {
		cout << "�ļ��޷��򿪣�" << endl;
		return;
	}
	f.seekg(0, std::ios_base::end);  // ���ļ���ָ�붨λ������ĩβ
	fileSize = f.tellg();
	packetNum = fileSize % DATA_AREA_SIZE ? fileSize / DATA_AREA_SIZE + 1 : fileSize / DATA_AREA_SIZE;  // �����һ��Сbug��֮ǰֱ��+1�Ļ��������1024��������������г�һ����������������Ҫ�ж�һ��
	cout << "�ļ���СΪ " << fileSize << " Bytes���ܹ�Ҫ���� " << packetNum + 1 << " �����ݰ�" << endl << endl;
	f.seekg(0, std::ios_base::beg);  // ���ļ���ָ�����¶�λ�����Ŀ�ʼ
	fileBuffer = new char[fileSize];
	f.read(fileBuffer, fileSize);
	f.close();
}

DWORD WINAPI send(LPVOID lparam) {
	sendBase = 0;
	nextSeqNum = 0;
	selectiveRepeatBuffer = new char* [WINDOW_SIZE];
	for (int i = 0; i < WINDOW_SIZE; i++)selectiveRepeatBuffer[i] = new char[sizeof(Packet)];  // char[32][1048]
	for (int i = 0; i < WINDOW_SIZE; i++)timerID[i] = -1;  // ˫�߳�������ʱ�����һЩͬ�������⣬��������Ϊ-1����һ��
	clock_t start = clock();
	
	// �ȷ�һ����¼�ļ��������ݰ���������HEAD��־λΪ1����ʾ��ʼ�ļ�����
	Packet* headPkt = new Packet;
	printTime();
	cout << "�����ļ�ͷ���ݰ�..." << endl;
	headPkt->setHEAD(0, fileSize, fileName);
	headPkt->checksum = checksum((uint32_t*)headPkt);
	printSendPacketMessage(headPkt);  // ���һ���ļ��ĸ�������
	cout << endl;
	err = sendto(socketClient, (char*)headPkt, BUFFER_SIZE, 0, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR));
	if (err == SOCKET_ERROR) {  // ����û��ʵ���ļ�ͷ��ȱʧ�ش�����
		cout << "Send Packet failed with ERROR: " << WSAGetLastError() << endl;
	}

	// ��ʼ����װ���ļ������ݰ�
	printTime();
	cout << endl << "��ʼ�����ļ����ݰ�..." << endl;
	Packet* sendPkt = new Packet;
	while (sendBase < packetNum) {
		while (nextSeqNum < sendBase + WINDOW_SIZE && nextSeqNum < packetNum) {  // ֻҪ��һ��Ҫ���͵ķ�����Ż��ڴ����ڣ��ͼ�����
			if (nextSeqNum == packetNum - 1) {  // ��������һ����
				sendPkt->setTAIL();
				sendPkt->fillData(nextSeqNum, fileSize - nextSeqNum * DATA_AREA_SIZE, fileBuffer + nextSeqNum * DATA_AREA_SIZE);
				sendPkt->checksum = checksum((uint32_t*)sendPkt);
			}
			else {  // ������1024Bytes���ݰ�
				sendPkt->fillData(nextSeqNum, DATA_AREA_SIZE, fileBuffer + nextSeqNum * DATA_AREA_SIZE);
				sendPkt->checksum = checksum((uint32_t*)sendPkt);
			}
			memcpy(selectiveRepeatBuffer[nextSeqNum - sendBase], sendPkt, sizeof(Packet));  // ���뻺����
			sendPacket(sendPkt);
			mutexLock.lock();
			timerID[nextSeqNum - sendBase] = SetTimer(NULL, 0, TIME_OUT, (TIMERPROC)resendPacket);  // ������ʱ��������nIDEvent=0������Ϊ���ھ����ΪNULL�������������ʲô������ν�������������·��䣬����ֵ����������nIDEvent
			nextSeqNum++;
			mutexLock.unlock();

			Sleep(10);  // ˯��10ms�������֮����һ����ʱ����
		}

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {  // �Բ鿴�ķ�ʽ��ϵͳ�л�ȡ��Ϣ�����Բ�����Ϣ��ϵͳ���Ƴ����Ƿ�������������ϵͳ����Ϣʱ������FALSE������ִ�к������롣
			if (msg.message == WM_TIMER) {  // ��ʱ����Ϣ
				DispatchMessage(&msg);
			}
		}
	}

	clock_t end = clock();
	printTime();
	cout << "�ļ�������ϣ�����ʱ��Ϊ��" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
	cout << "������Ϊ��" << ((float)fileSize) / ((end - start) / CLOCKS_PER_SEC) << " Bytes/s " << endl << endl;
	cout << endl << "**************************************************************************************************" << endl << endl;

	return 0;
}

DWORD WINAPI recv(LPVOID lparam) {
	// �����ǰ���ʹ����Ѿ��ù��ˣ��ͽ������ACK�׶�
	while (sendBase < packetNum) {
		Packet* recvPkt = new Packet;
		err = recvfrom(socketClient, (char*)recvPkt, BUFFER_SIZE, 0, (SOCKADDR*)&(socketAddr), &socketAddrLen);
		if (err > 0) {
			printReceivePacketMessage(recvPkt);
			ackHandler(recvPkt->ack);  // ����ack
		}
	}
	return 0;
}

int main() {
	// ��ʼ���ͻ���
	initSocket();

	// ��������˽�������
	connect();

	// ��ȡ�ļ�
	cout << "��������Ҫ������ļ���(�����ļ����ͺ�׺)��";
	fileName = new char[128];
	cin >> fileName;
	cout << "��������Ҫ������ļ�����·��(����·��)��";
	filePath = new char[128];
	cin >> filePath;
	readFile();
	// C:\\Users\\new\\Desktop\\test\\1.jpg
	// C:\\Users\new\\Desktop\\test\\helloworld.txt

	// �����շ��߳�
	hThread[0] = CreateThread(NULL, 0, send, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, recv, NULL, 0, NULL);
	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);  // �ȴ��շ��߳�ȫ�������Ž������߳�
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);

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

/*
void multithread_Reno(string filename, SOCKET& SendSocket, sockaddr_in& RecvAddr) {
	// ÿ���ļ�����Ԥ�ȳ�ʼ��
	Reno_init();

	// ��ǰԤ��packet_num
	ifstream fin(filename.c_str(), ifstream::binary);
	fin.seekg(0, std::ifstream::end);
	long size = fin.tellg();
	file_size = size;
	fin.seekg(0);
	packet_num = size / DEFAULT_BUFLEN + 1;

	// ���������ṹ��
	Send_params temp_send(filename, SendSocket, RecvAddr);
	Recv_params temp_recv(SendSocket, RecvAddr);

	//----------------------
	// ���������̣߳�һ�������̣߳�һ�������߳�
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, Send, (LPVOID)&temp_send, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, Recv, (LPVOID)&temp_recv, 0, NULL);
	// hThread[2] = CreateThread(NULL, 0, Timer, (LPVOID)&temp_recv, 0, NULL);

	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	// CloseHandle(hThread[2]);
}
*/