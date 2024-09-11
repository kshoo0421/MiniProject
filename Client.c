#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Client.h"

/* ===== main.c용 함수 구현 ===== */
/* 클라이언트 초기화 */
void C_init() {
    c_SetUserId(); // 사용자 ID 설정
}

/* 클라이언트를 서버에 연결 요청. 연결 실패 시 종료 */
void C_Connect() {
    c_InitClientSocket();
    c_ConnectToServer();
}

void C_ClientService() {
    // 자식 프로세스 생성
    int pid = fork();
    if (pid < 0) { // fork 실패
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) { // 자식 프로세스: 사용자 입력을 처리
        c_ChildProcess();
    } 
    else {
        c_ParentProcess(); // 부모 프로세스: 서버와의 통신 처리
    }
}

/* 클라이언트-서버 연결 끊기 */
void C_Close() {
    close(User.sockfd);
}


/* ===== 내부 함수 구현 ===== */
/* 사용자 ID 설정 */
void c_SetUserId() { 
    printf("채팅에 사용할 ID를 입력하세요(최대 20글자) : ");
    fgets(User.id, sizeof(User.id), stdin);
    size_t len = strlen(User.id);
    if (len > 0 && User.id[len - 1] == '\n') {
        User.id[len - 1] = '\0';  // 개행 문자 제거
    }
    printf("현재 id : %s\n", User.id);
}

/* 클라이언트 소켓 생성 */
void c_InitClientSocket() {
    User.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (User.sockfd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
}

/* 서버 주소 설정 */
void c_SetServerAddress() {
    // 서버 주소 설정
    User.server_addr.sin_family = AF_INET;
    User.server_addr.sin_addr.s_addr = inet_addr(PI_ADDR);  // 로컬 IP
    User.server_addr.sin_port = htons(PORT);

    // 서버 IP 주소 (localhost)
    if (inet_pton(AF_INET, PI_ADDR, &(User.server_addr.sin_addr)) <= 0) {
        perror("Invalid address / Address not supported");
        close(User.sockfd);
        exit(EXIT_FAILURE);
    }
}

/* 서버 연결 */
void c_ConnectToServer() {
    if (connect(User.sockfd, (struct sockaddr *)&(User.server_addr), 
    sizeof(User.server_addr)) < 0) {
        perror("Connection failed");
        close(User.sockfd);
        exit(EXIT_FAILURE);
    }
    printf("서버에 연결되었습니다.\n");
}

/* 자식 프로세스: 사용자 입력을 처리 */
void c_ChildProcess() {
    close(User.pipefd[0]);  // 자식은 읽기 파이프를 닫음

    while (1) {
        memset(User.buffer, 0, BUFFER_SIZE);
        fgets(User.buffer, BUFFER_SIZE, stdin);
        User.buffer[strcspn(User.buffer, "\n")] = '\0'; // 줄바꿈 제거

        // "exit" 입력 시 클라이언트 종료
        if (strcmp(User.buffer, "exit") == 0) {
            printf("종료\n");
            write(User.pipefd[1], "exit", strlen("exit"));
            break;
        }

        // 사용자 입력을 파이프로 부모에게 전달
        write(User.pipefd[1], User.buffer, strlen(User.buffer));
    }

    close(User.pipefd[1]);
    exit(0);
}

/* 부모 프로세스: 서버와의 통신 처리 */
void c_ParentProcess() {
    close(User.pipefd[1]);  // 부모는 쓰기 파이프를 닫음

    while (1) {
        // 파이프에서 사용자 입력 읽기
        memset(User.buffer, 0, BUFFER_SIZE);
        int bytes_read = read(User.pipefd[0], User.buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            User.buffer[bytes_read] = '\0';
            // "exit" 메시지를 받으면 종료
            if (strcmp(User.buffer, "exit") == 0) {
                break;
            }
            // 서버에 메시지 전송
            send(User.sockfd, User.buffer, strlen(User.buffer), 0);
        }

        // 서버로부터의 메시지 수신 처리
        memset(User.buffer, 0, BUFFER_SIZE);
        int valread = read(User.sockfd, User.buffer, BUFFER_SIZE);
        if (valread > 0) {
            User.buffer[valread] = '\0';
            printf("%s\n", User.buffer);
        }
    }
    close(User.pipefd[0]);
    close(User.sockfd);
}

/* "ID : (채팅 내용)"에 채팅 내용을 업데이트 */
void c_SetBuffer(char* chat) {
    int idLen = strlen(User.id);
    int chatLen = strlen(chat);
    if(idLen + chatLen + 2 >= BUFFER_SIZE) {
        chat[BUFFER_SIZE - idLen - 2] = '\0';
    }
    strcpy(User.buffer, User.id);
    User.buffer[idLen] = ':';
    User.buffer[idLen + 1] = ' ';

    strcpy((User.buffer) + idLen + 2, chat);
    
    // check
    printf("c_SetBuffer check : %s\n", User.buffer);
}

