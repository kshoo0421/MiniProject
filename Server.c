#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> // non-blocking
#include "Server.h"

/* ===== main.c용 함수 구현 ====== */
/* 1. 서버 초기화 */
void S_Init() {
    s_InitServerSocket();   // (1) 서버 소켓 초기화
    s_SetServerAddress();   // (2) 서버 주소 설정
    s_BindServerSocket();   // (3) 서버 주소 - 서버 소켓 연결
    s_InitPipes();          // (4) 파이프들 초기화
    s_InitClientSocket();   // (5) 클라이언트용 소켓 초기화
    s_ListenClients();      // (6) 클라이언트 Listen 등록
}

/* 2. 서버 운영 */
void S_ServerService() {
    while(1) {
        int new_socket = s_AcceptNewSocket();
        if(new_socket > 0) {    // 새 소켓 받아오기 성공 시
            int idx = s_UpdateNewSocket(new_socket);    // 등록
            s_ForkForClientMessage(idx);    // 해당 클라이언트용 추가 프로세스 실행
        }
        s_Broadcast();  // 전체 방송
    }
}

/* 3. 서버 종료 */ 
void S_Close() {
    close(ChatServer.server_sock);
}

/* ===== Server.c용 함수 구현 ====== */
/* === 1. S_Init() : 서버 초기화 === */
/* 1-(1) 서버 소켓 초기화 */
void s_InitServerSocket() {
    ChatServer.server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ChatServer.server_sock == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    s_MakeNonblock(ChatServer.server_sock);
}

/* 1-(2) 서버 주소 설정 */
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

/* 1-(3) 서버 주소 - 서버 소켓 연결 */
void s_BindServerSocket() {
    if (bind(ChatServer.server_sock, (struct sockaddr *)&(ChatServer.address), 
            sizeof(ChatServer.address)) < 0) {
        perror("bind failed");
        close(ChatServer.server_sock);
        exit(EXIT_FAILURE);
    }
}

/* 1-(4) 클라이언트용 소켓 초기화 */
void s_InitClientSocket() {
    // 1. 최대 클라이언트 갯수 초기값 : 50
    ChatServer.max_client = 50;

    // 2. 최대 클라이언트 갯수만큼 소켓 할당
    ChatServer.client_socket =      
        (int*)malloc(sizeof(int)*(ChatServer.max_client));

    // 3. 클라이언트 소켓 초기화
    for (int i = 0; i < ChatServer.max_client; i++) {
        ChatServer.client_socket[i] = 0;
    }
}

/* 1-(5) 파이프들 초기화 */
void s_InitPipes() {
    for (int i = 0; i < ChatServer.max_client; i++) {
        if (pipe(ChatServer.pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        s_MakeNonblock(ChatServer.pipes[i][0]);
        s_MakeNonblock(ChatServer.pipes[i][1]);
    }
}

/* 1-(6) 클라이언트 Listen 등록 */
void s_ListenClients() {
    if (listen(ChatServer.server_sock, ChatServer.max_client) < 0) {
        perror("listen failed");
        close(ChatServer.server_sock);
        exit(EXIT_FAILURE);
    }
}


/* === 2. S_ServerService() : 서버 운영 === */
/* 2-(1) 새 소켓 받아오기 */
int s_AcceptNewSocket() {
    int addrlen = sizeof(ChatServer.address);
    struct sockaddr client_addr;
    int new_socket = accept(ChatServer.server_sock, 
        (struct sockaddr*)&(client_addr), (socklen_t*)&addrlen);
    if(new_socket > 0) {
        printf("클라이언트 접속, socket fd: %d\n", new_socket);
    }
    return new_socket;
}

/* 2-(2) 새 소켓을 서버의 클라이언트 소켓에 등록 */
int s_UpdateNewSocket(int new_socket) {
    int idx = -1;
    for(int i = 0; i < ChatServer.max_client; i++) {
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

/* 2-(3) 해당 클라이언트의 메세지를 받는 프로세스 fork() */
void s_ForkForClientMessage(int idx) {
    pid_t pid = fork();
    if (pid == 0) { 
        s_GetMessageFromClient(idx); // 클라이언트 : 메시지 수신
        exit(0);
    }
}

/* 2-(4) 수신된 메시지 확인 후 배포 */
void s_Broadcast() {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int bytes_read;
    for(int i = 0; i < ChatServer.max_client; i++) {
        if(ChatServer.client_socket[i] == 0) continue; // 빈 소켓이면 통과

        bytes_read = read(ChatServer.pipes[i][0], buffer, BUFFER_SIZE);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            if(buffer[0] == '\0') continue;
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


/* === 내부 추가 함수 === */
/* 1-(1)-[1] 해당 소켓/파이프를 논블락으로 만들기 */
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

/* 2-(3)-[1] 클라이언트에서 메시지 받기 */    
void s_GetMessageFromClient(int idx) {
    close(ChatServer.server_sock); 
    char buffer[BUFFER_SIZE];

    s_MakeNonblock(ChatServer.client_socket[idx]);
    while (1) {
        // 클라이언트로부터 데이터 수신
        memset(buffer, 0, sizeof(buffer));
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
