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
	//配置参数
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
		select()机制中提供的fd_set的数据结构，实际上是long类型的数组，
		每一个数组元素都能与一打开的文件句柄（不管是socket句柄，还是其他文件或命名管道或设备句柄）建立联系，建立联系的工作由程序员完成.
		当调用select()时，由内核根据IO状态修改fd_set的内容，由此来通知执行了select()的进程哪个socket或文件句柄发生了可读或可写事件。
	*/
	fd_set rfds;				//用于检查socket是否有数据到来的的文件描述符，用于socket非阻塞模式下等待网络事件通知（有数据到来）
	fd_set wfds;				//用于检查socket是否可以发送的文件描述符，用于socket非阻塞模式下等待网络事件通知（可以发送数据）
	bool first_connetion = true;
    //初始化Winsock
	int nRc = WSAStartup(0x0202,&wsaData);

	if(nRc){
		printf("Winsock startup failed with error!\n");
	}

	if(wsaData.wVersion != 0x0202){
		printf("Winsock version is not correct!\n");
	}

	printf("Winsock startup Ok!\n");
    

	//监听socket
	SOCKET srvSocket;	

	//服务器地址和客户端地址
	sockaddr_in addr,clientAddr;

	//会话socket，负责和client进程通信
	SOCKET sessionSocket;

	//ip地址长度
	int addrLen;

	//创建监听socket, 第二个参数指定使用TCP
	srvSocket = socket(AF_INET,SOCK_STREAM,0);
	if(srvSocket != INVALID_SOCKET)
		printf("Socket create Ok!\n");


	//设置服务器的端口和地址
	addr.sin_family = AF_INET;//AF_INET表示使用 IPv4 地址。
	addr.sin_port = htons(config.port);
	addr.sin_addr.S_un.S_addr = inet_addr(config.address);
	/*  
		struct in_addr checkaddr;
		checkaddr.s_addr = addr.sin_addr.S_un.S_addr;
		cout << "转换回字符串: " << inet_ntoa(checkaddr) << endl;
	*/
	

	//binding
	int rtn = bind(srvSocket,(LPSOCKADDR)&addr,sizeof(addr));//服务器 socket 绑定到指定的地址和端口。
	if(rtn != SOCKET_ERROR)
		printf("Socket bind Ok!\n");
	else {
		printf("Socket bind failed\n");
		cout << WSAGetLastError();
	}

	//监听
	rtn = listen(srvSocket,5);
	if(rtn != SOCKET_ERROR)
		printf("Socket listen Ok!\n");

	clientAddr.sin_family =AF_INET;
	addrLen = sizeof(clientAddr);

	//设置接收缓冲区
	char recvBuf[4096];

	u_long blockMode = 1;//将srvSock设为非阻塞模式以监听客户连接请求

	//调用ioctlsocket，将srvSocket改为非阻塞模式，改成反复检查fd_set元素的状态，看每个元素对应的句柄是否可读或可写
	if ((rtn = ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO：允许或禁止套接口s的非阻塞模式。
		cout << "ioctlsocket() failed with error!\n";
		return;
	}
	cout << "ioctlsocket() for server socket ok!	\nWaiting for client connection and data\n";
	cout << "---------------------------------------------------------------------------------"<<endl;
	while(true){
		//清空rfds和wfds数组
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		//将srvSocket加入rfds数组
		//即：当客户端连接请求到来时，rfds数组里srvSocket对应的的状态为可读
		//因此这条语句的作用就是：设置等待客户连接请求
		FD_SET(srvSocket, &rfds);

		//如果first_connetion为true，sessionSocket还没有产生
		if (!first_connetion) {
			//将sessionSocket加入rfds数组和wfds数组
			//即：当客户端发送数据过来时，rfds数组里sessionSocket的对应的状态为可读；当可以发送数据到客户端时，wfds数组里sessionSocket的对应的状态为可写
			//因此下面二条语句的作用就是：
			//设置等待会话SOKCET可接受数据或可发送数据
			if (sessionSocket != INVALID_SOCKET) { //如果sessionSocket是有效的
				FD_SET(sessionSocket, &rfds);
				FD_SET(sessionSocket, &wfds);
			}
			
		}
		
		/*
			select工作原理：传入要监听的文件描述符集合（可读、可写，有异常）开始监听，select处于阻塞状态。
			当有可读写事件发生或设置的等待时间timeout到了就会返回，返回之前自动去除集合中无事件发生的文件描述符，返回时传出有事件发生的文件描述符集合。
			但select传出的集合并没有告诉用户集合中包括哪几个就绪的文件描述符，需要用户后续进行遍历操作(通过FD_ISSET检查每个句柄的状态)。
		*/
		//开始等待，等待rfds里是否有输入事件，wfds里是否有可写事件
		//The select function returns the total number of socket handles that are ready and contained in the fd_set structure
		//返回总共可以读或写的句柄个数
		int nTotal = select(0, &rfds, &wfds, NULL, NULL);

		//如果srvSock收到连接请求，接受客户连接请求
		if (FD_ISSET(srvSocket, &rfds)) {
			nTotal--;   //因为客户端请求到来也算可读事件，因此-1，剩下的就是真正有可读事件的句柄个数（即有多少个socket收到了数据）

			//产生会话SOCKET
			sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
			if (sessionSocket != INVALID_SOCKET) {
				printf("Socket listen one client request!\n");
				printf("Client IP：%s\n", inet_ntoa(clientAddr.sin_addr));
				printf("Client Port：%u\n", htons(clientAddr.sin_port));
			}
			else {
				cout << "Socket fail to connect with client:" << WSAGetLastError()<< endl;
			}
				
			//把会话SOCKET设为非阻塞模式
			if ((rtn = ioctlsocket(sessionSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO：允许或禁止套接口s的非阻塞模式。
				cout << "ioctlsocket() failed with error!\n";
				return;
			}
			cout << "ioctlsocket() for session socket ok!	\nWaiting for client connection and data\n";
			cout << "---------------------------------------------------------------------------------" << endl;
			//设置等待会话SOKCET可接受数据或可发送数据
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
				//获得请求文件路径
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
			else { //否则是收到了客户端断开连接的请求，也算可读事件。但rtn = 0
				printf("Client leaving ...\n");
				closesocket(sessionSocket);  //既然client离开了，就关闭sessionSocket
				nTotal--;	//因为客户端离开也属于可读事件，所以需要-1
				sessionSocket = INVALID_SOCKET; //把sessionSocket设为INVALID_SOCKET
			}
		}
		
		/*
		//检查会话SOCKET是否有数据到来或者是否可以发送数据
		if (nTotal > 0) { 
			//如果还有有可读事件，说明是会话socket有数据到来，则接受客户的数据
			
		}
		*/	
	}
}
void Configuration(ServerConfig& config) {
	//读取配置文件
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
			printf("head fail to send！\n");
			return;
		}
		if (send(sessionSocket, sendContentType, strlen(sendContentType), 0) == SOCKET_ERROR) {
			printf("head fail to send！\n");
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
			printf("head fail to send！\n");
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
	//使用 fseek 将文件指针移动到文件末尾，接着使用 ftell 获取当前文件指针的位置，这个位置就是文件的字节数。
	char* p = (char*)malloc(dataLen + 1);
	fseek(fp, 0L, SEEK_SET);
	fread(p, dataLen, 1, fp);
	//将文件指针移动到文件开头，然后使用 fread 读取文件的内容到分配的内存中。
	if (send(sessionsocket, p, dataLen, 0) == SOCKET_ERROR) {
		printf("fail to send data\n");
	}
	return;
}

