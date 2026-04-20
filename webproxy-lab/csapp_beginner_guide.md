# `csapp.c` 비전공자용 해설

이 문서는 [`csapp.c`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:1) 를 처음 읽는 사람을 위해 만든 해설판입니다.

원본 파일은 1000줄이 넘고, 같은 모양의 "wrapper 함수"가 아주 많이 반복됩니다. 그래서 이 가이드는:

- 같은 패턴은 한 번 자세히 설명하고
- 중요한 함수는 줄 흐름을 더 촘촘하게 풀어 쓰고
- 각 함수가 "왜 존재하는지"를 쉬운 말로 먼저 설명하는 방식으로 정리했습니다.

## 1. 이 파일이 하는 일

`csapp.c`는 운영체제 함수들을 "조금 더 쓰기 쉽게" 감싼 도우미 모음입니다.

예를 들어 원래 `read()`를 직접 쓰면:

- 실패했는지 매번 확인해야 하고
- 에러 메시지도 직접 출력해야 하고
- 종료할지 복구할지 직접 정해야 합니다.

그런데 `csapp.c`의 `Read()`는:

- 내부에서 `read()`를 호출하고
- 에러면 사람이 읽을 수 있는 메시지를 출력하고
- 프로그램을 종료합니다.

즉, `csapp.c`의 많은 함수는 다음 한 문장으로 요약됩니다.

`원래 시스템 함수를 호출하고, 실패하면 바로 에러를 보고하는 안전 포장지`

## 2. 가장 먼저 익혀야 할 공통 패턴

`csapp.c`에서 가장 자주 나오는 패턴은 아래와 같습니다.

```c
int Open(const char *pathname, int flags, mode_t mode)
{
    int rc;

    if ((rc = open(pathname, flags, mode)) < 0)
        unix_error("Open error");
    return rc;
}
```

한 줄씩 뜻을 풀면:

- `int Open(...)`
  - 우리가 직접 부를 wrapper 함수 이름입니다.
  - 원래 함수 `open()`과 이름이 비슷하지만 첫 글자를 대문자로 바꿔 구분합니다.
- `int rc;`
  - 반환값을 임시 저장할 변수입니다.
  - 여기서 `rc`는 보통 "return code"의 약자처럼 쓰입니다.
- `rc = open(...)`
  - 진짜 시스템 함수 `open()`을 호출합니다.
- `< 0`
  - Unix 계열 함수는 실패 시 `-1` 같은 음수를 자주 반환합니다.
- `unix_error("Open error");`
  - 에러 메시지를 출력하고 프로그램을 종료합니다.
- `return rc;`
  - 성공했으면 원래 시스템 함수가 준 결과를 그대로 돌려줍니다.

이 패턴을 이해하면 `Read`, `Write`, `Close`, `Socket`, `Bind`, `Accept`, `Connect` 같은 함수 대부분은 거의 같은 방식으로 읽을 수 있습니다.

## 3. 파일 맨 위: 에러 처리 함수들

관련 위치:
- [`csapp.c:25`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:25)

### `unix_error`

```c
void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
```

줄 의미:

- `fprintf(stderr, ...)`
  - 표준 출력이 아니라 표준 에러에 메시지를 출력합니다.
- `msg`
  - 우리가 붙인 설명 문자열입니다. 예: `"Open error"`
- `strerror(errno)`
  - 방금 실패한 시스템 호출의 실제 에러 이유를 사람이 읽는 문자열로 바꿉니다.
  - 예: `"Connection refused"`, `"No such file or directory"`
- `exit(0);`
  - 프로그램을 끝냅니다.

비슷한 함수:

- `posix_error`
  - `errno`가 아니라 전달받은 `code`를 사용합니다.
- `gai_error`
  - `getaddrinfo()` 계열 에러를 문자열로 바꿀 때 `gai_strerror()`를 씁니다.
- `app_error`
  - 시스템 에러 코드 없이, 단순 문자열만 출력합니다.
- `dns_error`
  - 오래된 DNS 함수용 에러 처리입니다.

핵심:

이 함수들은 "에러를 통일된 방식으로 보여주기 위한 공용 도우미"입니다.

## 4. 프로세스 제어 wrapper

관련 위치:
- [`Fork` 부터 `Getpgrp` 까지](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:69)

여기 함수들은 모두 "원래 시스템 함수 호출 + 실패 시 종료" 패턴입니다.

### `Fork`

