#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "Server.h"

/* ===== main.c용 함수 구현 ====== */
/* 서버 초기화 */
void S_Init() {
    s_InitClientSocket(); 
    s_InitServerSocket();    
    s_SetServerAddress();
    s_BindServerSocket();
    s_BindServerSocket();
}

/* 서버 운영 */
void S_ServerService() {
    // 클라이언트 접속 대기
    if (listen(ChatServer->server_fd, 3) < 0) {
        perror("listen failed");
        close(ChatServer->server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Chat server started on port %d\n", PORT);
}

/* 서버 종료 */ 
void S_Close() {
    close(ChatServer->server_fd);
}

/* ===== Server.c용 함수 구현 ====== */
/* 클라이언트 소켓 초기화 */
void s_InitClientSocket() { 
    // 최대 클라이언트 갯수만큼 할당
    ChatServer->client_socket = (int*)malloc(sizeof(int)*(ChatServer->max_client));

    // 클라이언트 소켓 초기화
    for (int i = 0; i < ChatServer->max_client; i++) {
        ChatServer->client_socket[i] = 0;
    }
}

/* 서버 소켓 초기화 */
void s_InitServerSocket(){
    ChatServer->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ChatServer->server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
}

/* 서버 주소 설정 */
void s_SetServerAddress() {
    ChatServer->address.sin_family = AF_INET;
    ChatServer->address.sin_addr.s_addr = INADDR_ANY;
    ChatServer->address.sin_port = htons(PORT);
}

/* 서버 소켓 바인딩 */
void s_BindServerSocket() {
    if (bind(ChatServer->server_fd, (struct sockaddr *)&(ChatServer->address), 
    sizeof(ChatServer->address)) < 0) {
        perror("bind failed");
        close(ChatServer->server_fd);
        exit(EXIT_FAILURE);
    }
}