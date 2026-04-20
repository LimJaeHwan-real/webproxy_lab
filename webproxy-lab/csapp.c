/*
 * csapp.c - Functions for the CS:APP3e book
 *
 * Updated 10/2016 reb:
 *   - Fixed bug in sio_ltoa that didn't cover negative numbers
 *
 * Updated 2/2016 droh:
 *   - Updated open_clientfd and open_listenfd to fail more gracefully
 *
 * Updated 8/2014 droh:
 *   - New versions of open_clientfd and open_listenfd are reentrant and
 *     protocol independent.
 *
 *   - Added protocol-independent inet_ntop and inet_pton functions. The
 *     inet_ntoa and inet_aton functions are obsolete.
 *
 * Updated 7/2014 droh:
 *   - Aded reentrant sio (signal-safe I/O) routines
 *
 * Updated 4/2013 droh:
 *   - rio_readlineb: fixed edge case bug
 *   - rio_readnb: removed redundant EINTR check
 */
/* $begin csapp.c */
#include "csapp.h"

/**************************
 * Error-handling functions
 **************************/
/* $begin errorfuns */
/* $begin unixerror */
void unix_error(char *msg) /* Unix-style error */
{
    /* errno에는 가장 최근에 실패한 시스템 호출의 이유가 들어 있다. */
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    /* 이 책의 래퍼 함수들은 실패하면 즉시 종료하는 방식으로 단순화되어 있다. */
    exit(0);
}
/* $end unixerror */