```c
pid_t Fork(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}
```

줄 의미:

- `pid_t pid;`
  - 프로세스 ID를 저장할 변수입니다.
- `pid = fork()`
  - 현재 프로세스를 복제해 자식 프로세스를 만듭니다.
- `< 0`
  - 실패면 음수입니다.
- `return pid;`
  - 부모에서는 자식 PID가 오고, 자식에서는 `0`이 옵니다.

### 나머지 함수 읽는 법

- `Execve`
  - 현재 프로세스 내용을 새 프로그램으로 갈아끼웁니다.
- `Wait`
  - 자식 프로세스 하나가 끝날 때까지 기다립니다.
- `Waitpid`
  - 특정 자식 또는 조건에 맞는 자식을 기다립니다.
- `Kill`
  - 프로세스에 시그널을 보냅니다.
- `Pause`
  - 시그널이 올 때까지 잠깁니다.
- `Sleep`
  - 정해진 초만큼 쉽니다.
- `Alarm`
  - 몇 초 뒤 시그널을 보내도록 예약합니다.
- `Setpgid`
  - 프로세스 그룹을 설정합니다.
- `Getpgrp`
  - 현재 프로세스 그룹 ID를 가져옵니다.

## 5. 시그널 wrapper

관련 위치:
- [`Signal`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:150)

### `Signal`

```c
handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}
```

줄 의미:

- `struct sigaction action, old_action;`
  - 새 설정과 이전 설정을 담는 구조체 두 개를 준비합니다.
- `action.sa_handler = handler;`
  - 이 시그널이 오면 실행할 함수를 등록합니다.
- `sigemptyset(&action.sa_mask);`
  - handler가 실행되는 동안 추가로 막을 시그널 목록을 비웁니다.
- `action.sa_flags = SA_RESTART;`
  - 시그널 때문에 끊긴 일부 시스템 호출을 자동 재시작하도록 요청합니다.
- `sigaction(...)`
  - 실제 커널에 시그널 처리 방식을 등록합니다.
- `return (old_action.sa_handler);`
  - 예전 handler를 돌려줍니다.

나머지 시그널 함수는 이름 그대로입니다.

- `Sigprocmask`
  - 어떤 시그널을 잠시 막을지 설정
- `Sigemptyset`
  - 시그널 집합 비우기
- `Sigfillset`
  - 시그널 집합을 모두 채우기
- `Sigaddset`
  - 집합에 시그널 추가
- `Sigdelset`
  - 집합에서 시그널 제거
- `Sigismember`
  - 어떤 시그널이 집합에 들어 있는지 확인
- `Sigsuspend`
  - 특정 마스크 상태로 잠깐 대기

### `Sigsuspend`가 특별한 이유

```c
int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set);
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}
```

`sigsuspend()`는 정상 동작해도 보통 시그널에 의해 깨어나면서 `-1`을 반환합니다.
그래서 이 함수는 "실패냐 아니냐"를 단순히 반환값만 보고 판단하지 않고, `errno == EINTR`인지 같이 봅니다.

## 6. Sio: 시그널 핸들러에서도 비교적 안전한 출력 함수

관련 위치:
- [`sio_reverse`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:224)
- [`sio_puts`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:267)

이 부분은 "시그널 핸들러 안에서도 쓸 수 있는 단순 출력"을 만들기 위한 코드입니다.

### `sio_reverse`

```c
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}
```

줄 의미:

- 문자열 앞뒤를 바꿔 가며 뒤집습니다.
- 숫자를 문자열로 바꿀 때 뒤집힌 순서로 만들어지기 때문에 마지막에 필요합니다.

### `sio_ltoa`

이 함수는 `long` 정수를 문자열로 바꿉니다.

중요 줄:

- `int neg = v < 0;`
  - 음수인지 먼저 기억합니다.
- `v = -v;`
  - 계산을 쉽게 하려고 양수로 바꿉니다.
- `v % b`
  - 현재 자리 숫자를 뽑습니다.
- `s[i++] = ...`
  - 문자 하나를 버퍼에 저장합니다.
- `v /= b`
  - 다음 자리로 넘어갑니다.
- `s[i] = '\0';`
  - 문자열 끝 표시를 넣습니다.
- `sio_reverse(s);`
  - 거꾸로 쌓였으므로 다시 뒤집습니다.

### `sio_puts`

```c
ssize_t sio_puts(char s[])
{
    return write(STDOUT_FILENO, s, sio_strlen(s));
}
```

