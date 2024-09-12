#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> // non-blocking
#include "Server.h"

/* ===== main.c용 함수 구현 ====== */
/* 서버 초기화 */
void S_Init() {
    s_InitClientSocket();
    s_InitServerSocket();
    s_InitPipes();    
    s_SetServerAddress();
    s_BindServerSocket();
}

/* 서버 운영 */
void S_ServerService() {
    s_ListenClients();
    s_WaitForAccept();
}

/* 서버 종료 */ 
void S_Close() {
    close(ChatServer.server_sock);
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
void s_InitServerSocket() {
    ChatServer.server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ChatServer.server_sock == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
}

/* 파이프 초기화 */
void s_InitPipes() {
    for (int i = 0; i < ChatServer.max_client; i++) {
        if (pipe(ChatServer.pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
}

/* 서버 주소 설정 */
void s_SetServerAddress() {
    ChatServer.address.sin_family = AF_INET;
    ChatServer.address.sin_addr.s_addr = inet_addr(PI_ADDR);  // 로컬 IP
    ChatServer.address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, PI_ADDR, &(ChatServer.address.sin_addr)) <= 0) {
        perror("Invalid address / Address not supported");
        close(ChatServer.server_sock);
        exit(EXIT_FAILURE);
    }
}

/* 서버 소켓 바인딩 */
void s_BindServerSocket() {
    if (bind(ChatServer.server_sock, (struct sockaddr *)&(ChatServer.address), 
        sizeof(ChatServer.address)) < 0) {
        perror("bind failed");
        close(ChatServer.server_sock);
        exit(EXIT_FAILURE);
    }
}

/* 클라이언트 접속 대기 */
void s_ListenClients() {
    if (listen(ChatServer.server_sock, ChatServer.max_client) < 0) {
        perror("listen failed");
        close(ChatServer.server_sock);
        exit(EXIT_FAILURE);
    }
}

void s_ReadAndBroad(int idx) {
    pid_t pid = fork();
    if (pid == 0) { 
        s_GetMessageFromClient(idx); // 클라이언트 : 메시지 수신
        exit(0);
    }

    char buffer[BUFFER_SIZE];
    while(1) {
        if(ChatServer.client_socket[idx] == 0) {
            exit(0);
        }

        int bytes_read = read(ChatServer.pipes[idx][0], buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Broadcasting message: %s\n", buffer);
            printf("from : %d\n", idx);
            // 다른 클라이언트들에게 메시지 브로드캐스트
            for (int i = 0; i < ChatServer.max_client; i++) {
                if (ChatServer.client_socket[i] != 0 && idx != i) {
                    send(ChatServer.client_socket[i], buffer, strlen(buffer), 0);
                }
            }
        }
    }
}

/* 클라이언트 Accept 대기 */
void s_WaitForAccept() {
    while (1) {
        int new_socket = s_AcceptNewSocket();
        printf("new_socket1 : %d\n", new_socket);

        int idx = s_UpdateNewSocket(new_socket);
        s_ForkForClientMessage(idx);
    }
}

/* 클라이언트 Accept. new_socket 반환 */
int s_AcceptNewSocket() {
    int addrlen = sizeof(ChatServer.address);
    struct sockaddr client_addr;
    int new_socket = accept(ChatServer.server_sock, 
        (struct sockaddr*)&(client_addr), (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    printf("클라이언트 접속, socket fd: %d\n", new_socket);
    return new_socket;
}

/* new_socket을 빈 파이프에 위치. 해당 인덱스 반환 */
int s_UpdateNewSocket(int new_socket) {
    int idx = -1;
    for(int i = 1; i < ChatServer.max_client; i++) {
        if(ChatServer.client_socket[i] == 0) {
            ChatServer.client_socket[i] = new_socket;
            idx = i;
            break;
        }
    }    
    if(idx == -1) {
        printf("idx Error\n");
        exit(1);
    }
    return idx;
}

/* 인덱스에 해당하는 클라이언트의 메시지 수신 */
void s_ForkForClientMessage(int idx) {
    pid_t pid = fork();
    if (pid == 0) { 
        s_ReadAndBroad(idx);
        exit(0);
    }
    /*
    pid = fork();
    if (pid == 0) { 
        s_GetMessageFromClient(idx); // 클라이언트 : 메시지 수신
        exit(0);
    }
    */
}

/* 클라이언트에서 메시지 받기 */    
void s_GetMessageFromClient(int idx) {
    close(ChatServer.server_sock); 
    char buffer[BUFFER_SIZE];

    while (1) {
        // 클라이언트로부터 데이터 수신
        int valread = read(ChatServer.client_socket[idx], buffer, BUFFER_SIZE);
        if (valread == 0) {
            // 클라이언트 연결 종료
            printf("Client disconnected, socket fd: %d\n", ChatServer.client_socket[idx]);
            close(ChatServer.client_socket[idx]);
            ChatServer.client_socket[idx] = 0;
            exit(0);
        }
        buffer[valread] = '\0';
        // 부모 프로세스에 메시지 전송
        write(ChatServer.pipes[idx][1], buffer, strlen(buffer));
    }
    exit(0); // 프로세스 종료
}

/* 해당 소켓/파이프를 논블락으로 만들기 */
void s_MakeNonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(1);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(1);
    }
}