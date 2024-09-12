#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Server.h"

/* ===== main.c용 함수 구현 ====== */
/* 서버 초기화 */
void S_Init() {
//    printf("S_Init()\n");
    s_InitClientSocket();
    s_InitServerSocket();    
    s_SetServerAddress();
    s_BindServerSocket();
}

/* 서버 운영 */
void S_ServerService() {
//    printf("S_ServerService()\n");
    // int pipes[ChatServer.max_client][2]; // 각 클라이언트와 통신할 파이프 배열
    s_ListenClients();

    while (1) {
        // printf("S_ServerService() - while() 1\n");

        // 새로운 클라이언트 연결 수락
        int addrlen = sizeof(ChatServer.address);
        printf("S_ServerService() - while() 2\n");
        int new_socket = accept(ChatServer.server_fd, 
            (struct sockaddr *)&(ChatServer.address), (socklen_t *)&addrlen);
        printf("S_ServerService() - while() 3\n");

        if (new_socket < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        printf("클라이언트 접속, socket fd: %d\n", new_socket);

        // 자식 프로세스 생성
        pid_t pid = fork();

        if (pid == 0) { // 자식 프로세스
            s_ChildProcess(new_socket);
        } 
        else { // 부모 프로세스
            s_ParentProcess(new_socket);
        }
    }
}

/* 서버 종료 */ 
void S_Close() {
    printf("S_Close()\n");
    close(ChatServer.server_fd);
}

/* ===== Server.c용 함수 구현 ====== */
/* 클라이언트 소켓 초기화 */
void s_InitClientSocket() {
    printf("s_InitClientSocket()\n");
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
void s_InitServerSocket() {
    printf("s_InitServerSocket()\n");
    ChatServer.server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ChatServer.server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
}

/* 서버 주소 설정 */
void s_SetServerAddress() {
    printf("s_SetServerAddress()\n");
    ChatServer.address.sin_family = AF_INET;
    ChatServer.address.sin_addr.s_addr = inet_addr(PI_ADDR);  // 로컬 IP
    ChatServer.address.sin_port = htons(PORT);
    printf("server ip1 : %u\n", ChatServer.address.sin_addr.s_addr);

    if (inet_pton(AF_INET, PI_ADDR, &(ChatServer.address.sin_addr)) <= 0) {
        perror("Invalid address / Address not supported");
        close(ChatServer.server_fd);
        exit(EXIT_FAILURE);
    }
}

/* 서버 소켓 바인딩 */
void s_BindServerSocket() {
    printf("s_BindServerSocket()\n");
    if (bind(ChatServer.server_fd, (struct sockaddr *)&(ChatServer.address), 
        sizeof(ChatServer.address)) < 0) {
        perror("bind failed");
        close(ChatServer.server_fd);
        exit(EXIT_FAILURE);
    }
    printf("server ip2 : %u\n", ChatServer.address.sin_addr.s_addr);
}

/* 클라이언트 접속 대기 */
void s_ListenClients() {
    printf("s_ListenClients()\n");
    if (listen(ChatServer.server_fd, ChatServer.max_client) < 0) {
        perror("listen failed");
        close(ChatServer.server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Chat server started on port %d\n", PORT);
}

/* 자식 프로세스: 클라이언트 통신 처리 */    
void s_ChildProcess(int new_socket) {
    printf("s_ChildProcess() 1\n");
    close(ChatServer.server_fd); // 자식 프로세스는 서버 소켓을 닫음

    char buffer[BUFFER_SIZE];

    while (1) {
        printf("s_ChildProcess() 2\n");
        // 클라이언트로부터 데이터 수신
        int valread = read(new_socket, buffer, BUFFER_SIZE);
        printf("s_ChildProcess() 3\n");
        if (valread == 0) {
            // 클라이언트 연결 종료
            printf("Client disconnected, socket fd: %d\n", new_socket);
            close(new_socket);
            exit(0);
        }

        printf("s_ChildProcess() 4\n");
        buffer[valread] = '\0';
        printf("Received from client %d: %s\n", new_socket, buffer);

        // 부모 프로세스에 메시지 전송
        write(ChatServer.pipes[new_socket][1], buffer, strlen(buffer));
    }
}

// 부모 프로세스: 다른 클라이언트들에게 메시지 브로드캐스트
void s_ParentProcess(int new_socket) {
    printf("s_ParentProcess() 1\n");
    close(new_socket); // 부모는 클라이언트 소켓을 닫음

    char buffer[BUFFER_SIZE];

    // 파이프에서 메시지를 읽고 다른 클라이언트에게 전송
    while (1) {
        printf("s_ParentProcess - while1\n");
        for (int i = 0; i < ChatServer.max_client; i++) {
            printf("s_ParentProcess - while2\n");

            int bytes_read = read(ChatServer.pipes[i][0], buffer, BUFFER_SIZE);

            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Broadcasting message: %s\n", buffer);

                // 다른 클라이언트들에게 메시지 브로드캐스트
                for (int j = 0; j < ChatServer.max_client; j++) {
                    if (ChatServer.client_socket[j] != 0 && i != j) {
                        send(ChatServer.client_socket[j], buffer, strlen(buffer), 0);
                    }
                }
            }
        }
    }
}