void posix_error(int code, char *msg) /* Posix-style error */
{
    /* pthread 계열 함수는 errno 대신 "반환값 자체"로 에러 코드를 주는 경우가 많다. */
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

void gai_error(int code, char *msg) /* Getaddrinfo-style error */
{
    /* getaddrinfo/getnameinfo는 전용 에러 문자열 함수(gai_strerror)를 써야 한다. */
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    exit(0);
}

void app_error(char *msg) /* Application error */
{
    /* 운영체제가 준 에러가 아니라 프로그램이 직접 만든 에러 메시지를 출력한다. */
    fprintf(stderr, "%s\n", msg);
    exit(0);
}
/* $end errorfuns */

void dns_error(char *msg) /* Obsolete gethostbyname error */
{
    /* 예전 DNS 함수용 에러 출력 함수다. */
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

/*********************************************
 * Wrappers for Unix process control functions
 ********************************************/

/* $begin forkwrapper */
pid_t Fork(void)
{
    pid_t pid;

    /* 현재 프로세스를 거의 똑같이 복제해 자식 프로세스를 하나 만든다. */
    if ((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}
/* $end forkwrapper */

void Execve(const char *filename, char *const argv[], char *const envp[])
{
    /* 현재 실행 중인 프로그램을 filename 프로그램으로 완전히 갈아끼운다. */
    if (execve(filename, argv, envp) < 0)
        unix_error("Execve error");
}

/* $begin wait */
pid_t Wait(int *status)
{
    pid_t pid;

    /* 종료된 자식 프로세스를 회수해 좀비 프로세스가 남지 않게 한다. */
    if ((pid = wait(status)) < 0)
        unix_error("Wait error");
    return pid;
}
/* $end wait */

pid_t Waitpid(pid_t pid, int *iptr, int options)
{
    pid_t retpid;

    /* 특정 자식 하나만 기다리거나, 옵션을 줘서 더 유연하게 상태를 확인할 수 있다. */
    if ((retpid = waitpid(pid, iptr, options)) < 0)
        unix_error("Waitpid error");
    return (retpid);
}

/* $begin kill */
void Kill(pid_t pid, int signum)
{
    int rc;

    /* 프로세스에 시그널을 보내 종료/중단/사용자 정의 처리를 유도한다. */
    if ((rc = kill(pid, signum)) < 0)
        unix_error("Kill error");
}
/* $end kill */

void Pause()
{
    /* 어떤 시그널이 올 때까지 현재 실행 흐름을 잠시 멈춘다. */
    (void)pause();
    return;
}

unsigned int Sleep(unsigned int secs)
{
    unsigned int rc;

    /* secs초 동안 잠든다. 도중에 시그널이 오면 일찍 깰 수 있다. */
    if ((rc = sleep(secs)) < 0)
        unix_error("Sleep error");
    return rc;
}

unsigned int Alarm(unsigned int seconds)
{
    /* 지정한 시간이 지나면 SIGALRM 시그널이 오도록 예약한다. */
    return alarm(seconds);
}

void Setpgid(pid_t pid, pid_t pgid)
{
    int rc;

    /* 프로세스를 특정 프로세스 그룹으로 묶는다. 셸의 job control과 관련 있다. */
    if ((rc = setpgid(pid, pgid)) < 0)
        unix_error("Setpgid error");
    return;
}

pid_t Getpgrp(void)
{
    /* 현재 프로세스가 속한 프로세스 그룹 id를 가져온다. */
    return getpgrp();
}

/************************************
 * Wrappers for Unix signal functions
 ***********************************/

/* $begin sigaction */
handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    /* signum 시그널이 왔을 때 handler 함수를 실행하도록 등록한다. */
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}
/* $end sigaction */

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    /* 시그널 집합을 잠시 막거나 원래 상태로 되돌릴 때 쓴다. */
    if (sigprocmask(how, set, oldset) < 0)
        unix_error("Sigprocmask error");
    return;
}

void Sigemptyset(sigset_t *set)
{
    /* 비어 있는 시그널 집합을 만든다. */
    if (sigemptyset(set) < 0)
        unix_error("Sigemptyset error");
    return;
}

void Sigfillset(sigset_t *set)
{
    /* 가능한 거의 모든 시그널을 집합에 넣는다. */
    if (sigfillset(set) < 0)
        unix_error("Sigfillset error");
    return;
}

void Sigaddset(sigset_t *set, int signum)
{
    /* 집합에 특정 시그널 하나를 추가한다. */
    if (sigaddset(set, signum) < 0)
        unix_error("Sigaddset error");
    return;
}

void Sigdelset(sigset_t *set, int signum)
{
    /* 집합에서 특정 시그널 하나를 제거한다. */
    if (sigdelset(set, signum) < 0)
        unix_error("Sigdelset error");
    return;
}

int Sigismember(const sigset_t *set, int signum)
{
    int rc;
    /* signum이 집합 안에 들어 있는지 검사한다. */
    if ((rc = sigismember(set, signum)) < 0)
        unix_error("Sigismember error");
    return rc;
}

int Sigsuspend(const sigset_t *set)
{
    /* 임시 시그널 마스크를 적용한 채 시그널이 올 때까지 잠든다. */
    int rc = sigsuspend(set); /* always returns -1 */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}

/*************************************************************
 * The Sio (Signal-safe I/O) package - simple reentrant output
 * functions that are safe for signal handlers.
 *************************************************************/

/* Private sio functions */

/* $begin sioprivate */
/* sio_reverse - Reverse a string (from K&R) */
static void sio_reverse(char s[])
{
    int c, i, j;

    /* 문자열 앞뒤를 바꿔서 뒤집는다. */
    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* sio_ltoa - Convert long to base b string (from K&R) */
static void sio_ltoa(long v, char s[], int b)
{
    int c, i = 0;
    /* 음수면 나중에 '-'를 붙이기 위해 먼저 기억해 둔다. */
    int neg = v < 0;

    if (neg)
        v = -v;

    do
    {
        /* 숫자를 한 자리씩 꺼내 문자로 바꿔 저장한다. */
        s[i++] = ((c = (v % b)) < 10) ? c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
        s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

/* sio_strlen - Return length of string (from K&R) */
static size_t sio_strlen(char s[])
{
    int i = 0;

    /* 문자열의 끝 표시인 '\0'를 만날 때까지 길이를 센다. */
    while (s[i] != '\0')
        ++i;
    return i;
}
/* $end sioprivate */

/* Public Sio functions */
/* $begin siopublic */

ssize_t sio_puts(char s[]) /* Put string */
{
    /* stdio 대신 write를 직접 사용해 시그널 핸들러 안에서도 비교적 안전하게 출력한다. */
    return write(STDOUT_FILENO, s, sio_strlen(s)); // line:csapp:siostrlen
}

ssize_t sio_putl(long v) /* Put long */
{
    char s[128];

    /* 숫자를 문자열로 바꾼 뒤, 문자열 출력 함수를 재사용한다. */
    sio_ltoa(v, s, 10); /* Based on K&R itoa() */ // line:csapp:sioltoa
    return sio_puts(s);
}

void sio_error(char s[]) /* Put error message and exit */
{
    sio_puts(s);
    /* signal handler 안에서는 exit보다 _exit가 더 안전하다. */
    _exit(1); // line:csapp:sioexit
}
/* $end siopublic */

/*******************************
 * Wrappers for the SIO routines
 ******************************/
ssize_t Sio_putl(long v)
{
    ssize_t n;

    /* 내부 함수 실패 시 직접 에러 출력 후 종료하는 래퍼다. */
    if ((n = sio_putl(v)) < 0)
        sio_error("Sio_putl error");
    return n;
}

ssize_t Sio_puts(char s[])
{
    ssize_t n;

    if ((n = sio_puts(s)) < 0)
        sio_error("Sio_puts error");
    return n;
}

void Sio_error(char s[])
{
    sio_error(s);
}

/********************************
 * Wrappers for Unix I/O routines
 ********************************/

int Open(const char *pathname, int flags, mode_t mode)
{
    int rc;

    /* 파일을 열고, 실패하면 -1을 넘기지 않고 바로 종료한다. */
    if ((rc = open(pathname, flags, mode)) < 0)
        unix_error("Open error");
    return rc;
}

ssize_t Read(int fd, void *buf, size_t count)
{
    ssize_t rc;

    /* 파일/소켓에서 최대 count바이트를 읽는다. */
    if ((rc = read(fd, buf, count)) < 0)
        unix_error("Read error");
    return rc;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
    ssize_t rc;

    /* 파일/소켓에 최대 count바이트를 쓴다. */
    if ((rc = write(fd, buf, count)) < 0)
        unix_error("Write error");
    return rc;
}

off_t Lseek(int fildes, off_t offset, int whence)
{
    off_t rc;

    /* 파일에서 읽고 쓰는 현재 위치를 앞이나 뒤로 이동시킨다. */
    if ((rc = lseek(fildes, offset, whence)) < 0)
        unix_error("Lseek error");
    return rc;
}

void Close(int fd)
{
    int rc;

    /* 열린 파일/소켓을 닫아 운영체제 자원을 반납한다. */
    if ((rc = close(fd)) < 0)
        unix_error("Close error");
}

int Select(int n, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout)
{
    int rc;

    /* 여러 fd 중 지금 읽기/쓰기 가능한 것이 생길 때까지 기다린다. */
    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
        unix_error("Select error");
    return rc;
}

int Dup2(int fd1, int fd2)
{
    int rc;

    /* fd2가 fd1과 같은 열린 파일을 가리키게 만든다. */
    if ((rc = dup2(fd1, fd2)) < 0)
        unix_error("Dup2 error");
    return rc;
}

void Stat(const char *filename, struct stat *buf)
{
    /* 파일 이름 기준으로 크기, 권한 같은 메타데이터를 읽어온다. */
    if (stat(filename, buf) < 0)
        unix_error("Stat error");
}

void Fstat(int fd, struct stat *buf)
{
    /* 이미 열린 fd 기준으로 파일 메타데이터를 읽어온다. */
    if (fstat(fd, buf) < 0)
        unix_error("Fstat error");
}

/*********************************
 * Wrappers for directory function
 *********************************/

DIR *Opendir(const char *name)
{
    /* 디렉터리를 읽기 위한 핸들을 연다. */
    DIR *dirp = opendir(name);

    if (!dirp)
        unix_error("opendir error");
    return dirp;
}

struct dirent *Readdir(DIR *dirp)
{
    struct dirent *dep;

    /* readdir는 "끝남"과 "실패"를 모두 NULL로 주므로 errno를 먼저 0으로 초기화한다. */
    errno = 0;
    dep = readdir(dirp);
    if ((dep == NULL) && (errno != 0))
        unix_error("readdir error");
    return dep;
}

int Closedir(DIR *dirp)
{
    int rc;

    /* 디렉터리 핸들을 닫는다. */
    if ((rc = closedir(dirp)) < 0)
        unix_error("closedir error");
    return rc;
}

/***************************************
 * Wrappers for memory mapping functions
 ***************************************/
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    void *ptr;

    /* 파일 내용을 메모리에 연결해 배열처럼 다룰 수 있게 한다. */
    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *)-1))
        unix_error("mmap error");
    return (ptr);
}

void Munmap(void *start, size_t length)
{
    /* mmap으로 연결한 메모리 매핑을 해제한다. */
    if (munmap(start, length) < 0)
        unix_error("munmap error");
}

/***************************************************
 * Wrappers for dynamic storage allocation functions
 ***************************************************/

void *Malloc(size_t size)
{
    void *p;

    /* 힙 영역에 size바이트를 동적으로 확보한다. */
    if ((p = malloc(size)) == NULL)
        unix_error("Malloc error");
    return p;
}

void *Realloc(void *ptr, size_t size)
{
    void *p;

    /* 기존 메모리 크기를 바꾸고 필요하면 새 위치로 옮긴다. */
    if ((p = realloc(ptr, size)) == NULL)
        unix_error("Realloc error");
    return p;
}

void *Calloc(size_t nmemb, size_t size)
{
    void *p;

    /* nmemb개 원소 공간을 만들고, 그 안을 0으로 초기화한다. */
    if ((p = calloc(nmemb, size)) == NULL)
        unix_error("Calloc error");
    return p;
}

void Free(void *ptr)
{
    /* malloc/calloc/realloc으로 받은 메모리를 운영체제에 돌려준다. */
    free(ptr);
}

/******************************************
 * Wrappers for the Standard I/O functions.
 ******************************************/
void Fclose(FILE *fp)
{
    /* FILE* 스트림을 닫고 내부 버퍼도 함께 정리한다. */
    if (fclose(fp) != 0)
        unix_error("Fclose error");
}

FILE *Fdopen(int fd, const char *type)
{
    FILE *fp;

    /* 숫자 fd를 FILE* 형태의 표준 I/O 스트림으로 감싼다. */
    if ((fp = fdopen(fd, type)) == NULL)
        unix_error("Fdopen error");

    return fp;
}

char *Fgets(char *ptr, int n, FILE *stream)
{
    char *rptr;

    /* 한 줄을 읽는다. EOF는 정상일 수 있으니 ferror로 진짜 실패만 구분한다. */
    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
        app_error("Fgets error");

    return rptr;
}

FILE *Fopen(const char *filename, const char *mode)
{
    FILE *fp;

    /* fopen으로 파일을 열고 실패 시 바로 종료한다. */
    if ((fp = fopen(filename, mode)) == NULL)
        unix_error("Fopen error");

    return fp;
}

void Fputs(const char *ptr, FILE *stream)
{
    /* 문자열을 FILE* 스트림에 쓴다. */
    if (fputs(ptr, stream) == EOF)
        unix_error("Fputs error");
}

size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t n;

    /* 최대 nmemb개 항목을 읽는다. 끝에 일찍 도달한 것과 실제 오류를 구분한다. */
    if (((n = fread(ptr, size, nmemb, stream)) < nmemb) && ferror(stream))
        unix_error("Fread error");
    return n;
}

