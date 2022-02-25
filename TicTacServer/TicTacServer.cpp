// TicTacServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <WinSock2.h>
#include <process.h>

#pragma comment(lib,"ws2_32.lib")

#define MAX_PACKETLEN 1024	//패킷 길이
#define PORT 3500	//포트번호
#define MAX_CLNT 2

unsigned WINAPI t_func(void* data);//클라이언트 처리 스레드 함수
void SendMsg(char* msg, int len);//메시지 보내는 함수
void SendInit(char* msg, int len);//초기화 메세지 보내는 함수

int clientCount = 0;	//현제 입장한 클라이언트 수
int drawCount = 0;	//무승부인지 아닌지 확인하는 카운트
SOCKET clientSocks[MAX_CLNT];//클라이언트 소켓 보관용 배열
HANDLE hMutex;//뮤텍스

int map[3][3] = { 0, }; // [가로][세로]

struct Packet {
    int setOrder;	//현제 보내는 데이터의 타입설정
    int setToken;	//차례보내기
    int setResult;	//결과보내기
    int _x;
    int _y;
};

void SendMsg(char* msg, int len) { //메시지를 모든 클라이언트에게 보낸다.
    int i;
    WaitForSingleObject(hMutex, INFINITE);//뮤텍스 실행
    for (i = 0; i < clientCount; i++)
        send(clientSocks[i], msg, len, 0);
    ReleaseMutex(hMutex);//뮤텍스 중지
}

void SendInit(char* msg, int len, int num) { //num에있는 소켓과 연결된 클라이언트에 메세지를 전달한다
		send(clientSocks[num], msg, len, 0);
}

void InitGame() {	//틱택톡 판을 0으로 초기화하기
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			map[i][j] = 0;
		}
	}
}

unsigned WINAPI t_func(void* data) {	//받은 메세지 처리
	SOCKET sockfd = *((SOCKET*)data);
	int strLen;
	struct sockaddr_in sock_addr;
	char szReceiveBuffer[MAX_PACKETLEN];
	Packet tempPacket;	//임시 패킷
	Packet* packet;	//데이터를 받을 패킷
	packet = new Packet();

	while ((strLen = recv(sockfd, szReceiveBuffer, sizeof(szReceiveBuffer) - 1, 0)) != 0) { //클라이언트로부터 메시지를 받을때까지 기다린다.
		if (strLen == -1) {
			break;
		}
		szReceiveBuffer[strLen] = '\0';
		packet = (Packet*)szReceiveBuffer;

		//임시패킷에 데이터 입력하기
		tempPacket.setOrder = packet->setOrder;
		tempPacket.setResult = packet->setResult;
		tempPacket.setToken = packet->setToken;
		tempPacket._x = packet->_x;
		tempPacket._y = packet->_y;

		//유저가 찍은 좌표에 아무것도 없을 경우
		if (map[tempPacket._x][tempPacket._y] == 0) {
			map[tempPacket._x][tempPacket._y] = tempPacket.setToken;	//해당 좌표에 유저의 순서를 입력한다

			tempPacket.setOrder = 0;	//현제 게임이 진행중이라는 뜻인 0을 보낸다
			tempPacket.setResult = tempPacket.setToken;	//결과에 현제 차례인 유저의 번호를 저장한다

			if (tempPacket.setToken == 1) {	//현제 1의 차례일경우 2로 변경
				tempPacket.setToken = 2;
			}
			else if (tempPacket.setToken == 2) {	//현제 2의 차례일경우 1로 변경
				tempPacket.setToken = 1;
			}
			SendMsg((char*)&tempPacket, sizeof(Packet));
			//해당 메세지를 보내게될경우 클라이언트에서는 setOrder가 0이라서 게임진행중이라고 인식하며 
			//받아온 위치에 결과로보낸 유저번호의 문양이 표시된다

			Packet _tempPack;	//기존의 tempPacket 을 이용하면 예상치못한 값을 보낼수있으므로 새로운 변수를 이용한다
			_tempPack.setOrder = 3;	//setOrder은 게임종료를 의미한다
			
			for (int i = 0; i < 3; i++)	//틱텍톡 가로세로 검사
			{
				if (((map[i][0] == map[i][1]) && (map[i][0] == map[i][2])) && (map[i][0] != 0 && map[i][1] != 0 && map[i][2] != 0)) {
					_tempPack.setToken = map[i][0];
					SendMsg((char*)&_tempPack, sizeof(Packet));//SendMsg에 받은 메시지를 전달한다.
					InitGame();
				}
				else if (((map[0][i] == map[1][i]) && (map[0][i] == map[2][i])) && (map[0][i] != 0 && map[1][i] != 0 && map[2][i] != 0)) {
					_tempPack.setToken = map[0][i];
					SendMsg((char*)&_tempPack, sizeof(Packet));//SendMsg에 받은 메시지를 전달한다.
					InitGame();
				}
			}
			//틱텍톡 대각선 검사
			if (((map[0][0] == map[1][1]) && (map[0][0] == map[2][2])) && (map[0][0] != 0 && map[1][1] != 0 && map[2][2] != 0)) {
				_tempPack.setToken = map[0][0];
				SendMsg((char*)&_tempPack, sizeof(Packet));//SendMsg에 받은 메시지를 전달한다.
				InitGame();
			}
			if (((map[2][0] == map[1][1]) && (map[2][0] == map[0][2])) && (map[2][0] != 0 && map[1][1] != 0 && map[0][2] != 0)) {
				_tempPack.setToken = map[2][0];
				SendMsg((char*)&_tempPack, sizeof(Packet));//SendMsg에 받은 메시지를 전달한다.
				InitGame();
			}
			//무승부카운트 초기화
			drawCount = 0;
			//무승부 검사
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					if (map[i][j] != 0) {
						drawCount++;
					}
					else {
						break;
					}
				}
			}
			if (drawCount == 9) {	//무승부카운트가 9 즉 게임판이 가득 찼을때
				_tempPack.setToken = 3;	//set Order가 3일때 SetToken 3은 무승부를 의미한다
				SendMsg((char*)&_tempPack, sizeof(Packet));//SendMsg에 받은 메시지를 전달한다.
				InitGame();
			}
		}
	}


	WaitForSingleObject(hMutex, INFINITE);//뮤텍스 실행
	for (int i = 0; i < clientCount; i++) {//배열의 갯수만큼
		if (sockfd == clientSocks[i]) {//만약 현재 clientSock값이 배열의 값과 같다면
			for (int j = i; j < MAX_CLNT; j++)
			{
				if (j == MAX_CLNT - 1) {
					clientSocks[i] = 0;	//배열의 마지막에있는 소켓은 0으로 초기화한다
				}
				else {
					clientSocks[i] = clientSocks[i + 1];//앞으로 땡긴다.
				}
			}
			break;
		}
	}
	clientCount--;//클라이언트 개수 하나 감소
	ReleaseMutex(hMutex);//뮤텍스 중지

	//유저와의 연결이끊기면 게임초기화및 연결이끊어짐을 뜻하는 setOreder 4 를 보낸다
	//유저한명이 연결중이고 그 유저가 끊었을시에도 작동하지만 클라이언트는 최대 2명을받고 한명만 연결중이라는건 게임진행중이아니며
	//소켓의 통신이 이미 끊겨진상태라서 별다른 처리없이 작동해도 문제는 생기지않는다
	InitGame();
	Packet _tempPack;
	_tempPack.setOrder = 4;
	SendMsg((char*)&_tempPack, sizeof(Packet));//SendMsg에 받은 메시지를 전달한다.
	printf("DisConnect and Thread Close : %d\n", GetCurrentThreadId());

	closesocket(sockfd);
	return 0;

}

