.DELETE_ON_ERROR :

CC = clang-13
CFLAGS = -Weverything -g
CFLAGS += -Ofast -march=native -flto
CFLAGS += -fsanitize=undefined,address,leak
#CFLAGS += -fsanitize=undefined,memory
#CFLAGS += -DNDEBUG

all : nnc srm.so

nnc : nnc.c nnc.h bootstrap.inc
	$(CC) $< -o $@ $(CFLAGS) -ldl

%.so : %.c
	$(CC) $< -o $@ $(CFLAGS) -shared -fPIC

srm.so : srm.c nnc.h

.PHONY : clean
clean :
	$(RM) nnc *.so