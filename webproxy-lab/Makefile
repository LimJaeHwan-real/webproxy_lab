# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread
PORT ?= 15213
HOST ?= 127.0.0.1

all: proxy echoserver echoclient

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

echoserver/echo.o: echoserver/echo.c echoserver/echo.h csapp.h
	$(CC) $(CFLAGS) -c echoserver/echo.c -o echoserver/echo.o

echoserver/echoserver.o: echoserver/echoserver.c echoserver/echo.h csapp.h
	$(CC) $(CFLAGS) -c echoserver/echoserver.c -o echoserver/echoserver.o

echoserver/echoclient.o: echoserver/echoclient.c csapp.h
	$(CC) $(CFLAGS) -c echoserver/echoclient.c -o echoserver/echoclient.o

echoserver: echoserver/echoserver.o echoserver/echo.o csapp.o
	$(CC) $(CFLAGS) echoserver/echoserver.o echoserver/echo.o csapp.o -o echoserver/echoserver $(LDFLAGS)

echoclient: echoserver/echoclient.o csapp.o
	$(CC) $(CFLAGS) echoserver/echoclient.o csapp.o -o echoserver/echoclient $(LDFLAGS)

run-echoserver: echoserver
	./echoserver/echoserver $(PORT)

run-echoclient: echoclient
	./echoserver/echoclient $(HOST) $(PORT)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz
	rm -f echoserver/*.o echoserver/echoserver echoserver/echoclient
