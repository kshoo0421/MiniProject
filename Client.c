#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> // non-blocking

#include "Client.h"

/* ===== main.c용 함수 구현 ===== */
/* 클라이언트 초기화 */
void C_init() {
    c_SetUserId(); // 사용자 ID 설정
}

/* 클라이언트를 서버에 연결 요청. 연결 실패 시 종료 */
void C_Connect() {
    c_InitClientSocket();
    c_SetServerAddress();
    c_ConnectToServer();
}

void C_ClientService() {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);        

    // 자식 프로세스 생성
    pid_t pid = fork();
    if (pid < 0) { // fork 실패
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) { // 자식 프로세스: 사용자 입력을 처리
        c_ChildProcess(pipefd, buffer);
    } 
    else {
        c_ParentProcess(pipefd, buffer); // 부모 프로세스: 서버와의 통신 처리
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
    User.id[strcspn(User.id, "\n")] = '\0'; // 줄바꿈 제거
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
void c_ChildProcess(int pipefd[2], char* buffer) {
    close(pipefd[0]);  // 자식은 읽기 파이프를 닫음

    while (1) {        
        fgets(buffer, BUFFER_SIZE, stdin);        
        buffer[strcspn(buffer, "\n")] = '\0'; // 줄바꿈 제거
        // "exit" 입력 시 클라이언트 종료
        if (strcmp(buffer, "exit") == 0) {
            printf("종료\n");
            write(pipefd[1], "exit", strlen("exit"));
            break;
        }

        // 사용자 입력을 파이프로 부모에게 전달
        write(pipefd[1], buffer, strlen(buffer));
        memset(buffer, 0, BUFFER_SIZE);        
    }

    close(pipefd[1]);
    exit(0);
}

/* 부모 프로세스: 서버와의 통신 처리 */
void c_ParentProcess(int pipefd[2], char* buffer) {
    close(pipefd[1]);  // 부모는 쓰기 파이프를 닫음
    c_MakeNonblock(pipefd[0]);
    c_MakeNonblock(User.sockfd);

    while (1) {
        // 파이프에서 사용자 입력 읽기
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(pipefd[0], buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            // "exit" 메시지를 받으면 종료
            if (strcmp(buffer, "exit") == 0) {
                break;
            }
            // 서버에 메시지 전송
            send(User.sockfd, buffer, strlen(buffer), 0);
        }

        // 서버로부터의 메시지 수신 처리
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(User.sockfd, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("server : %s\n", buffer);
        }
    }
    close(pipefd[0]);
}

void c_MakeNonblock(int fd) {
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