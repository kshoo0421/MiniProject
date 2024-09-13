#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> // non-blocking
#include <termios.h> // 글자 숨기기용
#include "Client.h"

/* ===== main.c용 함수 구현 ===== */
/* 1. 클라이언트 초기화 */
void C_init() {
//    c_SetUserId(); // 사용자 ID 설정
    c_InitClientSocket();
    c_SetServerAddress();
}

/* 2. 서버 연결 */
void C_Connect() {
    c_ConnectToServer();
}

/* 3. 클라이언트 서비스 */
void C_ClientService() {
    int pipefd[2], pipe2[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipe2) == -1) {
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
        c_ChildProcess(pipefd, pipe2, buffer);
    } 
    else {
        c_ParentProcess(pipefd, pipe2, buffer); // 부모 프로세스: 서버와의 통신 처리
    }
}

/* 4. 클라이언트 종료 */
void C_Close() {
    close(User.sockfd);
}

/* ===== 내부 함수 구현 ===== */
/* === 1. C_init() : 클라이언트 초기화 === */

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


/* 2. C_Connect() : 서버 연결 */
/* 2-(1) 서버 연결 */
void c_ConnectToServer() {
    if (connect(User.sockfd, (struct sockaddr *)&(User.server_addr), 
    sizeof(User.server_addr)) < 0) {
        perror("Connection failed");
        close(User.sockfd);
        exit(EXIT_FAILURE);
    }
    printf("서버에 연결되었습니다.\n");
}

/* 3. C_ClientService() : 클라이언트 서비스*/
/* 3-(1) 자식 프로세스: 사용자 입력을 처리 */
void c_ChildProcess(int pipefd[2], int pipe2[2], char* buffer) {
    close(pipefd[0]);  // 자식은 읽기 파이프를 닫음
    close(pipe2[1]); // 부모용 읽기 파이프 닫기
    printf("exit 입력 시 프로그램 종료\n");
    

    while (1) {        
        if(User.isLoginned == 0) {  // 아직 로그인 안됨
            printf("[로그인 시스템]\n");
            printf("1 : 로그인\n");
            printf("2 : 회원가입\n");
            printf("> ");

            memset(buffer, 0, BUFFER_SIZE); 
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // 줄바꿈 제거

            if(strcmp(buffer, "1") == 0) {
                printf("ID(20자 이내) : ");
                fgets(User.id, ID_SIZE, stdin);
                User.id[strcspn(User.id, "\n")] = '\0'; // 줄바꿈 제거

                printf("PW(20자 이내) : ");
                c_HideLetters();
                fgets(User.pw, PW_SIZE, stdin);
                User.pw[strcspn(User.pw, "\n")] = '\0'; // 줄바꿈 제거
                c_ShowLetters();
                printf("\n");

                c_SignIn(buffer, User.id, User.pw);
                write(pipefd[1], buffer, strlen(buffer));

                printf("로그인 시도 중입니다.\n");

                int bytes_read = read(pipe2[0], buffer, BUFFER_SIZE);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    if (strcmp(buffer, SIGNIN_SUCCESS) == 0) {
                        User.isLoginned = 1;
                    }
                }
            }
            else if(strcmp(buffer, "2") == 0) {
                printf("ID(20자 이내) : ");
                fgets(User.id, ID_SIZE, stdin);
                User.id[strcspn(User.id, "\n")] = '\0';

                printf("PW(20자 이내) : ");
                c_HideLetters();
                fgets(User.pw, PW_SIZE, stdin);
                User.pw[strcspn(User.pw, "\n")] = '\0';
                c_ShowLetters();
                printf("\n");

                c_SignUp(buffer, User.id, User.pw);
                write(pipefd[1], buffer, strlen(buffer));
                memset(buffer, 0, BUFFER_SIZE);
                printf("회원가입 중입니다.\n");
                
                int bytes_read = read(pipe2[0], buffer, BUFFER_SIZE);
            }
            else {
                printf("잘못된 값을 입력했습니다.\n");
                printf("다시 입력해주세요.\n");
            }
            continue;
        }

        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0'; // 줄바꿈 제거
        // "exit" 입력 시 클라이언트 종료
        if (strcmp(buffer, "exit") == 0) {
            printf("종료\n");
            write(pipefd[1], "exit", strlen("exit"));
            break;
        }

        if (strcmp(buffer, "logout") == 0) {
            printf("로그아웃\n");
            write(pipefd[1], "logout", strlen("logout"));
            User.isLoginned = 0;
            continue;
        }
        
        c_AddId(buffer);
        // 사용자 입력을 파이프로 부모에게 전달
        write(pipefd[1], buffer, strlen(buffer));
        memset(buffer, 0, BUFFER_SIZE);        
    }
    close(pipefd[1]);
    close(pipe2[0]);
    exit(0);
}