한 줄 핵심:

- `printf` 대신 `write`를 써서 더 단순하고 안전하게 출력합니다.

### `sio_error`

```c
void sio_error(char s[])
{
    sio_puts(s);
    _exit(1);
}
```

핵심:

- `exit()` 대신 `_exit()`를 써서 시그널 처리 문맥에서 더 단순하게 바로 종료합니다.

## 7. Unix I/O wrapper

관련 위치:
- [`Open` 부터 `Fstat` 까지](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:321)

이 부분은 거의 전부 같은 패턴입니다.

### 대표 예시: `Read`

```c
ssize_t Read(int fd, void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0)
        unix_error("Read error");
    return rc;
}
```

줄 의미:

- `fd`
  - 읽을 대상 파일 디스크립터
- `buf`
  - 읽은 데이터를 담을 메모리
- `count`
  - 최대 몇 바이트 읽을지
- `rc = read(...)`
  - 실제로 커널에게 읽기 요청
- `return rc`
  - 실제 읽은 바이트 수 반환

같은 방식으로 읽으면 됩니다.

- `Write`
  - 쓰기
- `Lseek`
  - 파일 위치 옮기기
- `Close`
  - 닫기
- `Select`
  - 여러 fd 상태 감시
- `Dup2`
  - fd 복제
- `Stat`
  - 파일 메타데이터 가져오기
- `Fstat`
  - 열린 fd 기준 메타데이터 가져오기

## 8. 디렉터리, 메모리 매핑, 메모리 할당, 표준 입출력 wrapper

관련 위치:
- [`Opendir`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:400)
- [`Mmap`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:432)
- [`Malloc`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:451)
- [`Fclose`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:486)

이 구간도 공통 철학은 같습니다.

- `Opendir`, `Readdir`, `Closedir`
  - 폴더 열기, 항목 하나 읽기, 닫기
- `Mmap`, `Munmap`
  - 파일이나 메모리를 프로세스 주소 공간에 직접 매핑
- `Malloc`, `Realloc`, `Calloc`, `Free`
  - 메모리 할당과 해제
- `Fopen`, `Fgets`, `Fputs`, `Fread`, `Fwrite`
  - `FILE *` 기반 표준 입출력 함수 wrapper

### `Readdir`가 조금 특별한 이유

```c
errno = 0;
dep = readdir(dirp);
if ((dep == NULL) && (errno != 0))
    unix_error("readdir error");
```

이렇게 하는 이유:

- `readdir()`는 "진짜 에러"일 때도 `NULL`
- "더 읽을 항목이 끝난 정상 상황"일 때도 `NULL`

이라서, 호출 전 `errno = 0`으로 비워 두고,
끝나고 `NULL`이면서 `errno != 0`이면 그때만 에러라고 판단합니다.

## 9. 소켓 wrapper

관련 위치:
- [`Socket` 부터 `Connect` 까지](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:547)

이 부분은 네트워크 프로그램에서 아주 자주 씁니다.

- `Socket`
  - 소켓 만들기
- `Setsockopt`
  - 소켓 옵션 설정
- `Bind`
  - 소켓에 주소 묶기
- `Listen`
  - 서버 소켓을 "연결 대기" 상태로 바꾸기
- `Accept`
  - 들어온 연결 하나 받기
- `Connect`
  - 클라이언트가 서버에 연결 요청하기

### `Connect`

```c
void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen)
{
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
        unix_error("Connect error");
}
```

뜻:

- `sockfd`
  - 이미 `socket()`으로 만든 소켓 번호
- `serv_addr`
  - 연결할 서버 주소
- `addrlen`
  - 주소 구조체 길이
- `connect(...)`
  - 실제 연결 요청
- 실패 시 바로 에러 출력 후 종료

## 10. 주소 해석 wrapper

관련 위치:
- [`Getaddrinfo`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:602)

요즘 네트워크 코드에서는 오래된 `gethostbyname()`보다 `getaddrinfo()`를 더 많이 씁니다.

### `Getaddrinfo`

```c
void Getaddrinfo(const char *node, const char *service,
                 const struct addrinfo *hints, struct addrinfo **res)
{
    int rc;

    if ((rc = getaddrinfo(node, service, hints, res)) != 0)
        gai_error(rc, "Getaddrinfo error");
}
```

핵심:

- `node`
  - 호스트 이름 또는 IP