void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    /* 요청한 개수만큼 모두 못 쓰면 오류로 처리한다. */
    if (fwrite(ptr, size, nmemb, stream) < nmemb)
        unix_error("Fwrite error");
}

/****************************
 * Sockets interface wrappers
 ****************************/

int Socket(int domain, int type, int protocol)
{
    int rc;

    /* 네트워크 통신을 위한 소켓 끝점(endpoint)을 하나 만든다. */
    if ((rc = socket(domain, type, protocol)) < 0)
        unix_error("Socket error");
    return rc;
}

void Setsockopt(int s, int level, int optname, const void *optval, int optlen)
{
    int rc;

    /* 소켓 동작 방식을 바꾼다. 예: 포트 재사용 허용, 버퍼 크기 조절 */
    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
        unix_error("Setsockopt error");
}

void Bind(int sockfd, struct sockaddr *my_addr, int addrlen)
{
    int rc;

    /* 이 소켓이 어떤 IP와 포트를 사용할지 운영체제에 등록한다. */
    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
        unix_error("Bind error");
}

void Listen(int s, int backlog)
{
    int rc;

    /* 서버 소켓을 "연결 요청을 기다리는 상태"로 바꾼다. */
    if ((rc = listen(s, backlog)) < 0)
        unix_error("Listen error");
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    /* 대기열에 들어온 클라이언트 연결 하나를 꺼내 새 연결 소켓을 만든다. */
    if ((rc = accept(s, addr, addrlen)) < 0)
        unix_error("Accept error");
    return rc;
}

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen)
{
    int rc;

    /* 클라이언트 소켓을 서버 주소로 실제 연결한다. */
    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
        unix_error("Connect error");
}

