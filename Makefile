CC = gcc
GG = g++
CFLAGS = -Wall
PROGS =	robotClient robotServer
DEPS = utility.h utility.cpp Makefile setupClientSocket.inc
SUPRESS = -Wno-write-strings -Wno-sign-compare

all: $(PROGS)

robotClient: client.cpp clientMessenger.h clientMessenger.cpp $(DEPS)
	${GG} -o $@ -g ${SUPRESS} client.cpp clientMessenger.cpp utility.cpp ${CFLAGS}

robotServer: server.cpp serverMessenger.h serverMessenger.cpp $(DEPS)
	${GG} -o $@ -g ${SUPRESS} server.cpp serverMessenger.cpp utility.cpp Compression.cpp ${CFLAGS}

clean:
	rm -f ${PROGS} position* *.jpg
