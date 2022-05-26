.DELETE_ON_ERROR :

TARGET = nnc bootstrap bootstrap.h srm.so

CC = clang-14
CFLAGS = -Weverything -g
CFLAGS += -Ofast -march=native -flto -fuse-ld=lld
CFLAGS += -fsanitize=undefined,address,leak
#CFLAGS += -fsanitize=undefined,memory
#CFLAGS += -DNDEBUG
#CFLAGS += -D_FORTIFY_SOURCE=2

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