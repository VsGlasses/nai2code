#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma GCC diagnostic ignored "-Wzero-length-array"

typedef enum __attribute__((packed)) {
    T$MAX = UINT16_MAX
} CELL_TAG;

_Static_assert(sizeof(CELL_TAG) == sizeof(uint16_t),"");

typedef enum __attribute__((packed)) {
    CELL_IDX_NIL = 0,
    CELL_IDX$MAX = UINT16_MAX
} CELL_IDX;

_Static_assert(sizeof(CELL_IDX) == sizeof(uint16_t),"");

#define CELL_SIZE_BITS (3)
#define CELL_SIZE (1 << CELL_SIZE_BITS)

typedef struct {
    CELL_TAG tag;
    uint8_t  str[0];
    CELL_IDX car;
    CELL_IDX cdr;
    CELL_IDX lbl;
    union {
        int64_t fix;
        double  flt;
    } num[0];
} CELL;

_Static_assert(sizeof(CELL) == CELL_SIZE,"");
#undef CELL_SIZE

	int
main(void)
{
	puts("Hello,C world");

	return EXIT_SUCCESS;
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
