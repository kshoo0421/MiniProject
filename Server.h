#ifndef _SERVER_H_
#define _SERVER_H_

#include <arpa/inet.h> // struct sockaddr_in

#include "Macros.h"

typedef struct server {
    int server_sock, cur_mem;
    char IDPW[MAX_CLIENT][20];
    int loginned[MAX_CLIENT];
    int client_socket[MAX_CLIENT];
    int pipes[50][2];

    struct sockaddr_in address;
} Server;

// 서버는 하나만 존재
static Server ChatServer;

/* main.c용 함수. S_로 시작 */
void S_Init();          // 1. 서버 초기화
void S_ServerService(); // 2. 서버 운영
void S_Close();         // 3. 서버 종료

/* Server.c용 함수 선언. s_로 시작*/
/* 1. S_Init() : 서버 초기화 */
void s_MakeDaemon();        // (1) 데몬 만들기
void s_InitMembers();       // (2) 멤버 관련 초기화
void s_InitClientSocket();  // (3) 클라이언트용 소켓 초기화
void s_InitServerSocket();  // (4) 서버 소켓 초기화
void s_InitPipes();         // (5) 파이프들 초기화
void s_SetServerAddress();  // (6) 서버 주소 설정
void s_BindServerSocket();  // (7) 서버 주소 - 서버 소켓 연결
void s_ListenClients();     // (8) 클라이언트 Listen 등록

/* 2. S_ServerService() : 서버 운영 */
void s_SetSaHandler();                  // (1) 핸들러 추가
int s_AcceptNewSocket();                // (2) 새 소켓 받아오기
int s_UpdateNewSocket(int new_socket);  // (3) 새 소켓을 서버의 클라이언트 소켓에 등록
void s_ForkForClientMessage(int idx);   // (4) 해당 클라이언트의 메세지를 받는 프로세스 fork()
void s_Broadcast();                     // (5) 수신된 메시지 확인 후 배포


/* 내부 추가 함수 */
void s_HandleSigchld(int sig);         // 1-[1]  SIGCHLD 핸들러: 자식 프로세스 종료 시 호출
void s_MakeNonblock(int fd);            // 1-[2] 해당 소켓/파이프를 논블락으로 만들기
void s_GetMessageFromClient(int idx);   // 2-[1] 클라이언트에서 메시지 받기
#endif