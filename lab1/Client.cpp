#include<iostream>
#include<string>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<Windows.h>
#include<thread>
#pragma comment(lib,"ws2_32.lib")

#define DEFAULT_BUFLEN 500

using namespace std;

string quit_string = "quit"; // �˳��ź�
char user_name[20]; // �û���

DWORD WINAPI recv(LPVOID lparam_socket) {
    int recvResult;
    SOCKET* recvSocket = (SOCKET*)lparam_socket; // һ��Ҫʹ��ָ�룬��ΪҪָ��connect socket��λ��

    while (1) {
        char recvBuf[DEFAULT_BUFLEN] = "";
        recvResult = recv(*recvSocket, recvBuf, DEFAULT_BUFLEN, 0);
        if (recvResult > 0) {
            SYSTEMTIME systime = { 0 };
            GetLocalTime(&systime);
            cout << endl << endl;
            cout << "#################################################################" << endl;
            cout << endl;
            cout << "[" << systime.wYear << "-" << systime.wMonth << "-" << systime.wDay << " ";
            cout << systime.wHour << ":" << systime.wMinute << ":" << systime.wSecond; 
            if (recvBuf[0] == '0') {
                // ��ȡ��ǰ�û��б�
                char message[DEFAULT_BUFLEN];
                for (int i = 1; i < DEFAULT_BUFLEN; i++) {
                    message[i - 1] = recvBuf[i];
                }
                cout << " ��ǰ�����û�" << "] " << message << endl;
                cout << "#################################################################" << endl;
            }
            else if (recvBuf[0] == '1') {
                // Ⱥ��
                char user_name[20], message[DEFAULT_BUFLEN];
                for (int i = 1; i <= 20; i++)user_name[i - 1] = recvBuf[i];
                for (int i = 21; i < DEFAULT_BUFLEN; i++)message[i - 21] = recvBuf[i];
                cout << " �յ����� " << user_name << " Ⱥ������Ϣ��" << "] " << message << endl;
                cout << "#################################################################" << endl;
            }
            else if (recvBuf[0] == '2') {
                // ˽��
                char des_user[20], message[DEFAULT_BUFLEN];
                for (int i = 1; i <= 20; i++)des_user[i - 1] = recvBuf[i];
                for (int i = 21; i < DEFAULT_BUFLEN; i++)message[i - 21] = recvBuf[i];
                cout << " �յ����� " << des_user << " ˽������Ϣ" << "] " << message << endl;
                cout << "#################################################################" << endl;
            }
            else if (recvBuf[0] == '9') {
                // �յ�˽��������Ϣ
                char des_user[20], message[DEFAULT_BUFLEN];
                for (int i = 1; i <= 20; i++)des_user[i - 1] = recvBuf[i];
                for (int i = 21; i < DEFAULT_BUFLEN; i++)message[i - 21] = recvBuf[i];
                cout << " ���͸��û� " << des_user << " ��˽����Ϣ] " << message << endl;
                cout << "#################################################################" << endl;
            }
        }
        else {
            closesocket(*recvSocket);
            return 1;
        }
    }
}