/*******************************
 * Protocol-independent wrappers
 *******************************/
/* $begin getaddrinfo */
void Getaddrinfo(const char *node, const char *service,
                 const struct addrinfo *hints, struct addrinfo **res)
{
    int rc;

    /* 호스트명과 포트를 실제 연결 가능한 주소 구조체 목록으로 바꿔 준다. */
    if ((rc = getaddrinfo(node, service, hints, res)) != 0)
        gai_error(rc, "Getaddrinfo error");
}
/* $end getaddrinfo */

void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
                 size_t hostlen, char *serv, size_t servlen, int flags)
{
    int rc;

    /* sockaddr를 사람이 읽기 쉬운 host/port 문자열로 바꾼다. */
    if ((rc = getnameinfo(sa, salen, host, hostlen, serv,
                          servlen, flags)) != 0)
        gai_error(rc, "Getnameinfo error");
}

void Freeaddrinfo(struct addrinfo *res)
{
    /* getaddrinfo가 동적으로 만든 주소 리스트를 한 번에 해제한다. */
    freeaddrinfo(res);
}

void Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    /* 바이너리 형태의 IP 주소를 "127.0.0.1" 같은 문자열로 바꾼다. */
    if (!inet_ntop(af, src, dst, size))
        unix_error("Inet_ntop error");
}