- `service`
  - 포트 문자열 또는 서비스 이름
- `hints`
  - 원하는 주소 조건
- `res`
  - 결과 주소 목록

`getaddrinfo()`는 일반 Unix 함수처럼 `-1`이 아니라 `0` 성공, 그 외 에러 코드 방식이라 `gai_error()`를 씁니다.

### `Inet_pton`

```c
rc = inet_pton(af, src, dst);
if (rc == 0)
    app_error("inet_pton error: invalid dotted-decimal address");
else if (rc < 0)
    unix_error("Inet_pton error");
```

이 함수는 에러 상황이 둘로 나뉩니다.

- `rc == 0`
  - 문법상 잘못된 주소 문자열
- `rc < 0`
  - 시스템 호출 수준의 진짜 에러

## 11. 오래된 DNS wrapper

관련 위치:
- [`Gethostbyname`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:651)

이 함수들은 책 예제 호환용입니다.

- `Gethostbyname`
- `Gethostbyaddr`

문서 주석에도 적혀 있듯이:

- thread-safe 하지 않고
- 최신 코드에서는 `getaddrinfo()`가 더 좋습니다.

## 12. 스레드와 세마포어 wrapper

관련 위치:
- [`Pthread_create`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:675)
- [`Sem_init`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:728)

여기도 패턴은 거의 같습니다.

### `Pthread_create`

```c
if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
    posix_error(rc, "Pthread_create error");
```

뜻:

- 새 스레드를 만들고
- 실패하면 POSIX 에러 코드를 사람이 읽는 문자열로 바꿔 출력합니다.

### 세마포어 함수

- `Sem_init`
  - 세마포어 초기화
- `P`
  - 세마포어 값을 줄이며, 필요하면 기다림
- `V`
  - 세마포어 값을 늘리며, 기다리던 쪽을 깨울 수 있음

운영체제 수업에서는 보통:

- `P` = 자원 얻기 전 잠금
- `V` = 자원 사용 후 잠금 해제

처럼 이해하면 됩니다.

## 13. Rio: 이 파일에서 가장 중요한 부분 중 하나

관련 위치:
- [`rio_readn`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:754)
- [`rio_writen`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:782)
- [`rio_read`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:813)
- [`rio_readlineb`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:882)

Rio는 "Robust I/O"의 약자입니다.

왜 필요하냐면 일반 `read()`/`write()`는:

- 한 번에 원하는 만큼 다 안 읽힐 수 있고
- 시그널 때문에 중간에 끊길 수 있고
- 텍스트 줄 단위 읽기가 불편합니다.

Rio는 이런 문제를 줄여 줍니다.

### `rio_readn`: 딱 n바이트를 최대한 끝까지 읽기

```c
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
                nread = 0;
            else
                return -1;
        }
        else if (nread == 0)
            break;
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);
}
```

줄 의미:

- `nleft = n`
  - 아직 더 읽어야 하는 바이트 수
- `bufp = usrbuf`
  - 현재 어디에 써 넣을지 가리키는 포인터
- `while (nleft > 0)`
  - 목표량을 다 읽을 때까지 반복
- `read(fd, bufp, nleft)`
  - 남은 양만큼 읽어 봄
- `errno == EINTR`
  - 시그널 때문에 잠깐 끊겼다면 진짜 실패로 보지 않고 다시 시도
- `nread == 0`
  - EOF, 즉 더 읽을 데이터가 없음
- `nleft -= nread`
  - 남은 목표량 줄이기
- `bufp += nread`
  - 다음 기록 위치로 이동
- `return (n - nleft)`
  - 실제로 읽은 총량 반환

### `rio_writen`: 다 쓸 때까지 반복 쓰기

`rio_writen`은 `rio_readn`과 거의 같은 아이디어입니다.

- 한 번 `write()` 했다고 해서 요청한 바이트를 전부 다 쓰는 건 아닐 수 있으니
- 남은 양이 0이 될 때까지 계속 씁니다.

### `rio_read`: 내부 버퍼를 가진 진짜 핵심 함수

이 함수는 사용자에게 직접 많이 보이진 않지만 Rio의 심장입니다.

핵심 아이디어:

- 커널에서 한 번에 좀 크게 읽어 내부 버퍼에 저장
- 사용자가 요청할 때는 그 내부 버퍼에서 잘라서 전달
- 내부 버퍼가 비면 다시 `read()`

중요 줄:

- `while (rp->rio_cnt <= 0)`
  - 내부 버퍼에 읽지 않은 데이터가 없으면 새로 채워야 함
- `rp->rio_cnt = read(...)`
  - 커널에서 한 번 가져옴
- `rp->rio_bufptr = rp->rio_buf;`
  - 내부 버퍼의 시작점으로 포인터 초기화
- `cnt = n;`
  - 기본적으로 사용자가 원하는 양만큼 복사하려 함
- `if ((size_t)rp->rio_cnt < n) cnt = ...`
  - 내부 버퍼에 남은 양이 더 적으면 그만큼만 복사
- `memcpy(...)`
  - 내부 버퍼에서 사용자 버퍼로 복사

### `rio_readinitb`

```c
void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}
```

뜻:

- 이 `rio_t` 구조체가 어느 fd를 읽을지 등록
- 아직 내부 버퍼에 읽은 데이터는 없다고 표시
- 버퍼 포인터를 버퍼 시작으로 맞춤

### `rio_readnb`

이 함수는 "버퍼 기반으로 n바이트 읽기"입니다.

- `rio_read()`를 여러 번 호출하면서
- 원하는 양만큼 채우거나
- EOF를 만나면 멈춥니다.

### `rio_readlineb`

이 함수는 텍스트 줄 단위 읽기에서 아주 중요합니다.

```c
for (n = 1; n < maxlen; n++)
{
    if ((rc = rio_read(rp, &c, 1)) == 1)
    {
        *bufp++ = c;
        if (c == '\n')
        {
            n++;
            break;
        }
    }
    ...
}
*bufp = 0;
return (ssize_t)(n - 1);
```

줄 의미:

- 한 글자씩 읽습니다.
- 읽은 글자를 사용자 버퍼에 넣습니다.
- 줄바꿈 문자 `\n`을 만나면 한 줄이 끝났다고 보고 멈춥니다.
- 마지막에 `\0`를 붙여 C 문자열로 만듭니다.

왜 한 글자씩 읽냐면:

- "줄 끝"을 정확히 찾기 위해서입니다.
- 대신 진짜 성능 손해를 줄이려고 내부적으로는 `rio_read()`의 버퍼를 사용합니다.

## 14. Rio wrapper

관련 위치:
- [`Rio_readn` 부터 `Rio_readlineb` 까지](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:917)

이 부분은 아주 단순합니다.

- 소문자 `rio_*`
  - 실제 구현
- 대문자 `Rio_*`
  - 구현을 감싸고, 실패하면 즉시 종료하는 wrapper

예:

```c
ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
        unix_error("Rio_readnb error");
    return rc;
}
```

## 15. `open_clientfd`: 클라이언트 연결 도우미

관련 위치:
- [`open_clientfd`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:971)

이 함수는 "호스트 이름 + 포트"를 받아 서버에 연결되는 소켓을 만들어 줍니다.

### 줄 흐름 해설

```c
int open_clientfd(char *hostname, char *port)
{
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;
```

- `clientfd`
  - 최종적으로 반환할 소켓 번호
- `rc`
  - 각종 함수 반환값 저장
- `hints`
  - `getaddrinfo()`에게 줄 조건
- `listp`
  - 주소 후보 목록의 시작점
- `p`
  - 목록을 순회할 때 쓰는 현재 포인터

```c
memset(&hints, 0, sizeof(struct addrinfo));
```

- `hints` 구조체를 0으로 초기화합니다.
- 쓰레기값이 남아 있으면 안 되기 때문입니다.

```c
hints.ai_socktype = SOCK_STREAM;
```

- TCP 연결용 소켓을 원한다고 지정합니다.

```c
hints.ai_flags = AI_NUMERICSERV;
```

- 포트는 `"80"` 같은 숫자 문자열로 처리하겠다는 뜻입니다.

```c
hints.ai_flags |= AI_ADDRCONFIG;
```

- 현재 컴퓨터 환경에 맞는 주소 계열만 추천받겠다는 뜻입니다.

```c
if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0)
```

- `hostname`, `port`에 맞는 주소 후보들을 한꺼번에 받아옵니다.

```c
for (p = listp; p; p = p->ai_next)
```

- 후보 주소들을 하나씩 시험해 봅니다.

```c
if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
    continue;
```

- 현재 후보 주소에 맞는 소켓을 만듭니다.
- 실패하면 다음 후보로 넘어갑니다.

```c
if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
    break;
```

