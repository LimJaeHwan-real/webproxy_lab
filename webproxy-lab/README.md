####################################################################
# CS:APP Proxy Lab
#
# 학생용 소스 파일
####################################################################

이 디렉터리에는 CS:APP Proxy Lab을 수행하는 데 필요한 파일들이 들어 있습니다.

proxy.c
csapp.h
csapp.c
    이 파일들은 시작용(starter) 파일입니다. csapp.c와 csapp.h는
    교재에 설명되어 있습니다.

    이 파일들은 원하는 대로 자유롭게 수정해도 됩니다. 또한 필요한
    추가 파일을 만들어 함께 제출해도 됩니다.

    프록시나 tiny 서버에 사용할 고유한 포트를 만들려면
    `port-for-user.pl` 또는 `free-port.sh`를 사용하세요.

Makefile
    프록시 프로그램을 빌드하는 메이크파일입니다. 해답을 빌드하려면
    "make"를 입력하고, 깨끗한 상태에서 다시 빌드하려면
    "make clean" 다음 "make"를 입력하세요.

    제출할 tar 파일을 만들려면 "make handin"을 입력하세요.
    이 파일은 원하는 방식으로 수정해도 됩니다. 강의자는 여러분의
    Makefile을 사용해서 소스 코드로부터 프록시를 빌드합니다.

port-for-user.pl
    특정 사용자에 대해 임의의 포트를 생성합니다.
    사용법: ./port-for-user.pl <userID>

free-port.sh
    사용하지 않는 TCP 포트를 찾아주는 편리한 스크립트입니다.
    프록시나 tiny 서버에 사용할 수 있습니다.
    사용법: ./free-port.sh

driver.sh
    Basic, Concurrency, Cache 항목을 위한 자동 채점기입니다.
    사용법: ./driver.sh

nop-server.py
    자동 채점기를 위한 보조 도구입니다.

tiny
    CS:APP 교재에 나오는 Tiny 웹 서버입니다.