void Inet_pton(int af, const char *src, void *dst)
{
    int rc;

    /* 문자열 IP 주소를 네트워크에서 쓰는 이진 형식으로 바꾼다. */
    rc = inet_pton(af, src, dst);
    if (rc == 0)
        app_error("inet_pton error: invalid dotted-decimal address");
    else if (rc < 0)
        unix_error("Inet_pton error");
}

/*******************************************
 * DNS interface wrappers.
 *
 * NOTE: These are obsolete because they are not thread safe. Use
 * getaddrinfo and getnameinfo instead
 ***********************************/

/* $begin gethostbyname */
struct hostent *Gethostbyname(const char *name)
{
    struct hostent *p;

    /* 예전 DNS 조회 함수다. thread-safe가 아니라서 요즘은 권장되지 않는다. */
    if ((p = gethostbyname(name)) == NULL)
        dns_error("Gethostbyname error");
    return p;
}
/* $end gethostbyname */

struct hostent *Gethostbyaddr(const char *addr, int len, int type)
{
    struct hostent *p;

    /* IP 주소를 이용해 호스트 이름을 얻는 예전 방식 함수다. */
    if ((p = gethostbyaddr(addr, len, type)) == NULL)
        dns_error("Gethostbyaddr error");
    return p;
}

/************************************************
 * Wrappers for Pthreads thread control functions
 ************************************************/

void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp,
                    void *(*routine)(void *), void *argp)
{
    int rc;

    /* 새 스레드를 만들어 routine(argp)를 동시에 실행시킨다. */
    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
        posix_error(rc, "Pthread_create error");
}

void Pthread_cancel(pthread_t tid)
{
    int rc;

    /* 다른 스레드에 "취소 요청"을 보낸다. 바로 끝나지 않을 수도 있다. */
    if ((rc = pthread_cancel(tid)) != 0)
        posix_error(rc, "Pthread_cancel error");
}

void Pthread_join(pthread_t tid, void **thread_return)
{
    int rc;

    /* 대상 스레드가 끝날 때까지 기다리고, 끝난 결과도 받아온다. */
    if ((rc = pthread_join(tid, thread_return)) != 0)
        posix_error(rc, "Pthread_join error");
}

/* $begin detach */
void Pthread_detach(pthread_t tid)
{
    int rc;

    /* join 없이도 종료 자원이 자동 회수되도록 스레드를 분리 상태로 만든다. */
    if ((rc = pthread_detach(tid)) != 0)
        posix_error(rc, "Pthread_detach error");
}
/* $end detach */

void Pthread_exit(void *retval)
{
    /* 현재 스레드만 종료한다. 프로세스 전체 종료와는 다르다. */
    pthread_exit(retval);
}

pthread_t Pthread_self(void)
{
    /* 현재 실행 중인 스레드의 id를 돌려준다. */
    return pthread_self();
}

void Pthread_once(pthread_once_t *once_control, void (*init_function)())
{
    /* 여러 스레드가 와도 init_function이 딱 한 번만 실행되게 보장한다. */
    pthread_once(once_control, init_function);
}

/*******************************
 * Wrappers for Posix semaphores
 *******************************/

