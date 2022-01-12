#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma GCC diagnostic ignored "-Wzero-length-array"

typedef enum __attribute__((packed)) {
    T$MAX = UINT16_MAX
} OBJ_TAG;

_Static_assert(sizeof(OBJ_TAG) == sizeof(uint16_t),"");

typedef enum __attribute__((packed)) {
    OBJ_IDX_NIL = 0,
    OBJ_IDX$MAX = UINT16_MAX
} OBJ_IDX;

_Static_assert(sizeof(OBJ_IDX) == sizeof(uint16_t),"");

#define OBJ_SIZE_BITS (3)
#define OBJ_SIZE (1 << OBJ_SIZE_BITS)

typedef union {
    int64_t fix;
    double  flt;
} OBJ_NUM;

typedef struct {
    uint64_t len;
    void *   ptr;
} OBJ_MEM;

typedef struct {
    OBJ_TAG tag;
    uint8_t str[0];
    OBJ_IDX car;
    OBJ_IDX cdr;
    OBJ_IDX lbl;
    OBJ_NUM num[0];
    OBJ_MEM mem[0];
} OBJ;

_Static_assert(sizeof(OBJ) == OBJ_SIZE,"");
#undef OBJ_SIZE

	int
main(void)
{
	puts("Hello,C world");

	return EXIT_SUCCESS;
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
