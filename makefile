.DELETE_ON_ERROR :

TARGET = nnc bootstrap bootstrap.h srm.so

CC = clang-14
CFLAGS = -Weverything -g
#CFLAGS = -Wall -g -ferror-limit=999
CFLAGS += -Ofast -march=native -flto
CFLAGS += -fsanitize=undefined,address,leak
#CFLAGS += -fsanitize=undefined,memory
#CFLAGS += -DNDEBUG
CFLAGS += -D_FORTIFY_SOURCE=2
#CFLAGS += -fuse-ld=lld

all : $(TARGET)

nnc : nnc.c nnc.h bootstrap.h
	$(CC) $< -o $@ $(CFLAGS) -ldl

bootstrap.h : bootstrap
	./bootstrap > $@

bootstrap : bootstrap.c nnc.h
	$(CC) $< -o $@ $(CFLAGS)

%.so : %.c
	$(CC) $< -o $@ $(CFLAGS) -shared -fPIC

srm.so : srm.c nnc.h

.PHONY : clean
clean :
	$(RM) $(TARGET)