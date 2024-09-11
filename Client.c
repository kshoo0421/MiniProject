#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Client.h"

/* ===== main.c용 함수 구현 ===== */
/* 클라이언트 초기화 */
void C_init() {
    User = (Client*)malloc(sizeof(Client));
    c_SetUserId(); // 사용자 ID 설정
}

/* 클라이언트를 서버에 연결 요청. 연결 실패 시 종료 */
void C_Connect() {
    c_InitClientSocket();
    c_ConnectToServer();
}

void C_ClientService() {
    while(1) {
        c_PrintOptions();
        int option = c_SelectOption();
        if(option == -1) {
            printf("연결을 종료하고, 프로그램을 종료합니다.\n");
            break;
        }

        switch(option) {
        case 1:
            printf("접속할 수 있는 대화방을 출력합니다.");
            break;
        default:
            printf("잘못된 값을 입력하였습니다. 다시 입력해주세요.\n");
            break;
        };
    }
}

/* 클라이언트-서버 연결 끊기 */
void C_Close() {
    close(User->sockfd);
}


/* ===== 내부 함수 구현 ===== */
/* 사용자 ID 설정 */
void c_SetUserId() { 
    printf("채팅에 사용할 ID를 입력하세요(최대 20글자) : ");
    fgets(User->id, sizeof(User->id), stdin);
    size_t len = strlen(User->id);
    if (len > 0 && User->id[len - 1] == '\n') {
        User->id[len - 1] = '\0';  // 개행 문자 제거
    }
    printf("현재 id : %s\n", User->id);
}

/* 클라이언트 소켓 생성 */
void c_InitClientSocket() {
    User->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (User->sockfd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
}

/* 서버 주소 설정 */
void c_SetServerAddress() {
    // 서버 주소 설정
    User->server_addr.sin_family = AF_INET;
    User->server_addr.sin_addr.s_addr = inet_addr(PI_ADDR);  // 로컬 IP
    User->server_addr.sin_port = htons(PORT);

    // 서버 IP 주소 (localhost)
    if (inet_pton(AF_INET, PI_ADDR, &(User->server_addr.sin_addr)) <= 0) {
        perror("Invalid address / Address not supported");
        close(User->sockfd);
        exit(EXIT_FAILURE);
    }
}

/* 서버 연결 */
void c_ConnectToServer() {
    if (connect(User->sockfd, (struct sockaddr *)&(User->server_addr), sizeof(User->server_addr)) < 0) {
        perror("Connection failed");
        close(User->sockfd);
        exit(EXIT_FAILURE);
    }
    printf("서버에 연결되었습니다.\n");
}

/* 클라이언트 옵션 출력 */
void c_PrintOptions() {

} 

/* 옵션 선택 */
int c_SelectOption() {  

} 


/* "ID : (채팅 내용)"에 채팅 내용을 업데이트 */
void c_SetBuffer(char* chat) {
    int idLen = strlen(User->id);
    int chatLen = strlen(chat);
    if(idLen + chatLen + 2 >= BUFFER_SIZE) {
        chat[BUFFER_SIZE - idLen - 2] = '\0';
    }
    strcpy(User->buffer, User->id);
    User->buffer[idLen] = ':';
    User->buffer[idLen + 1] = ' ';

    strcpy((User->buffer) + idLen + 2, chat);
    
    // check
    printf("c_SetBuffer check : %s\n", User->buffer);
}
