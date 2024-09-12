#ifndef _CLIENT_H_
#define _CLIENT_H_

/* C언어 표준 헤더 */
#include <arpa/inet.h> // struct sockaddr_in
#include <sys/socket.h>

/* 추가 헤더 */
#include "Macros.h"

/* 클라이언트 구조체 정의 */
typedef struct client {
    int sockfd;
    char id[ID_SIZE];
    struct sockaddr_in server_addr;
} Client;

// 클라이언트는 하나만 존재
static Client User;

/* main.c용 함수. C_로 시작 */
void C_init();          // 클라이언트 초기화
void C_Connect();       // 서버 연결
void C_ClientService(); // 클라이언트 서비스
void C_Close();         // 클라이언트 종료

/* Client.c용 함수 선언. c_로 시작 */
void c_SetUserId(); // 사용자 ID 설정
void c_InitClientSocket(); // 클라이언트 소켓 생성
void c_SetServerAddress(); // 서버 주소 설정
void c_ConnectToServer(); // 서버 연결

void c_ChildProcess(int pipefd[2], char* buffer); // 자식 프로세스: 사용자 입력을 처리
void c_ParentProcess(int pipefd[2], char* buffer); // 부모 프로세스: 서버와의 통신 처리
void c_MakeNonblock(int fd);

// void c_SetBuffer(char* chat); // "ID : (채팅 내용)"양식으로 버퍼 업데이트
#endif