int main()
{
	WSADATA wsaData;
	SOCKET listen_s, client_s;
	struct sockaddr_in server_addr, client_addr;
	HANDLE hTread;
	int addr_len; 
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}
	listen_s = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_s == INVALID_SOCKET) {
		return 1;
	}
	ZeroMemory(&server_addr, sizeof(struct sockaddr_in));

	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(listen_s, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		return 0;
	}
	if (listen(listen_s, 5) == SOCKET_ERROR) {
		return 0;
	}

	while (1) {
		addr_len = sizeof(client_addr);
		client_s = accept(listen_s, (SOCKADDR*)&client_addr, &addr_len);//서버에게 전달된 클라이언트 소켓을 clientSock에 전달

		if (clientCount < MAX_CLNT) {
			WaitForSingleObject(hMutex, INFINITE);//뮤텍스 실행
			clientSocks[clientCount++] = client_s;//클라
			hTread = (HANDLE)_beginthreadex(NULL, 0, t_func, (void*)&client_s, 0, NULL);//HandleClient 쓰레드 실행, clientSock을 매개변수로 전달
			printf("Connected Client IP : %s\n", inet_ntoa(client_addr.sin_addr)); //이언트 소켓배열에 방금 가져온 소켓 주소를 전달
			ReleaseMutex(hMutex);//뮤텍스 중지
		}
		else {
			shutdown(client_s, SD_BOTH);
		}

		if (clientCount == MAX_CLNT) {
			Packet tempPacket;
			tempPacket.setOrder = 1;	//setOrder 1은 초기화를 뜻한다
			tempPacket.setToken = 1;
			SendInit((char*)&tempPacket, sizeof(Packet), 0);//SendMsg에 받은 메시지를 전달한다.
			tempPacket.setOrder = 2;
			SendInit((char*)&tempPacket, sizeof(Packet), 1);//SendMsg에 받은 메시지를 전달한다.
		}
	}

	closesocket(listen_s);
	WSACleanup();
	return 0;
}