/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  // 클라이언트 소켓과 RIO 버퍼를 연결한다.
  // 이후에는 Rio_readlineb로 HTTP 요청을 한 줄씩 안전하게 읽을 수 있다.
  Rio_readinitb(&rio, fd);

  // HTTP 요청의 첫 줄(request line)을 읽는다.
  // 예: "GET /index.html HTTP/1.1\r\n"
  // 11.6(A) 요구사항에 맞게 이 줄도 그대로 출력해서
  // 브라우저가 실제로 어떤 방식으로 요청했는지 확인할 수 있게 한다.
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request line and headers:\n");
  printf("%s", buf);

  // 요청 라인에서 메서드, URI, HTTP 버전을 분리한다.
  // exercise 11.6(C)에서는 여기서 읽은 version 값을 보고
  // 브라우저가 HTTP/1.0을 쓰는지 HTTP/1.1을 쓰는지 판단할 수 있다.
  sscanf(buf, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  // Parse URI from GET request
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static) // static content
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read this file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  else
  { // dynamic content
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // Build the HTTP response body
  snprintf(body, MAXBUF, "<html><title>Tine Error</title>");
  snprintf(body + strlen(body), MAXBUF - strlen(body),
           "<body bgcolor="
           "ffffff"
           ">\r\n");
  snprintf(body + strlen(body), MAXBUF - strlen(body), "%s: %s\r\n", errnum, shortmsg);
  snprintf(body + strlen(body), MAXBUF - strlen(body), "<p>%s: %s\r\n", longmsg, cause);
  snprintf(body + strlen(body), MAXBUF - strlen(body), "<hr><em>The Tiny Web server</em>\r\n");

  // Print the HTTP response
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  // request line 다음에는 0개 이상의 request header가 이어진다.
  // 각 줄을 하나씩 읽어서 그대로 출력하면 exercise 11.6(A)의
  // "every request header를 echo" 요구사항을 만족할 수 있다.
  //
  // 헤더의 끝은 빈 줄("\r\n")로 표시되므로,
  // 빈 줄을 만나기 전까지 모든 헤더 줄을 정확히 한 번씩 출력한다.
  while (1)
  {
    Rio_readlineb(rp, buf, MAXLINE);
    if (!strcmp(buf, "\r\n"))
    {
      // 빈 줄은 "헤더가 여기서 끝났다"는 의미다.
      // 가독성을 위해 터미널에도 한 줄 띄워서 요청 블록의 끝을 보여준다.
      printf("\n");
      break;
    }

    // 실제 브라우저가 보낸 헤더를 수정하지 않고 그대로 출력한다.
    // 예: Host, User-Agent, Accept, Sec-Fetch-* 등
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  // strstr(문자열, 찾을 문자열)
  // 찾을 문자열이 처음 나타나는 위치의 주소 반환
  // 없으면 null 반환
  if (!strstr(uri, "cgi-bin")) // Static content
  {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  { // Dynamic content
    ptr = index(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  // Send response headers to client
  get_filetype(filename, filetype);
  snprintf(buf, MAXBUF, "HTTP/1.0 200 OK\r\n");
  snprintf(buf + strlen(buf), MAXBUF - strlen(buf), "Server: Tiney Web Server\r\n");
  snprintf(buf + strlen(buf), MAXBUF - strlen(buf), "Connection: close\r\n");
  snprintf(buf + strlen(buf), MAXBUF - strlen(buf), "Content-length: %d\r\n", filesize);
  snprintf(buf + strlen(buf), MAXBUF - strlen(buf), "Content-type: %s\r\n\r\n", filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  // Send reponse body to client
  srcfd = Open(filename, O_RDONLY, 0);
  // mmap addr null = 커널이 매핑할 가상 메모리 주소 알아서 정해라
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype)
{
  // 파일 이름에 포함된 확장자를 보고 HTTP Content-type을 결정한다.
  // 브라우저는 이 값을 보고 응답 본문을 HTML로 렌더링할지,
  // 이미지로 표시할지, 비디오로 재생할지를 판단한다.
  //
  // Tiny는 정적 파일을 그대로 읽어서 보내는 구조이므로,
  // 새로운 파일 형식을 지원할 때는 보통 여기에서 MIME 타입만
  // 올바르게 지정해 주면 된다.
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mpg") || strstr(filename, ".mpeg"))
    // MPEG 비디오 파일은 video/mpeg로 응답해야 브라우저가
    // 다운로드 대상이 아니라 비디오 컨텐츠로 해석할 수 있다.
    strcpy(filetype, "video/mpeg");
  else
    // 위에서 모르는 확장자는 일단 일반 텍스트로 보낸다.
    // 교육용 Tiny 예제의 기본 동작을 그대로 유지한다.
    strcpy(filetype, "text/plain");
}
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  // Return first part of HTTP reponse
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) // Child
  {
    // Real server would set all CGI vars here
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}
