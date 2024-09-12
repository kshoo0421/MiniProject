#ifndef _MACROS_H_
#define _MACROS_H_

/* 클라이언트인지 서버인지 구분 */
#define CLIENT_MODE
// #define SERVER_MODE

/* 포트 번호 고정 */
#define PORT 5100

/* 라즈베리파이 주소 */
#define PI_ADDR "127.0.0.1"

/* USER : ID/PW */
#define ID_SIZE 21 // 20글자 + '\0'

/* 버퍼 사이즈 고정*/
#define BUFFER_SIZE 1024

#endif