DWORD WINAPI send(LPVOID lparam_socket) {

    // ������Ϣֱ��quit�˳�����
    int sendResult;
    SOCKET* sendSocket = (SOCKET*)lparam_socket;

    while (1)
    {
        // ������Ϣ
        char sendBuf[DEFAULT_BUFLEN] = "";
        char temp[DEFAULT_BUFLEN] = "";
        cout << endl << endl;
        cout << "-----------------------------------------------------------------" << endl;
        cout << "�����������Ϣ��";
        cin >> temp;
        // cin.getline(temp, DEFAULT_BUFLEN); // ��֤��������ո�getline�������ú����Ի��з�Ϊ����
        if (temp == quit_string) {
            closesocket(*sendSocket);
            cout << endl << "�����˳�" << endl;
            return 1;
        }
        while (1) {
            cout << "��Ⱥ��������1����˽��������2��";
            string flag;
            cin >> flag;
            if (flag == "1") {
                sendBuf[0] = '1';
                for (int i = 1; i <= 20; i++)sendBuf[i] = user_name[i - 1];
                break;
            }
            else if (flag == "2") {
                sendBuf[0] = '2';
                char des_user[20];
                cout << "������Ҫ˽�����û�����";
                cin >> des_user;
                for (int i = 1; i <= 20; i++)sendBuf[i] = des_user[i - 1];
                break;
            }
            else {
                cout << "Ŀǰ��֧�ִ˹��ܣ�����������" << endl;
            }
        }
        for (int i = 21; i < DEFAULT_BUFLEN; i++) {
            sendBuf[i] = temp[i-21];
        }
        sendResult = send(*sendSocket, sendBuf, DEFAULT_BUFLEN, 0);
        if (sendResult == SOCKET_ERROR) {
            cout << "send failed with error: " << WSAGetLastError() << endl;
            closesocket(*sendSocket);
            WSACleanup();
            return 1;
        }
        else {
            SYSTEMTIME systime = { 0 };
            GetLocalTime(&systime);
            cout << endl << systime.wYear << "-" << systime.wMonth << "-" << systime.wDay << " ";
            cout << systime.wHour << ":" << systime.wMinute << ":" << systime.wSecond;
            cout << " ��Ϣ�ѳɹ�����" << endl;
            cout << "-----------------------------------------------------------------" << endl;
        }
        Sleep(1000); // ͣ��1���ٽ�������
    }
}


int main() {
    int iResult; // ʹ��iResult��ֵ���������������Ƿ�����ɹ�
    WSADATA wsaData;
    SOCKET connectSocket = INVALID_SOCKET;

    int recvBuflen = DEFAULT_BUFLEN;
    int sendBuflen = DEFAULT_BUFLEN;


    // ��ʼ�� Winsock,�����Ϣ��ϸ����
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        cout << "WSAStartup failed with error: " << iResult << endl;
        return 1;
    }


    // �ͻ��˴���SOCKET�ڴ������ӵ������
    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket == INVALID_SOCKET) {
        cout << "socket failed with error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }


    // ���������ip
    cout << "������Ҫ���ӵķ�����ip��ַ��";
    char ip[20];
    cin >> ip;
    // �����û���
    cout << "����������û�����";
    cin >> user_name;


    // ����sockaddr_in�ṹ����ת����SOCKADDR�Ľṹ
    // Ҫ���ӵķ���˵�IP��ַ���˿ں�
    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    inet_pton(AF_INET, "10.130.106.124", &clientService.sin_addr.s_addr);
    // inet_pton(AF_INET, ip, &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(920);


    // Connect���ӵ������
    iResult = connect(connectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR) {
        cout << "connect failed with error: " << WSAGetLastError() << endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }
    // ��������˷����û���
    send(connectSocket, user_name, sizeof(user_name), 0);
    char nameList[1000], msg[1000];
    recv(connectSocket, nameList, sizeof(nameList), 0);
    for (int i = 1; i < 1000; i++)msg[i - 1] = nameList[i];
    

    // ��ӡ��������ı�־
    cout << endl;
    cout << "*****************************************************************" << endl;
    cout << "*                                                               *" << endl;
    cout << "  ��ӭ " << user_name << " ������������                          " << endl;
    cout << "*                                                               *" << endl;
    cout << "*                                                               *" << endl;
    cout << "*  ��ǰ�����û�                                                 *" << endl;    
    cout << "*                                                               *" << endl;
    cout << "  " << msg << "                                                  " << endl;
    cout << "*                                                               *" << endl;
    cout << "*                                                               *" << endl;
    cout << "*                                        ����quit���˳��������� *" << endl;
    cout << "*                                                               *" << endl;
    cout << "*****************************************************************" << endl;
    cout << endl << endl;

    // ���������̣߳�һ�������̣߳�һ�������߳�
    HANDLE hThread[2];
    hThread[0] = CreateThread(NULL, 0, recv, (LPVOID)&connectSocket, 0, NULL);
    hThread[1] = CreateThread(NULL, 0, send, (LPVOID)&connectSocket, 0, NULL);

    WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
    CloseHandle(hThread[0]);
    CloseHandle(hThread[1]);

    // �ر�socket
    iResult = closesocket(connectSocket);
    WSACleanup();
    return 0;
}
