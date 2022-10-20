#include<iostream>
#include<string.h>
#include<string>
#include<map>
#include<queue>
#include <typeinfo>
#include<WinSock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable : 4996)

#define DEFAULT_BUFLEN 500

using namespace std;

queue<string> message_queue; // ���������Ϣ���������Σ���Ҫ�����Ϣ���е���һ����������Ϣת�����߳�
map<SOCKET,string> user_list; // Ŀǰ�����û�����

void appendUser(SOCKET &socket, char a[]) {
    string s = "";
    for (int i = 0; a[i]; i++)s += a[i];
    user_list[socket] = s;
}

DWORD WINAPI handlerRequest(LPVOID lparam)
{
    // Ϊÿһ�����ӵ��˶˿ڵ��û�����һ���߳�
    SOCKET ClientSocket = (SOCKET)lparam;

    char curr_username[20];
    recv(ClientSocket, curr_username, sizeof(curr_username), 0); // �����û���
    appendUser(ClientSocket, curr_username);

    SYSTEMTIME sysTime = { 0 };
    GetLocalTime(&sysTime);
    cout << endl;
    cout << "[" << sysTime.wYear << "-" << sysTime.wMonth << "-" << sysTime.wDay << " ";
    cout << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << "] ";
    cout << curr_username << " �Ѽ�����������     ";
    cout << "Ŀǰ����������" << user_list.size() << "��" << endl;


    // ���û����͵�ǰ�����û�����
    if (user_list.size() > 0) {
        string nameList = "";
        for (auto it : user_list) {
            nameList += it.second;
            nameList += " ";
        }
        char sendList[1000];
        sendList[0] = '0';
        for (int i = 0; i < nameList.length(); i++)sendList[i + 1] = nameList[i];
        sendList[nameList.length()] = '\0';
        send(ClientSocket, sendList, sizeof(sendList), 0);
    }
    else {
        char sendList[1000];
        sendList[0] = '0';
        string msg = "��ǰ�����߳�Ա";
        for (int i = 0; i < msg.length(); i++)sendList[i + 1] = msg[i];
        send(ClientSocket, sendList, sizeof(sendList), 0);
    }


    // ѭ�����ܿͻ�������
    int recvResult;
    int sendResult;
    do {
        char recvBuf[DEFAULT_BUFLEN] = "";
        char sendBuf[DEFAULT_BUFLEN] = "";
        recvResult = recv(ClientSocket, recvBuf, DEFAULT_BUFLEN, 0);
        if (recvResult > 0) {
            SYSTEMTIME logTime = { 0 };
            GetLocalTime(&logTime);
            if (recvBuf[0] == '0') {
                // ��ȡ��ǰ�û��б�
                char message[DEFAULT_BUFLEN];
                for (int i = 1; i < DEFAULT_BUFLEN; i++) {
                    message[i] = recvBuf[i];
                }
                cout << endl;
                cout << "��ǰ�����û�" << "] " << message << endl;
            }
            else if (recvBuf[0] == '1') {
                // Ⱥ��
                char message[DEFAULT_BUFLEN];
                for (int i = 21; i < DEFAULT_BUFLEN; i++) {
                    message[i - 21] = recvBuf[i];
                }

                cout << endl;
                cout << "[" << logTime.wYear << "-" << logTime.wMonth << "-" << logTime.wDay << " ";
                cout << logTime.wHour << ":" << logTime.wMinute << ":" << logTime.wSecond;
                cout << " ���� " << curr_username << " ��Ⱥ����Ϣ] " << message << endl;

                // �������û��ַ���Ϣ
                for (auto it : user_list) {
                    if (it.first != ClientSocket) {
                        sendResult = send(it.first, recvBuf, DEFAULT_BUFLEN, 0);
                        if (sendResult == SOCKET_ERROR)cout << "send failed with error: " << WSAGetLastError() << endl;
                    }
                }
            }
            else if (recvBuf[0] == '2') {
                // ˽��
                char des_user[20], message[DEFAULT_BUFLEN];
                for (int i = 1; i <= 20; i++)des_user[i - 1] = recvBuf[i];
                for (int i = 21; i < DEFAULT_BUFLEN; i++)message[i - 21] = recvBuf[i];
                cout << endl;
                cout << "[" << curr_username << " ˽���� " << des_user << " ����Ϣ��" << "] " << message << endl;
                // ��ָ���û�������Ϣ
                bool success = 0; // ���ͳɹ���
                for (auto it : user_list) {
                    string sdes_user = "";
                    for (int i = 0; des_user[i]; i++)sdes_user += des_user[i];
                    if (it.second == sdes_user) {
                        for (int i = 1; i <= 20; i++)recvBuf[i] = curr_username[i - 1];
                        sendResult = send(it.first, recvBuf, DEFAULT_BUFLEN, 0); // ����
                        if (sendResult == SOCKET_ERROR)cout << "send failed with error: " << WSAGetLastError() << endl;
                        else success = 1;
                        break;
                    }
                }
                if (!success) {
                    string smsg = "����ʧ�ܣ����û������ڻ�δ����";
                    char msg[100];
                    for (int i = 0; i < smsg.length(); i++)msg[i] = smsg[i];
                    msg[smsg.length()] = '\0';
                    for (int i = 1; i <= 20; i++)sendBuf[i] = des_user[i - 1];
                    for (int i = 0; i < 100; i++)sendBuf[i + 21] = msg[i];
                    sendBuf[0] = '9';
                    sendResult = send(ClientSocket, sendBuf, DEFAULT_BUFLEN, 0); // ���ͳ�����Ϣ��ԭ�û�
                    if (sendResult == SOCKET_ERROR)cout << "send failed with error: " << WSAGetLastError() << endl;
                }
            }
        }
    } while (recvResult != SOCKET_ERROR);

    GetLocalTime(&sysTime);
    cout << endl;
    cout << "[" << sysTime.wYear << "-" << sysTime.wMonth << "-" << sysTime.wDay << " ";
    cout << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << "] ";
    cout << curr_username << " �뿪����������" << endl;

    user_list.erase(ClientSocket);
    closesocket(ClientSocket);
    return 0;
}