void Sem_init(sem_t *sem, int pshared, unsigned int value)
{
    /* 세마포어를 초기화한다. 세마포어는 "허가증 개수"처럼 생각하면 쉽다. */
    if (sem_init(sem, pshared, value) < 0)
        unix_error("Sem_init error");
}

void P(sem_t *sem)
{
    /* 허가증 하나를 가져간다. 없으면 여기서 기다린다. */
    if (sem_wait(sem) < 0)
        unix_error("P error");
}

void V(sem_t *sem)
{
    /* 허가증 하나를 반납한다. 기다리던 스레드가 깨어날 수 있다. */
    if (sem_post(sem) < 0)
        unix_error("V error");
}

/****************************************
 * The Rio package - Robust I/O functions
 ****************************************/

/*
 * rio_readn - Robustly read n bytes (unbuffered)
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    /* 아직 읽지 못한 바이트 수를 추적한다. */
    size_t nleft = n;
    ssize_t nread;
    /* bufp는 사용자 버퍼 안에서 다음에 데이터를 채울 위치다. */
    char *bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            /* 시그널 때문에 잠깐 끊긴 경우는 실패로 보지 않고 다시 시도한다. */
            if (errno == EINTR) /* Interrupted by sig handler return */
                nread = 0;      /* and call read() again */
            else
                return -1; /* errno set by read() */
        }
        else if (nread == 0)
            /* 더 읽을 데이터가 없다는 뜻이다. 파일이면 EOF, 소켓이면 연결 종료일 수 있다. */
            break; /* EOF */
        /* 이번에 읽은 만큼 남은 양을 줄이고, 버퍼 포인터를 앞으로 이동한다. */
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft); /* Return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - Robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    /* bufp는 다음에 쓸 데이터를 가리킨다. */
    char *bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nwritten = write(fd, bufp, nleft)) <= 0)
        {
            /* write도 시그널 때문에 중간에 끊길 수 있으므로 다시 시도한다. */
            if (errno == EINTR) /* Interrupted by sig handler return */
                nwritten = 0;   /* and call write() again */
            else
                return -1; /* errno set by write() */
        }
        /* 일부만 써졌더라도 남은 부분을 계속 이어서 쓴다. */
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */

/*
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0)
    { /* Refill if buf is empty */
        /* 내부 버퍼가 비었으면 커널에서 데이터를 한 번에 읽어 와 버퍼를 채운다. */
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
                           sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0)
        {
            if (errno != EINTR) /* Interrupted by sig handler return */
                return -1;
        }
        else if (rp->rio_cnt == 0) /* EOF */
            return 0;
        else
            /* 이제 버퍼 맨 앞부터 읽기 시작하면 된다. */
            rp->rio_bufptr = rp->rio_buf; /* Reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    /* 사용자가 요청한 양과 내부 버퍼에 남은 양 중 더 작은 값만 꺼낸다. */
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    /* 내부 버퍼에서 사용자 버퍼로 실제 바이트를 복사한다. */
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    /* 다음 읽기를 위해 포인터와 남은 개수를 갱신한다. */
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd)
{
    /* 이 구조체가 어느 fd를 읽을지 기록하고 버퍼 상태를 초기화한다. */
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}
/* $end rio_readinitb */

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0)
    {
        /* 내부 버퍼를 활용해 안정적으로 여러 바이트를 읽는다. */
        if ((nread = rio_read(rp, bufp, nleft)) < 0)
            return -1; /* errno set by read() */
        else if (nread == 0)
            break; /* EOF */
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft); /* return >= 0 */
}
/* $end rio_readnb */

/*
 * rio_readlineb - Robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    /* 한 줄 끝('\n')을 찾기 위해 문자 단위로 읽는다. */
    for (n = 1; n < maxlen; n++)
    {
        if ((rc = rio_read(rp, &c, 1)) == 1)
        {
            *bufp++ = c;
            if (c == '\n')
            {
                /* 줄바꿈 문자를 만나면 한 줄을 다 읽은 것이다. */
                n++;
                break;
            }
        }
        else if (rc == 0)
        {
            if (n == 1)
                return 0; /* EOF, no data read */
            else
                break; /* EOF, some data was read */
        }
        else
            return -1; /* Error */
    }
    /* C 문자열로 쓰기 위해 마지막에 '\0'를 붙인다. */
    *bufp = 0;
    return n - 1;
}
/* $end rio_readlineb */

