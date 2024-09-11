#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Server.h"

/* ===== main.c용 함수 구현 ====== */
/* 서버 초기화 */
void S_Init() {
    s_InitClientSocket();
    s_InitServerSocket();    
    s_SetServerAddress();
    s_BindServerSocket();
}

/* 서버 운영 */
void S_ServerService() {
    // int pipes[ChatServer.max_client][2]; // 각 클라이언트와 통신할 파이프 배열
    s_ListenClients();

    while (1) {
        // 새로운 클라이언트 연결 수락
        int addrlen = sizeof(ChatServer.address);
        ChatServer.new_socket = accept(ChatServer.server_fd, 
            (struct sockaddr *)&(ChatServer.address), (socklen_t *)&addrlen);
        if (ChatServer.new_socket < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        printf("클라이언트 접속, socket fd: %d\n", ChatServer.new_socket);

        // 자식 프로세스 생성
        ChatServer.pid = fork();

        if (ChatServer.pid == 0) { // 자식 프로세스
            s_ChildProcess();
        } 
        else { // 부모 프로세스
            s_ParentProcess();
        }
    }
}

/* 서버 종료 */ 
void S_Close() {
    close(ChatServer.server_fd);
}

/* ===== Server.c용 함수 구현 ====== */
/* 클라이언트 소켓 초기화 */
void s_InitClientSocket() { 
    // 최대 클라이언트 갯수 초기값 : 50
    ChatServer.max_client = 50;
    // 최대 클라이언트 갯수만큼 할당
    ChatServer.client_socket = (int*)malloc(sizeof(int)*(ChatServer.max_client));

    // 클라이언트 소켓 초기화
    for (int i = 0; i < ChatServer.max_client; i++) {
        ChatServer.client_socket[i] = 0;
    }
}

/* 서버 소켓 초기화 */
void s_InitServerSocket(){
    ChatServer.server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ChatServer.server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
}

/* 서버 주소 설정 */
void s_SetServerAddress() {
    ChatServer.address.sin_family = AF_INET;
    ChatServer.address.sin_addr.s_addr = INADDR_ANY;
    ChatServer.address.sin_port = htons(PORT);
}

/* 서버 소켓 바인딩 */
void s_BindServerSocket() {
    if (bind(ChatServer.server_fd, (struct sockaddr *)&(ChatServer.address), 
    sizeof(ChatServer.address)) < 0) {
        perror("bind failed");
        close(ChatServer.server_fd);
        exit(EXIT_FAILURE);
    }
}

/* 클라이언트 접속 대기 */
void s_ListenClients() {
    if (listen(ChatServer.server_fd, ChatServer.max_client) < 0) {
        perror("listen failed");
        close(ChatServer.server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Chat server started on port %d\n", PORT);
}

/* 자식 프로세스: 클라이언트 통신 처리 */    
void s_ChildProcess() {
    close(ChatServer.server_fd); // 자식 프로세스는 서버 소켓을 닫음
    while (1) {
        // 클라이언트로부터 데이터 수신
        int valread = read(ChatServer.new_socket, ChatServer.buffer, BUFFER_SIZE);
        if (valread == 0) {
            // 클라이언트 연결 종료
            printf("Client disconnected, socket fd: %d\n", ChatServer.new_socket);
            close(ChatServer.new_socket);
            exit(0);
        }

        ChatServer.buffer[valread] = '\0';
        printf("Received from client %d: %s\n", ChatServer.new_socket, ChatServer.buffer);

        // 부모 프로세스에 메시지 전송
        write(ChatServer.pipes[ChatServer.new_socket][1], ChatServer.buffer, strlen(ChatServer.buffer));
    }
}


// 부모 프로세스: 다른 클라이언트들에게 메시지 브로드캐스트
void s_ParentProcess() {
    close(ChatServer.new_socket); // 부모는 클라이언트 소켓을 닫음

    // 파이프에서 메시지를 읽고 다른 클라이언트에게 전송
    while (1) {
        for (int i = 0; i < ChatServer.max_client; i++) {
            char msg_buffer[BUFFER_SIZE];
            int bytes_read = read(ChatServer.pipes[i][0], msg_buffer, BUFFER_SIZE);

            if (bytes_read > 0) {
                msg_buffer[bytes_read] = '\0';
                printf("Broadcasting message: %s\n", msg_buffer);

                // 다른 클라이언트들에게 메시지 브로드캐스트
                for (int j = 0; j < ChatServer.max_client; j++) {
                    if (ChatServer.client_socket[j] != 0 && i != j) {
                        send(ChatServer.client_socket[j], msg_buffer, strlen(msg_buffer), 0);
                    }
                }
            }
        }
    }
}