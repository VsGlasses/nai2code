#pragma once

#include <stdint.h>

#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma GCC diagnostic ignored "-Wc2x-extensions"
#pragma GCC diagnostic ignored "-Wc++-compat"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma GCC diagnostic ignored "-Wzero-length-array"

#if !defined(NNC_)
#define NNC_(n) NNC_ ## n
#endif

typedef enum __attribute__((packed)) {
    NNC_(TAG_NIL) = 0,
    NNC_(TAG_LST),
    NNC_(TAG_NUM),
    NNC_(TAG_VAR),
    NNC_(TAG_LEV),    // Load Effective VAR
    NNC_(TAG_IF),
    NNC_(TAG_CALL),
    NNC_(TAG_KWD$MIN),
    NNC_(TAG_KWD_let) = NNC_(TAG_KWD$MIN),
    NNC_(TAG_KWD_2let),
    NNC_(TAG_KWD_3let),
    NNC_(TAG_KWD_call),
    NNC_(TAG_KWD_def),
    NNC_(TAG_KWD_PLUS),
    NNC_(TAG_KWD_DOT),
    NNC_(TAG_KWD_LT),
    NNC_(TAG_KWD_1MINUS),
    NNC_(TAG_KWD_MUL),
    NNC_(TAG_KWD_eq),
    NNC_(TAG_KWD_dup),
    NNC_(TAG_KWD_dlopen),
    NNC_(TAG_KWD_dlsym),
    NNC_(TAG_KWD_gc),
    NNC_(TAG_KWD$MAX) = NNC_(TAG_KWD_gc),
    NNC_(TAG_NUM_REF),
    NNC_(TAG_DEF),
    NNC_(TAG_DLH_REF),
    NNC_(TAG_SBR_REF),
    NNC_(TAG_MOV),
    NNC_(TAG_DLH),   // Dynamic Loading Handle
    NNC_(TAG_SBR),
    NNC_(TAG_SYM),
    NNC_(TAG$MAX) = UINT16_MAX
} NNC_(TAG);

_Static_assert(sizeof(NNC_(TAG)) == sizeof(uint16_t));

typedef enum __attribute__((packed)) {
    NNC_(IDX_NIL) = 0,
    NNC_(IDX$SYM_TOP),
    NNC_(IDX_SYM_ATret) = NNC_(IDX$SYM_TOP),
    NNC_(IDX$GC_TOP),
    NNC_(IDX$MAX) = UINT16_MAX
} NNC_(IDX);

_Static_assert(sizeof(NNC_(IDX)) == sizeof(uint16_t));

typedef struct NNC_(STATE) NNC_(STATE);
typedef void NNC_(SBR)(NNC_(STATE) const *);

typedef struct {
    struct {
        NNC_(TAG) tag;
        char      str[0];
    };
    NNC_(IDX) cdr;
    union {
        struct {
            NNC_(IDX) sym;
            NNC_(IDX) car;
        };
         int32_t i32;
        uint32_t u32;
        float    f32;
    };
    void      *ptr[0];
    void      *dlh[0]; // dlopen() handle
    NNC_(SBR) *sbr[0];
} NNC_(OBJ);

enum {
    NNC_(OBJ_SIZE_BITS) = 3,
    NNC_(OBJ_SIZE_MASK) = sizeof(NNC_(OBJ)) - 1,
    NNC_(OBJ_SIZE_PTR) = __builtin_offsetof(NNC_(OBJ),ptr[1]),
    NNC_(OBJ_SIZE_DLH) = __builtin_offsetof(NNC_(OBJ),dlh[1]),
    NNC_(OBJ_SIZE_SBR) = __builtin_offsetof(NNC_(OBJ),sbr[1]),
};

_Static_assert(sizeof(NNC_(OBJ)) == 1 << NNC_(OBJ_SIZE_BITS));
_Static_assert(sizeof(NNC_(OBJ)) * 2  == NNC_(OBJ_SIZE_PTR));
_Static_assert(sizeof(NNC_(OBJ)) * 2  == NNC_(OBJ_SIZE_DLH));
_Static_assert(sizeof(NNC_(OBJ)) * 2  == NNC_(OBJ_SIZE_SBR));

struct NNC_(STATE) {
    NNC_(OBJ)       * (*gc_malloc)(size_t);
    NNC_(OBJ) const * (*SR)(void);
    NNC_(OBJ) const * (*CR)(void);
    NNC_(OBJ) const * (*DR)(void);
    NNC_(OBJ) const * (*PTR)(NNC_(IDX));
};

    static inline NNC_(IDX)
NNC_(nOBJs)(size_t const nbytes)
{
    return (NNC_(IDX))((nbytes + NNC_(OBJ_SIZE_MASK)) >> NNC_(OBJ_SIZE_BITS));
}

    static inline size_t
NNC_(sym_len)(NNC_(TAG) const t)
{
    return __builtin_offsetof(NNC_(OBJ),str) + (t - NNC_(TAG_SYM)) + sizeof("");
}

#undef NNC_

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