/* 3-(2) 부모 프로세스: 서버와의 통신 처리 */
void c_ParentProcess(int pipefd[2], int pipe2[2], char* buffer) {
    close(pipefd[1]);  // 부모는 쓰기 파이프를 닫음
    close(pipe2[0]);    // 자식이 읽는 파이프 닫기
    c_MakeNonblock(pipefd[0]);
    c_MakeNonblock(User.sockfd);

    while (1) {
        if(User.isLoginned == 0) { // 로그인이 안되었다면
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_read = read(pipefd[0], buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';

                if (strcmp(buffer, "exit") == 0) {
                    printf("프로그램을 종료합니다.\n");
                    exit(0);
                    break;
                }
                send(User.sockfd, buffer, strlen(buffer), 0);
            }
            
            sleep(1);   // 1초 대기

            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(User.sockfd, buffer, BUFFER_SIZE);
            if (valread > 0) {
                buffer[valread] = '\0';
                if(strcmp(buffer, SIGNIN_FAILED) == 0) {
                    printf("로그인에 실패했습니다.\n");
                    printf("다시 시도해주세요.\n");
                }
                else if(strcmp(buffer, SIGNIN_SUCCESS) == 0) {
                    printf("로그인에 성공했습니다.\n");
                    User.isLoginned = 1;
                }
                else if(strcmp(buffer, SIGNUP_FAILED) == 0) {
                    printf("회원가입에 실패했습니다.\n");
                    printf("다시 시도해주세요.\n");
                }
                else if(strcmp(buffer, SIGNUP_SUCCESS) == 0) {
                    printf("회원가입에 성공했습니다.\n");
                    printf("로그인해주세요.\n");
                }
                else if(strcmp(buffer, SIGNOUT_FAILED) == 0) {
                    printf("로그아웃에 실패했습니다.\n");
                    printf("다시 시도해주세요.\n");
                }
                else if(strcmp(buffer, SIGNUP_SUCCESS) == 0) {
                    printf("로그아웃에 성공했습니다.\n");
                    printf("로그인해주세요.\n");
                }
                write(pipe2[1], buffer, strlen(buffer));
            }
            continue;
        }

        // 파이프에서 사용자 입력 읽기
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(pipefd[0], buffer, BUFFER_SIZE - strlen(User.id) - 5);
        // id 칸 확보
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            // "exit" 메시지를 받으면 종료
            if (strcmp(buffer, "exit") == 0) {
                printf("프로그램을 종료합니다.\n");
                break;
            }
            if (strcmp(buffer, "logout") == 0) {
                c_SignOut(buffer, User.id, User.pw);
                send(User.sockfd, buffer, strlen(buffer), 0);
                User.isLoginned = 0;
                continue;
            }
            // 서버에 메시지 전송
            send(User.sockfd, buffer, strlen(buffer), 0);
        }

        // 서버로부터의 메시지 수신 처리
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(User.sockfd, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("%s\n", buffer);
        }
    }
    close(User.sockfd);
    close(pipefd[0]);
    close(pipe2[1]);
    exit(0);
}


/* 추가 내부 함수 : 3-(2) */
/* 3-(2)-[1] 해당 fd를 non-block으로 만듦 */
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

/* 3-(2)-[2] 송신하는 내용 앞에 id 추가 */
void c_AddId(char buffer[BUFFER_SIZE]) {
    char newBuffer[BUFFER_SIZE] = "[";
    strcat(newBuffer, User.id);
    strcat(newBuffer, "] : ");
    strcat(newBuffer, buffer);
    strcpy(buffer, newBuffer);
}

void c_SignIn(char buffer[BUFFER_SIZE], char id[20], char pw[20]) {
    strcpy(buffer, "SI:");
    strcat(buffer, id);
    strcat(buffer, "|");
    strcat(buffer, pw);
}

void c_SignUp(char buffer[BUFFER_SIZE], char id[20], char pw[20]) {
    strcpy(buffer, "SU:");
    strcat(buffer, id);
    strcat(buffer, "|");
    strcat(buffer, pw);
}

void c_SignOut(char buffer[BUFFER_SIZE], char id[20], char pw[20]) {
    strcpy(buffer, "SO:");
    strcat(buffer, id);
    strcat(buffer, "|");
    strcat(buffer, pw);
}

void c_ClearInputBuffer() {
    int c; // getchar()로 입력 버퍼에 남은 모든 문자를 읽기
    // 아무것도 하지 않음, 남은 데이터를 무시    
    while ((c = getchar()) != '\n' && c != EOF);
}

void c_HideLetters() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);  // 현재 터미널 속성 가져오기
    tty.c_lflag &= ~ECHO;           // ECHO 비트 해제: 입력된 문자 표시 안 함
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);  // 변경된 속성 즉시 적용
}

void c_ShowLetters() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);  // 현재 터미널 속성 가져오기
    tty.c_lflag |= ECHO;            // ECHO 비트 설정: 입력된 문자 표시
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);  // 변경된 속성 즉시 적용
}