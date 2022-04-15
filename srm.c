#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>

#include "nnc.h"

#define CAR(p) (st->car(p))
#define CDR(p) (st->cdr(p))

extern NNC_SBR Open,GetChar;

static void destructor(void) __attribute__((destructor));
static void constructor(void) __attribute__((constructor));

static uint8_t *addr = MAP_FAILED;
static size_t len,get_idx;

// https://www.ipa.go.jp/security/awareness/vendor/programmingv2/contents/c803.html
    static int
open_safely_readonly(
    char const  * const pathname,
    struct stat * const fstat_result)
{
    struct stat lstat_result;

    if (lstat(pathname,&lstat_result)) return -1;

    if (!S_ISREG(lstat_result.st_mode)) return -1;

    int const fd = open(pathname,O_RDONLY);
    if (fd < 0) return -1;

    if (fstat(fd,fstat_result)) {
        close(fd);
        return -1;
    }

    if (lstat_result.st_ino != fstat_result->st_ino ||
        lstat_result.st_dev != fstat_result->st_dev) {
        close(fd);
        return -1;
    }

    return fd;
}

    void
Open(
    NNC_STATE const * const st)
{
    NNC_OBJ const * o = CDR(st->Cp());
    assert(NNC_TAG_VAR == o->tag);
    st->iC(o->cdr);

    if (MAP_FAILED != addr) return;

    o = CAR(o);
    assert(NNC_TAG_SYM < o->tag);

    {
        struct stat fstat_result;
        int const fd = open_safely_readonly(o->str,&fstat_result);
        if (0 > fd) return;

        if (0 >= fstat_result.st_size) {
            close(fd);
            return;
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
        len = fstat_result.st_size;
#pragma GCC diagnostic pop

        addr = mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
        close(fd);
    }

    if (MAP_FAILED == addr) return;

    get_idx = 0;
}

    void
GetChar(
    NNC_STATE const * const st)
{
    NNC_OBJ * const o = st->gc_malloc(sizeof(NNC_OBJ));

    o->tag = NNC_TAG_NUM;
    o->cdr = st->Si();
    st->pS(o);

    st->iC(st->Cp()->cdr);

    if (MAP_FAILED == addr) {
        o->i32 = -1;
        return;
    }

    o->i32 = addr[get_idx++];

    if (get_idx < len) return;

    if (munmap(addr,len)) assert(0);

    addr = MAP_FAILED;
}

    static void
constructor(void)
{
    printf("constructor called\n");
    assert(MAP_FAILED == addr);
}

    static void
destructor(void)
{
    printf("destructor called\n");
    if (MAP_FAILED == addr) return;

    if (munmap(addr,len)) assert(0);

    addr = MAP_FAILED;
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
