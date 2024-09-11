#include "Macros.h"

#ifdef CLIENT_MODE
#include "Client.h"
int main()
{
    C_init();           // 클라이언트 초기화
    C_Connect();        // 서버 연결
    C_ClientService();  // 서비스
    C_Close();          // 서버 연결 종료
    return 0;
}
#endif

#ifdef SERVER_MODE
#include "Server.h"
int main()
{
    S_Init();           // 서버 초기화
    S_ServerService();  // 서버 서비스
    S_Close();          // 서버 종료
    return 0;
}
#endif