/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    /* 소문자 rio_*는 에러를 반환하고, 대문자 Rio_*는 실패 시 종료한다. */
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
        unix_error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n)
{
    /* 정확히 n바이트를 다 못 쓰면 오류로 본다. */
    if (rio_writen(fd, usrbuf, n) != n)
        unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    /* 내부 초기화 함수를 그대로 감싼 래퍼다. */
    rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
        unix_error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
        unix_error("Rio_readlineb error");
    return rc;
}

/********************************
 * Client/server helper functions
 ********************************/
/*
 * open_clientfd - Open connection to server at <hostname, port> and
 *     return a socket descriptor ready for reading and writing. This
 *     function is reentrant and protocol-independent.
 *
 *     On error, returns:
 *       -2 for getaddrinfo error
 *       -1 with errno set for other errors.
 */
/* $begin open_clientfd */
int open_clientfd(char *hostname, char *port)
{
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses */
    /* hints를 0으로 채워서 쓰지 않는 필드가 쓰레기값을 갖지 않게 한다. */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV; /* service 인자를 숫자 포트로 해석하라고 힌트를 준다. */
    hints.ai_flags |= AI_ADDRCONFIG; /* 현재 머신에서 실제 사용 가능한 주소 계열 위주로 후보를 준다. */
    /* 예를 들어 google.com은 여러 IP 후보를 가질 수 있어서 목록 형태로 돌아온다. */
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) /* 연결 시도 후보 목록을 받아온다. */
    {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next)
    {
        /* 후보 주소를 하나씩 보면서 "실제로 연결 가능한지" 시험해 본다. */
        /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* Socket failed, try the next */

        /* Connect to the server */
        /* connect가 성공하면 이 clientfd는 바로 서버와 통신 가능한 소켓이 된다. */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break; /* Success */
        if (close(clientfd) < 0)
        { /* Connect failed, try another */ // line:netp:openclientfd:closefd
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    /* Clean up */
    /* getaddrinfo가 만들어 준 후보 주소 리스트는 더 이상 필요 없으니 해제한다. */
    freeaddrinfo(listp);
    if (!p) /* All connects failed */
        return -1;
    else /* The last connect succeeded */
        return clientfd;
}
/* $end open_clientfd */

/*
 * open_listenfd - Open and return a listening socket on port. This
 *     function is reentrant and protocol-independent.
 *
 *     On error, returns:
 *       -2 for getaddrinfo error
 *       -1 with errno set for other errors.
 */
/* $begin open_listenfd */
int open_listenfd(char *port)
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval = 1;

    /* Get a list of potential server addresses */
    /* 서버가 바인드할 수 있는 주소 후보를 얻기 위한 조건을 준비한다. */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* Accept connections */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV;            /* ... using port number */
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0)
    {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next)
    {
        /* 주소 후보를 하나씩 시도해서 실제로 bind 가능한 것을 찾는다. */
        /* Create a socket descriptor */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* Socket failed, try the next */

        /* Eliminates "Address already in use" error from bind */
        /* 서버를 바로 재시작할 때 같은 포트를 빨리 다시 쓰도록 도와주는 옵션이다. */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, // line:netp:csapp:setsockopt
                   (const void *)&optval, sizeof(int));

        /* Bind the descriptor to the address */
        /* bind가 성공하면 이 소켓은 해당 IP/포트의 "문" 역할을 맡는다. */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* Success */
        if (close(listenfd) < 0)
        { /* Bind failed, try the next */
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    /* Clean up */
    /* 주소 후보 리스트는 역할을 다 했으니 메모리를 반납한다. */
    freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
    {
        /* listen까지 실패하면 이 소켓은 더 쓸 수 없으니 닫는다. */
        close(listenfd);
        return -1;
    }
    return listenfd;
}
/* $end open_listenfd */

/****************************************************
 * Wrappers for reentrant protocol-independent helpers
 ****************************************************/
int Open_clientfd(char *hostname, char *port)
{
    int rc;

    /* open_clientfd 실패를 직접 처리하지 않게 해 주는 종료형 래퍼다. */
    if ((rc = open_clientfd(hostname, port)) < 0)
        unix_error("Open_clientfd error");
    return rc;
}

int Open_listenfd(char *port)
{
    int rc;

    /* open_listenfd 실패를 직접 처리하지 않게 해 주는 종료형 래퍼다. */
    if ((rc = open_listenfd(port)) < 0)
        unix_error("Open_listenfd error");
    return rc;
}

/* $end csapp.c */
