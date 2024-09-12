#ifndef _SERVER_H_
#define _SERVER_H_

#include <arpa/inet.h> // struct sockaddr_in

#include "Macros.h"

typedef struct server {
    int max_client, server_fd, max_sd, sd, activity;
    int* client_socket;
    int pipes[50][2];

    struct sockaddr_in address;
} Server;

// 서버는 하나만 존재
static Server ChatServer;

/* main.c용 함수. S_로 시작 */
void S_Init();          // 서버 초기화
void S_ServerService(); // 서버 운영
void S_Close();         // 서버 종료

/* Server.c용 함수 선언. s_로 시작*/
void s_InitClientSocket(); // 클라이언트 소켓 초기화
void s_InitServerSocket(); // 서버 소켓 초기화
void s_SetServerAddress(); // 서버 주소 설정
void s_BindServerSocket(); // 서버 소켓 바인딩

void s_ListenClients(); // 클라이언트 접속 대기
void s_ChildProcess(int new_socket);
void s_ParentProcess(int new_socket);
#endif