#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>

#include "nnc.h"

#pragma GCC diagnostic ignored "-Wvla"

extern NNC_SBR Open;
static void destructor(void) __attribute__((destructor));
static void constructor(void) __attribute__((constructor));

static int fd = -1;
static size_t len = 0;
static uint8_t *addr = MAP_FAILED;

// https://www.ipa.go.jp/security/awareness/vendor/programmingv2/contents/c803.html
    static int
open_safely_readonly(
    char const  * const pathname,
    struct stat * const fstat_result)
{
    struct stat lstat_result;

    if (lstat(pathname,&lstat_result)) return -1;

    if (!S_ISREG(lstat_result.st_mode)) return -1;

    int const new_fd = open(pathname,O_RDONLY);
    if (new_fd < 0) return -1;

    if (fstat(new_fd,fstat_result)) {
        close(new_fd);
        return -1;
    }

    if (lstat_result.st_ino != fstat_result->st_ino ||
        lstat_result.st_dev != fstat_result->st_dev) {
        close(new_fd);
        return -1;
    }

    return new_fd;
}

    void
Open(
    NNC_STATE const * const st)
{
    if (!(0 > fd)) return;

    NNC_OBJ const * o = st->PTR(st->CR()->cdr);
    assert(NNC_TAG_VAR == o->tag);
    o = st->PTR(o->car);
    assert(NNC_TAG_SYM < o->tag);

    {
        struct stat fstat_result;
        if (0 > (fd = open_safely_readonly(o->str,&fstat_result))) return;

        if (0 >= fstat_result.st_size) {
            close(fd);
            fd = -1;
            return;
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
        len = fstat_result.st_size;
#pragma GCC diagnostic pop
    }

    if (MAP_FAILED == (addr = mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0))) {
        close(fd);
        fd = -1;
        len = 0;
        return;
    }

    printf("%.*s\n",(int)len,addr);
}

    void
Getchar(
    NNC_STATE const * const st)
{
}

    static void
constructor(void)
{
    printf("constructor called\n");
    assert(0 > fd);
}

    static void
destructor(void)
{
    printf("destructor called\n");
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
