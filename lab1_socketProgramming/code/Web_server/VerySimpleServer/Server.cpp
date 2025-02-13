#pragma once
#include "winsock2.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;

#pragma comment(lib,"ws2_32.lib")
struct ServerConfig
{
	//���ò���
	char address[50];
	int port;
	char documentRoot[100];
};
void Configuration(ServerConfig& config);
void sendHead(const char* filename, const char* suffixname, SOCKET s);
void sendData(const char* filename, SOCKET s);

void main(){
	//configuration
	ServerConfig config;
	Configuration(config);
	/*
		cout << "Configuration:" << endl;
		cout << "	"<<config.address << endl;
		cout << "	" << config.port << endl;
		cout << "	" << config.documentRoot << endl;
	*/
	
	WSADATA wsaData;
	/*
		select()�������ṩ��fd_set�����ݽṹ��ʵ������long���͵����飬
		ÿһ������Ԫ�ض�����һ�򿪵��ļ������������socket��������������ļ��������ܵ����豸�����������ϵ��������ϵ�Ĺ����ɳ���Ա���.
		������select()ʱ�����ں˸���IO״̬�޸�fd_set�����ݣ��ɴ���ִ֪ͨ����select()�Ľ����ĸ�socket���ļ���������˿ɶ����д�¼���
	*/
	fd_set rfds;				//���ڼ��socket�Ƿ������ݵ����ĵ��ļ�������������socket������ģʽ�µȴ������¼�֪ͨ�������ݵ�����
	fd_set wfds;				//���ڼ��socket�Ƿ���Է��͵��ļ�������������socket������ģʽ�µȴ������¼�֪ͨ�����Է������ݣ�
	bool first_connetion = true;
    //��ʼ��Winsock
	int nRc = WSAStartup(0x0202,&wsaData);

	if(nRc){
		printf("Winsock startup failed with error!\n");
	}

	if(wsaData.wVersion != 0x0202){
		printf("Winsock version is not correct!\n");
	}

	printf("Winsock startup Ok!\n");
    

	//����socket
	SOCKET srvSocket;	

	//��������ַ�Ϳͻ��˵�ַ
	sockaddr_in addr,clientAddr;

	//�Ựsocket�������client����ͨ��
	SOCKET sessionSocket;

	//ip��ַ����
	int addrLen;

	//��������socket, �ڶ�������ָ��ʹ��TCP
	srvSocket = socket(AF_INET,SOCK_STREAM,0);
	if(srvSocket != INVALID_SOCKET)
		printf("Socket create Ok!\n");


	//���÷������Ķ˿ں͵�ַ
	addr.sin_family = AF_INET;//AF_INET��ʾʹ�� IPv4 ��ַ��
	addr.sin_port = htons(config.port);
	addr.sin_addr.S_un.S_addr = inet_addr(config.address);
	/*  
		struct in_addr checkaddr;
		checkaddr.s_addr = addr.sin_addr.S_un.S_addr;
		cout << "ת�����ַ���: " << inet_ntoa(checkaddr) << endl;
	*/
	

	//binding
	int rtn = bind(srvSocket,(LPSOCKADDR)&addr,sizeof(addr));//������ socket �󶨵�ָ���ĵ�ַ�Ͷ˿ڡ�
	if(rtn != SOCKET_ERROR)
		printf("Socket bind Ok!\n");
	else {
		printf("Socket bind failed\n");
		cout << WSAGetLastError();
	}

	//����
	rtn = listen(srvSocket,5);
	if(rtn != SOCKET_ERROR)
		printf("Socket listen Ok!\n");

	clientAddr.sin_family =AF_INET;
	addrLen = sizeof(clientAddr);

	//���ý��ջ�����
	char recvBuf[4096];

	u_long blockMode = 1;//��srvSock��Ϊ������ģʽ�Լ����ͻ���������

	//����ioctlsocket����srvSocket��Ϊ������ģʽ���ĳɷ������fd_setԪ�ص�״̬����ÿ��Ԫ�ض�Ӧ�ľ���Ƿ�ɶ����д
	if ((rtn = ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO��������ֹ�׽ӿ�s�ķ�����ģʽ��
		cout << "ioctlsocket() failed with error!\n";
		return;
	}
	cout << "ioctlsocket() for server socket ok!	\nWaiting for client connection and data\n";
	cout << "---------------------------------------------------------------------------------"<<endl;
	while(true){
		//���rfds��wfds����
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		//��srvSocket����rfds����
		//�������ͻ�������������ʱ��rfds������srvSocket��Ӧ�ĵ�״̬Ϊ�ɶ�
		//��������������þ��ǣ����õȴ��ͻ���������
		FD_SET(srvSocket, &rfds);

		//���first_connetionΪtrue��sessionSocket��û�в���
		if (!first_connetion) {
			//��sessionSocket����rfds�����wfds����
			//�������ͻ��˷������ݹ���ʱ��rfds������sessionSocket�Ķ�Ӧ��״̬Ϊ�ɶ��������Է������ݵ��ͻ���ʱ��wfds������sessionSocket�Ķ�Ӧ��״̬Ϊ��д
			//�����������������þ��ǣ�
			//���õȴ��ỰSOKCET�ɽ������ݻ�ɷ�������
			if (sessionSocket != INVALID_SOCKET) { //���sessionSocket����Ч��
				FD_SET(sessionSocket, &rfds);
				FD_SET(sessionSocket, &wfds);
			}
			
		}
		
		/*
			select����ԭ������Ҫ�������ļ����������ϣ��ɶ�����д�����쳣����ʼ������select��������״̬��
			���пɶ�д�¼����������õĵȴ�ʱ��timeout���˾ͻ᷵�أ�����֮ǰ�Զ�ȥ�����������¼��������ļ�������������ʱ�������¼��������ļ����������ϡ�
			��select�����ļ��ϲ�û�и����û������а����ļ����������ļ�����������Ҫ�û��������б�������(ͨ��FD_ISSET���ÿ�������״̬)��
		*/
		//��ʼ�ȴ����ȴ�rfds���Ƿ��������¼���wfds���Ƿ��п�д�¼�
		//The select function returns the total number of socket handles that are ready and contained in the fd_set structure
		//�����ܹ����Զ���д�ľ������
		int nTotal = select(0, &rfds, &wfds, NULL, NULL);

		//���srvSock�յ��������󣬽��ܿͻ���������
		if (FD_ISSET(srvSocket, &rfds)) {
			nTotal--;   //��Ϊ�ͻ���������Ҳ��ɶ��¼������-1��ʣ�µľ��������пɶ��¼��ľ�����������ж��ٸ�socket�յ������ݣ�

			//�����ỰSOCKET
			sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
			if (sessionSocket != INVALID_SOCKET) {
				printf("Socket listen one client request!\n");
				printf("Client IP��%s\n", inet_ntoa(clientAddr.sin_addr));
				printf("Client Port��%u\n", htons(clientAddr.sin_port));
			}
			else {
				cout << "Socket fail to connect with client:" << WSAGetLastError()<< endl;
			}
				
			//�ѻỰSOCKET��Ϊ������ģʽ
			if ((rtn = ioctlsocket(sessionSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO��������ֹ�׽ӿ�s�ķ�����ģʽ��
				cout << "ioctlsocket() failed with error!\n";
				return;
			}
			cout << "ioctlsocket() for session socket ok!	\nWaiting for client connection and data\n";
			cout << "---------------------------------------------------------------------------------" << endl;
			//���õȴ��ỰSOKCET�ɽ������ݻ�ɷ�������
			FD_SET(sessionSocket, &rfds);
			FD_SET(sessionSocket, &wfds);

			first_connetion = false;

		}
		if (FD_ISSET(sessionSocket, &rfds)) {
			//receiving data from client
			memset(recvBuf, '\0', 4096);
			rtn = recv(sessionSocket, recvBuf, 2048, 0);
			if (rtn > 0) {
				printf("Received %d bytes from client:\n%s", rtn, recvBuf);
				cout << "---------------------------------------------------------------------------------" << endl;
				//��������ļ�·��
				string request(recvBuf);
				string requestedFile = request.substr(request.find(" ") + 1);
				requestedFile = requestedFile.substr(0, requestedFile.find(" "));

				string suffixname = request.substr(request.find(".") + 1);
				suffixname = suffixname.substr(0, suffixname.find(" "));
				string filePath = config.documentRoot + requestedFile;
				cout << "full filePath:" << filePath << endl;
				cout << "suffixname:" << suffixname << endl;
				sendHead(filePath.c_str(), suffixname.c_str(), sessionSocket);
				sendData(filePath.c_str(), sessionSocket);
				cout << "---------------------------------------------------------------------------------" << endl;
			}
			else { //�������յ��˿ͻ��˶Ͽ����ӵ�����Ҳ��ɶ��¼�����rtn = 0
				printf("Client leaving ...\n");
				closesocket(sessionSocket);  //��Ȼclient�뿪�ˣ��͹ر�sessionSocket
				nTotal--;	//��Ϊ�ͻ����뿪Ҳ���ڿɶ��¼���������Ҫ-1
				sessionSocket = INVALID_SOCKET; //��sessionSocket��ΪINVALID_SOCKET
			}
		}
		
		/*
		//���ỰSOCKET�Ƿ������ݵ��������Ƿ���Է�������
		if (nTotal > 0) { 
			//��������пɶ��¼���˵���ǻỰsocket�����ݵ���������ܿͻ�������
			
		}
		*/	
	}
}
void Configuration(ServerConfig& config) {
	//��ȡ�����ļ�
	FILE* config_file = fopen("../src/config.txt", "r");
	if (!config_file) {
		perror("Could not open config file");
	}
	fscanf(config_file, "%s %d %s", &config.address, &config.port, config.documentRoot);
	fclose(config_file);
}
void sendHead(const char* filename, const char* suffixname, SOCKET sessionSocket) {
	char contentType[20] = "";
	if (strcmp(suffixname, "html") == 0) strcpy(contentType, "text/html");
	if (strcmp(suffixname, "jpg") == 0) strcpy(contentType, "image/jpg");
	if (strcmp(suffixname, "jpeg") == 0) strcpy(contentType, "image/jpeg");
	if (strcmp(suffixname, "png") == 0) strcpy(contentType, "iamge/png");
	if (strcmp(suffixname, "gif") == 0) strcpy(contentType, "image/gif");

	char sendContentType[40] = "Content-Type: ";
	strcat(sendContentType, contentType);
	strcat(sendContentType, "\r\n");

	const char* found = "HTTP/1.1 200 OK\r\n"; //200 OK
	const char* nofound = "HTTP/1.1 404 Not Found\r\n"; //404 Not Found
	const char* forbidden = "HTTP/1.1 403 FORBIDDEN\r\n"; //403 Forbidden
	//e.g. for 403 Forbidden
	//cout << strcmp(filename, "C:\\Users\\ZPD\\Desktop\\Networking\\Web_server/private.jpeg");
	if (strcmp(filename, "../src/private.jpeg") == 0) {
		if (send(sessionSocket, forbidden, strlen(forbidden), 0) == SOCKET_ERROR) {
			printf("head fail to send��\n");
			return;
		}
		if (send(sessionSocket, sendContentType, strlen(sendContentType), 0) == SOCKET_ERROR) {
			printf("head fail to send��\n");
			return;
		}
		printf("403 Forbidden!\n");
		return;
	}
	
	FILE* fp = fopen(filename, "rb");
	if (fp == NULL) {                    
		if (send(sessionSocket, nofound, strlen(nofound), 0) == SOCKET_ERROR) {
			printf("head fail to send\n");
			return;
		}
		strcpy(sendContentType, "Content-Type: text/html\r\n");
		printf("404 NO FOUND\n");
	}
	else {                                  //200 OK
		if (send(sessionSocket, found, strlen(found), 0) == SOCKET_ERROR) {
			printf("head fail to send��\n");
			return;
		}
		printf("200 OK\n");
	}
	if (contentType) {
		if (send(sessionSocket, sendContentType, strlen(sendContentType), 0) == SOCKET_ERROR) {
			printf("contentType fail to send\n");
			return;
		}
	}
	if (send(sessionSocket, "\r\n", 2, 0) == SOCKET_ERROR) {
		printf("fail to send\n");
		return;
	}
	return;
}
/*
void sendData(const char* filename, SOCKET sessionSocket) {
	string content = getFileContent(filename);
	if(send(sessionSocket, content.c_str(), content.size(), 0)){
		cout << "Data fail to send" << endl;
		return;
	}

}
*/
void sendData(const char* filename, SOCKET sessionsocket) {
	FILE* fp = fopen(filename, "rb");
	if (fp == NULL) {
		fp = fopen("../src/nofound.html", "rb");
	}
	fseek(fp, 0L, SEEK_END);
	int dataLen = ftell(fp);  
	//ʹ�� fseek ���ļ�ָ���ƶ����ļ�ĩβ������ʹ�� ftell ��ȡ��ǰ�ļ�ָ���λ�ã����λ�þ����ļ����ֽ�����
	char* p = (char*)malloc(dataLen + 1);
	fseek(fp, 0L, SEEK_SET);
	fread(p, dataLen, 1, fp);
	//���ļ�ָ���ƶ����ļ���ͷ��Ȼ��ʹ�� fread ��ȡ�ļ������ݵ�������ڴ��С�
	if (send(sessionsocket, p, dataLen, 0) == SOCKET_ERROR) {
		printf("fail to send data\n");
	}
	return;
}