int main()
{
    WSADATA wsaData; // �����洢��WSAStartup�������ú󷵻ص�Windows Sockets���ݣ�����Winsock.dllִ�е����ݡ�
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // ָ��socket�淶�İ汾
    if (iResult != NO_ERROR) {
        cout << "WSAStartup failed with error: " << iResult << endl;
        return 1;
    }


    // ����һ��������SOCKET
    // �����connect��������´���һ���߳�
    SOCKET listenSocket;
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // ͨ��Э�飺IPv4 InternetЭ��; �׽���ͨ�����ͣ�TCP����;  Э���ض����ͣ�ĳЩЭ��ֻ��һ�����ͣ���Ϊ0
    if (listenSocket == INVALID_SOCKET) {
        cout << "socket failed with error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }


    // ��ȡ����ip
    char ip[20] = { 0 };
    struct hostent* phostinfo = gethostbyname("");
    char* p = inet_ntoa(*((struct in_addr*)(*phostinfo->h_addr_list)));
    strncpy(ip, p, sizeof(ip));
    cout << "��������ipΪ��" << ip << endl;


    // ��bind������IP��ַ�Ͷ˿ں�
    sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "10.130.106.124", &sockAddr.sin_addr.s_addr);
    // inet_pton(AF_INET, ip, &sockAddr.sin_addr.s_addr); // �����ʮ���Ƶ�ip��ַת��Ϊ�������紫�����ֵ��ʽ
    sockAddr.sin_port = htons(920); // �˿ں�
    iResult = bind(listenSocket, (SOCKADDR*)&sockAddr, sizeof(sockAddr)); // bind������һ������Э���ַ����һ���׽��֡���������Э�飬Э���ַ��32λ��IPv4��ַ����128λ��IPv6��ַ��16λ��TCP��UDP�˿ںŵ����
    if (iResult == SOCKET_ERROR) {
        wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }


    // ʹsocket�������״̬������Զ�������Ƿ���
    if (listen(listenSocket, 5) == SOCKET_ERROR) {
        cout << "listen failed with error: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }


    // ����ÿ���µ�����ʹ�ö��̴߳���
    cout << "�ȴ��ͻ�������..." << endl << endl;
    while (1) {
        sockaddr_in clientAddr;
        int len = sizeof(clientAddr);
        SOCKET AcceptSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &len); // ����һ���ض�socket����ȴ������е���������
        if (AcceptSocket == INVALID_SOCKET) {
            cout << "accept failed with error: " << WSAGetLastError() << endl;
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }
        else {
            HANDLE hThread = CreateThread(NULL, 0, handlerRequest, (LPVOID)AcceptSocket, 0, NULL); // �����̣߳����Ҵ�����clientͨѶ���׽���
            CloseHandle(hThread); // �رն��̵߳�����
        }
    }
    

    // �رշ����SOCKET
    iResult = closesocket(listenSocket);
    if (iResult == SOCKET_ERROR) {
        cout << "close failed with error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    WSACleanup();
    return 0;
}
