#include "../webproxy-lab-owned/csapp.c"
#include "echo.h"
/* 한 클라이언트와 에코 통신을 수행하는 함수를 미리 선언한다. */

int main(int argc, char **argv)
{
    /* listenfd는 서버가 연결을 기다리는 리스닝 소켓 디스크립터다. */
    int listenfd;
    /* connfd는 실제로 특정 클라이언트와 통신할 때 쓰는 연결 소켓 디스크립터다. */
    int connfd;
    /* clientlen은 클라이언트 주소 구조체의 크기를 저장하는 변수다. */
    socklen_t clientlen;
    /* clientaddr는 IPv4/IPv6 어느 쪽이 와도 담을 수 있는 범용 주소 구조체다. */
    struct sockaddr_storage clientaddr;
    /* client_hostname은 접속한 클라이언트의 호스트 이름 또는 주소 문자열을 저장한다. */
    char client_hostname[MAXLINE];
    /* client_port는 접속한 클라이언트의 포트 번호 문자열을 저장한다. */
    char client_port[MAXLINE];

    /* 인자가 정확히 1개 포트 번호인지 확인한다. */
    if (argc != 2)
    {
        /* 사용법이 틀렸다면 표준 에러로 올바른 실행 형식을 알려준다. */
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        /* 잘못된 실행이므로 프로그램을 종료한다. */
        exit(0);
    }

    /* 지정한 포트에서 연결을 받아들이는 리스닝 소켓을 연다. */
    listenfd = Open_listenfd(argv[1]);

    /* 서버는 계속 살아 있으면서 여러 클라이언트를 순차적으로 처리한다. */
    while (1)
    {
        /* accept 전에 주소 구조체 크기를 매번 정확히 넣어준다. */
        clientlen = sizeof(struct sockaddr_storage);

        /* 새 클라이언트 연결 요청을 받아 connfd를 얻는다. */
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        /* 접속한 클라이언트의 주소와 포트를 사람이 읽을 수 있는 문자열로 변환한다. */
        Getnameinfo((SA *)&clientaddr, clientlen,
                    client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);

        /* 어떤 클라이언트가 접속했는지 서버 화면에 출력한다. */
        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        /* 현재 연결된 클라이언트와 echo 서비스를 수행한다. */
        echo(connfd);

        /* 현재 클라이언트와의 통신이 끝났으므로 연결 소켓을 닫는다. */
        Close(connfd);
    }

    /* 무한 루프 서버라서 실제로 여기까지는 오지 않지만 형식상 0을 반환한다. */
    return 0;
}
