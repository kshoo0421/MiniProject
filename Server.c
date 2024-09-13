#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> // non-blocking
#include <sys/wait.h>
#include "Server.h"

/* ===== main.c용 함수 구현 ====== */
/* 1. 서버 초기화 */
void S_Init() {
    s_MakeDaemon();         // (1) 서버 프로그램을 데몬 프로세스로
    s_InitMembers();        // (2) 서버 멤버변수 초기화
    s_InitClientSocket();   // (3) 클라이언트용 소켓 초기화
    s_InitServerSocket();   // (4) 서버 소켓 초기화
    s_InitPipes();          // (5) 파이프들 초기화
    s_SetServerAddress();   // (6) 서버 주소 설정
    s_BindServerSocket();   // (7) 서버 주소 - 서버 소켓 연결
    s_ListenClients();      // (8) 클라이언트 Listen 등록
}

/* 2. 서버 운영 */
void S_ServerService() {
    s_SetSaHandler(); // 자식 프로세스 종료 시 자동 회수
    while(1) {
        int new_socket = s_AcceptNewSocket(); // 클라이언트 연결 -> 새 소켓 생성
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

void s_InitMembers() {
    ChatServer.cur_mem = 0;
    for(int i = 0; i < MAX_CLIENT; i++) {
        ChatServer.loginned[i] = 0;
        memset(ChatServer.IDPW[i], 0, 20);
    }
}


/* === 1. S_Init() : 서버 초기화 === */
void s_MakeDaemon() {
    pid_t pid;
    if((pid = fork()) < 0) {
        perror("error()");
    } else if(pid != 0) { /* 부모 프로세스는 종료한다. */
        exit(0);
    }
}

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
    // 3. 클라이언트 소켓 초기화
    for (int i = 0; i < MAX_CLIENT; i++) {
        ChatServer.client_socket[i] = 0;
    }
}

/* 1-(5) 파이프들 초기화 */
void s_InitPipes() {
    for (int i = 0; i < MAX_CLIENT; i++) {
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
    if (listen(ChatServer.server_sock, MAX_CLIENT) < 0) {
        perror("listen failed");
        close(ChatServer.server_sock);
        exit(EXIT_FAILURE);
    }
}


/* === 2. S_ServerService() : 서버 운영 === */
void s_SetSaHandler() {
    // SIGCHLD 시그널을 처리할 핸들러 설정
    struct sigaction sa;
    sa.sa_handler = &s_handle_sigchld;
    sa.sa_flags = SA_RESTART;  // 중단된 시스템 호출 재시작
    sigaction(SIGCHLD, &sa, NULL);
}

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
    for(int i = 0; i < MAX_CLIENT; i++) {
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
    }
}

/* 2-(4) 수신된 메시지 확인 후 배포 */
void s_Broadcast() {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int bytes_read;
    for(int i = 0; i < MAX_CLIENT; i++) {
        if(ChatServer.client_socket[i] == 0) continue; // 빈 소켓이면 통과

        bytes_read = read(ChatServer.pipes[i][0], buffer, BUFFER_SIZE);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            if(buffer[0] == '\0') continue;
            else if(buffer[0] == 'S') { // 로그인 관련
                if(buffer[1] == 'I') {
                    int idx = -1;
                    for(int i = 0; i < ChatServer.cur_mem; i++) {
                        if(strcmp(buffer, ChatServer.IDPW[i]) == 0) {
                            if(ChatServer.loginned[i] == 0) {
                                ChatServer.loginned[i] = 1; // 로그인 활성화
                                idx = i;
                            }
                            break;
                        }
                    }
                    if(idx != -1) {
                        strcpy(buffer, SIGNIN_SUCCESS); // 로그인 성공
                    }
                    else {
                        strcpy(buffer, SIGNIN_FAILED); // 로그인 실패
                    }
                }
                else if(buffer[1] == 'U') {
                    buffer[1] = 'I';
                    int idx = -1;
                    for(int i = 0; i < ChatServer.cur_mem; i++) {
                        if(strcmp(buffer, ChatServer.IDPW[i]) == 0) {
                            idx = i;
                            break;
                        }
                    }
                    if(idx != -1) {
                        strcpy(buffer, SIGNUP_FAILED); // 회원가입 실패
                    }
                    else {
                        strcpy(ChatServer.IDPW[ChatServer.cur_mem++], buffer);
                        strcpy(buffer, SIGNUP_SUCCESS); // 회원가입 성공
                    }
                }
                else if(buffer[1] == 'O') { // 로그아웃
                    buffer[1] = 'I';
                    int idx = -1;
                    for(int i = 0; i < ChatServer.cur_mem; i++) {
                        if(strcmp(buffer, ChatServer.IDPW[i]) == 0) {
                            if(ChatServer.loginned[i] == 1) {
                                ChatServer.loginned[i] = 0;
                                idx = i;
                            }
                            break;
                        }
                    }
                    if(idx == -1) {
                        strcpy(buffer, SIGNOUT_FAILED); // 로그아웃 실패
                    }
                    else {
                        strcpy(buffer, SIGNOUT_SUCCESS); // 로그아웃 성공
                    }
                }
                send(ChatServer.client_socket[i], buffer, strlen(buffer), 0);
            }
            else { // if(buffer[0] == '[') 
                printf("Broadcasting message: %s\n", buffer);
                // 다른 클라이언트들에게 메시지 브로드캐스트
                for (int j = 0; j < MAX_CLIENT; j++) {
                    if (ChatServer.client_socket[j] != 0 && i != j) {
                        send(ChatServer.client_socket[j], buffer, strlen(buffer), 0);
                    }
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
        if (valread == 0) { // 클라이언트 연결 종료
            printf("클라이언트 연결 종료: %d\n", ChatServer.client_socket[idx]);
            break;
        }
        buffer[valread] = '\0';
        // 부모 프로세스에 메시지 전송
        write(ChatServer.pipes[idx][1], buffer, strlen(buffer));
    }
    close(ChatServer.client_socket[idx]);
    close(ChatServer.pipes[idx][1]);
    ChatServer.client_socket[idx] = 0;
    exit(0); // 프로세스 종료
}

// SIGCHLD 핸들러: 자식 프로세스 종료 시 호출
void s_handle_sigchld(int sig) {
    int status;
    pid_t pid;

    // 종료된 모든 자식 프로세스 회수
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("자식 프로세스 회수(PID: %d), \
            종료 상태: %d\n", pid, WEXITSTATUS(status));
    }
}

