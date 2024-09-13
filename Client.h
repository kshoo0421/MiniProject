#ifndef _CLIENT_H_
#define _CLIENT_H_

/* C언어 표준 헤더 */
#include <arpa/inet.h> // struct sockaddr_in
#include <sys/socket.h>

/* 추가 헤더 */
#include "Macros.h"

/* 클라이언트 구조체 정의 */
typedef struct client {
    int isLoginned, sockfd;
    char id[ID_SIZE];
    char pw[PW_SIZE];
    struct sockaddr_in server_addr;
} Client;

// 클라이언트는 하나만 존재
static Client User;

/* main.c용 함수. C_로 시작 */
void C_init();          // 1. 클라이언트 초기화
void C_Connect();       // 2. 서버 연결
void C_ClientService(); // 3. 클라이언트 서비스
void C_Close();         // 4. 클라이언트 종료

/* Client.c용 함수 선언. c_로 시작 */
/* 1. C_init() : 클라이언트 초기화 */
void c_SetUserId();         // (1) 사용자 ID 설정
void c_InitClientSocket();  // (2) 클라이언트 소켓 생성
void c_SetServerAddress();  // (3) 서버 주소 설정

/* 2. C_Connect() : 서버 연결 */
void c_ConnectToServer();   // (1) 서버 연결

/* 3. C_ClientService() : 클라이언트 서비스*/
// (1) 자식 프로세스 : 사용자 입력을 처리
void c_ChildProcess(int pipefd[2], int pipe2[2], char* buffer); 
// (2) 부모 프로세스: 서버와의 통신 처리
void c_ParentProcess(int pipefd[2], int pipe2[2], char* buffer); 

/* 추가 내부 함수 : 3-(2) */
void c_MakeNonblock(int fd);            // [1] 해당 fd를 non-block으로 만듦
void c_AddId(char buffer[BUFFER_SIZE]); // [2] 송신하는 내용 앞에 id 추가
void c_SignIn(char buffer[BUFFER_SIZE], char id[20], char pw[20]);
void c_SignUp(char buffer[BUFFER_SIZE], char id[20], char pw[20]);
void c_SignOut(char buffer[BUFFER_SIZE], char id[20], char pw[20]);

void c_ClearInputBuffer();
void c_HideLetters();
void c_ShowLetters();
#endif