- 연결에 성공하면 반복을 끝냅니다.

```c
if (close(clientfd) < 0)
```

- 연결 실패한 소켓은 닫고 다음 후보를 시험합니다.

```c
freeaddrinfo(listp);
```

- 주소 목록 메모리를 정리합니다.

```c
if (!p)
    return -1;
else
    return clientfd;
```

- 후보를 끝까지 돌았는데 성공한 주소가 없으면 실패
- 성공한 후보가 있으면 그 소켓 번호 반환

## 16. `open_listenfd`: 서버 리스닝 소켓 도우미

관련 위치:
- [`open_listenfd`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:1022)

이 함수는 서버 쪽에서 "지정 포트로 접속을 기다리는 소켓"을 만들어 줍니다.

### 줄 흐름 해설

```c
struct addrinfo hints, *listp, *p;
int listenfd, rc, optval = 1;
```

- `listenfd`
  - 최종 리스닝 소켓 번호
- `optval = 1`
  - `setsockopt()`에 줄 옵션 값

```c
memset(&hints, 0, sizeof(struct addrinfo));
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
hints.ai_flags |= AI_NUMERICSERV;
```

뜻:

- TCP 서버 소켓을 만들고
- 특정 원격 호스트가 아니라 "내 컴퓨터의 사용 가능한 주소"에 바인드하고
- 포트는 숫자로 받겠다는 설정입니다.

```c
if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0)
```

- 서버는 자기 자신 주소에 바인드하면 되므로 호스트 이름 대신 `NULL`
- 대신 포트 번호만으로 바인드 가능한 주소 후보 목록을 얻습니다.

```c
for (p = listp; p; p = p->ai_next)
```

- 각 주소 후보를 하나씩 시험합니다.

```c
if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
    continue;
```

- 후보 주소 형식에 맞는 소켓 생성

```c
setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
           (const void *)&optval, sizeof(int));
```

- 서버를 재시작할 때 "Address already in use" 문제가 덜 나게 하는 대표 옵션입니다.

```c
if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
    break;
```

- 주소 묶기에 성공하면 그 후보를 사용합니다.

```c
if (close(listenfd) < 0)
```

- 바인드 실패한 소켓은 닫고 다음 후보로 이동

```c
if (listen(listenfd, LISTENQ) < 0)
{
    close(listenfd);
    return -1;
}
```

- 바인드에 성공한 소켓을 "이제 연결을 기다리는 소켓"으로 바꿉니다.

## 17. 마지막 wrapper 두 개

관련 위치:
- [`Open_clientfd`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:1077)
- [`Open_listenfd`](/workspaces/webproxy_lab_docker/webproxy-lab/csapp.c:1086)

이 둘은:

- 소문자 helper 함수는 에러 코드를 반환하고
- 대문자 wrapper 함수는 실패 시 바로 종료하게 만든 버전입니다.

예:

```c
if ((rc = open_clientfd(hostname, port)) < 0)
    unix_error("Open_clientfd error");
```

## 18. 이 파일을 읽는 가장 좋은 순서

처음 읽는다면 이 순서를 추천합니다.

1. `unix_error`, `app_error`
2. `Open`, `Read`, `Write`, `Close`
3. `Socket`, `Bind`, `Listen`, `Accept`, `Connect`
4. `rio_readn`, `rio_writen`, `rio_read`, `rio_readlineb`
5. `open_clientfd`, `open_listenfd`
6. 나머지 wrapper 함수들

이 순서가 좋은 이유:

- 먼저 "wrapper의 공통 구조"를 익히고
- 다음에 실제 네트워크 실습에서 많이 쓰는 부분을 이해하면
- 나머지 함수는 훨씬 쉽게 읽히기 때문입니다.

## 19. 한 줄 요약

`csapp.c`는 새로운 알고리즘 덩어리라기보다, 운영체제/네트워크 함수를 실패 처리까지 포함해 안전하고 편하게 쓰도록 감싼 실습용 도구 상자입니다.

정말 중요하게 봐야 할 부분은:

- `Signal`
- `rio_*`
- `open_clientfd`
- `open_listenfd`

입니다.

원하시면 다음 단계로 이어서:

- `csapp.c`를 200줄씩 잘라서 진짜 한 줄마다 번호 붙여 설명하거나
- `webproxy-lab/csapp.c` 자체 옆에 주석판 `csapp_annotated.c`를 따로 만들어 드릴 수 있습니다.
