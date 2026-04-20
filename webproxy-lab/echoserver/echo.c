#include "../csapp.h"
#include "echo.h"

void echo(int connfd)
{
    /* n은 한 번 읽을 때 실제로 읽은 바이트 수를 저장한다. */
    size_t n;
    /* buf는 클라이언트에게서 받은 한 줄 데이터를 저장하는 버퍼다. */
    char buf[MAXLINE];
    /* rio는 Robust I/O 패키지가 사용하는 내부 상태 구조체다. */
    rio_t rio;

    /* 연결 소켓 connfd를 rio 버퍼 구조체와 연결해 초기화한다. */
    Rio_readinitb(&rio, connfd);

    /* 한 줄씩 읽어서 더 이상 읽을 데이터가 없을 때까지 반복한다. */
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        /* 서버가 몇 바이트를 받았는지 로그처럼 출력한다. */
        printf("server received %zu bytes\n", n);
        /* 받은 데이터를 그대로 다시 클라이언트에게 돌려보낸다. */
        Rio_writen(connfd, buf, n);
    }
}
