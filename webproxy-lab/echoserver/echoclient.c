#include "../csapp.h"

int main(int argc, char **argv)
{
    /* clientfd는 서버와 연결된 클라이언트 소켓 디스크립터다. */
    int clientfd;
    /* buf는 사용자가 입력한 문자열과 서버가 돌려준 문자열을 담는 버퍼다. */
    char buf[MAXLINE];
    /* host는 접속할 서버 호스트 이름 또는 IP 주소를 가리킨다. */
    char *host;
    /* port는 접속할 서버 포트 번호 문자열을 가리킨다. */
    char *port;
    /* rio는 소켓에서 줄 단위로 안정적으로 읽기 위한 버퍼 구조체다. */
    rio_t rio;

    /* 실행 인자가 호스트와 포트까지 총 3개인지 확인한다. */
    if (argc != 3) {
        /* 사용법이 틀렸다면 표준 에러로 실행 형식을 출력한다. */
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        /* 잘못된 사용이므로 프로그램을 종료한다. */
        exit(0);
    }

    /* 첫 번째 인자를 접속 대상 호스트로 사용한다. */
    host = argv[1];
    /* 두 번째 인자를 접속 대상 포트로 사용한다. */
    port = argv[2];

    /* 서버(host, port)에 TCP 연결을 열고 연결 소켓을 얻는다. */
    clientfd = Open_clientfd(host, port);

    /* 연결된 소켓을 rio 버퍼와 연결해서 줄 단위 읽기를 준비한다. */
    Rio_readinitb(&rio, clientfd);

    /* 표준 입력에서 한 줄씩 읽어서 서버로 보내고 응답을 받는다. */
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        /* 사용자가 입력한 문자열 전체를 서버로 전송한다. */
        Rio_writen(clientfd, buf, strlen(buf));

        /* 서버가 돌려준 한 줄을 읽는다. */
        if (Rio_readlineb(&rio, buf, MAXLINE) == 0) {
            /* 서버가 예상보다 빨리 연결을 끊었다면 에러로 처리한다. */
            app_error("server closed connection unexpectedly");
        }

        /* 서버가 에코해서 돌려준 결과를 표준 출력에 그대로 출력한다. */
        Fputs(buf, stdout);
    }

    /* 입력이 끝났으면 서버와의 연결 소켓을 닫는다. */
    Close(clientfd);

    /* 정상 종료를 의미하는 0을 반환한다. */
    return 0;
}
