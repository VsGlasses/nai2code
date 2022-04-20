#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define NNC_(n) n
#include "nnc.h"

#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wgnu-case-range"
#pragma GCC diagnostic ignored "-Wgnu-label-as-value"
#pragma GCC diagnostic ignored "-Wgnu-designator"

#if defined(NDEBUG)
#undef assert
#define assert(t) __builtin_assume(t)
#pragma GCC diagnostic ignored "-Wassume"
#endif

static union {
    OBJ            obj[ IDX$MAX + 1];
    uint16_t const u16[(IDX$MAX + 1) * (sizeof(OBJ) / sizeof(uint16_t))];
} heap[2] = {
    { .u16 = {
        TAG_NIL,/* cdr */IDX_NIL,/* sym */IDX_SYM_ATret,/* car */IDX_NIL,
        TAG_SYM + sizeof("@ret") - sizeof(""),'@' | ('r' << 8),'e' | ('t' << 8),'\0',

#include "bootstrap.h"

    } },{ .u16 = {
        TAG_NIL,/* cdr */IDX_NIL,/* sym */IDX_SYM_ATret,/* car */IDX_NIL,
        TAG_SYM + sizeof("@ret") - sizeof(""),'@' | ('r' << 8),'e' | ('t' << 8),'\0',
    } }
};

_Static_assert(sizeof(heap->obj) == sizeof(heap->u16));

#define COUNTOF(array) (sizeof(array) / sizeof(0[array]))

#define PTR_NIL PTR(IDX_NIL)

#if 1
#   define likely(x)   __builtin_expect(!!(x),1)
#   define unlikely(x) __builtin_expect(!!(x),0)
#elif 1
#   define likely(x)   (!!(x))
#   define unlikely(x) (!!(x))
#else
#   define likely(x)   __builtin_expect(!!(x),0)
#   define unlikely(x) __builtin_expect(!!(x),1)
#endif

static OBJ const *          hp_top =  heap->obj;
static OBJ       * restrict hp     = &heap->obj[ORG_HP];

static inline OBJ const * PTR(IDX i) __attribute__((assume_aligned(sizeof(OBJ),sizeof(OBJ))));
static inline OBJ const * PTR(IDX i) { return &hp_top[i]; }

#define IDX(p) ((IDX)((p) - hp_top))

static const OBJ *SR,*CR,*DR;

static inline OBJ const * CAR(OBJ const * const p) { return PTR(p->car); }
static inline OBJ const * CDR(OBJ const * const p) { return PTR(p->cdr); }

    static size_t
obj_len(OBJ const * const o)
{
    switch (o->tag) {
        case TAG_NIL ... TAG_SBR_REF:
            return sizeof(OBJ);

        case TAG_MOV:
            return (size_t)o->car << OBJ_SIZE_BITS;

        case TAG_DLH:
        case TAG_SBR:
            return OBJ_SIZE_PTR;

        case TAG_SYM ... TAG$MAX:
            return sym_len(o->tag);
    }
}

    static IDX
gc_copy(
    OBJ * const prev_top,
    IDX   const i)
{
    if (i < IDX$GC_TOP) return i;

    OBJ * const o = &prev_top[i];

    if (TAG_MOV == o->tag) return o->cdr;

    const size_t len = obj_len(o);
    __builtin_memcpy(hp,o,len);
    o->tag = TAG_MOV;
    o->cdr = IDX(hp);
    o->car = nOBJs(len);
    hp += o->car;
    return o->cdr;
}

    static void
gc(void)
{
    OBJ * const prev_top = heap[hp_top != heap->obj].obj,
        * const next_top = heap[hp_top == heap->obj].obj,
        * const restrict prev_hp = hp,
        * o = hp = next_top + IDX$GC_TOP;

    assert(hp_top == prev_top);
    hp_top = next_top;

    SR = PTR(gc_copy(prev_top,(IDX)(SR - prev_top)));
    CR = PTR(gc_copy(prev_top,(IDX)(CR - prev_top)));
    DR = PTR(gc_copy(prev_top,(IDX)(DR - prev_top)));

    while (unlikely(o < hp)) switch (o->tag) {
        case TAG_NIL:
        case TAG_LST:
        case TAG_IF_LT:
        case TAG_NUM_REF:
        case TAG_DEF:
        case TAG_DLH_REF:
        case TAG_SBR_REF:
            o->sym = gc_copy(prev_top,o->sym);
            __attribute__((fallthrough));

        case TAG_VAR:
        case TAG_LEV:
        case TAG_CALL:
            o->car = gc_copy(prev_top,o->car);
            __attribute__((fallthrough));

        case TAG_NUM:
        case TAG_KWD$MIN ... TAG_KWD$MAX:
            o->cdr = gc_copy(prev_top,o->cdr);
            ++o;
            continue;

        case TAG_MOV:
            __builtin_unreachable();

        case TAG_DLH:
        case TAG_SBR:
            o->sym = gc_copy(prev_top,o->sym);
            o->car = gc_copy(prev_top,o->car);
            o->cdr = gc_copy(prev_top,o->cdr);
            o += OBJ_SIZE_PTR >> OBJ_SIZE_BITS;
            continue;

        case TAG_SYM ... TAG$MAX:
            o += nOBJs(sym_len(o->tag));
            continue;
    }

    // destructors
    o = prev_top;
    while (unlikely(o < prev_hp)) switch (o->tag) {

        case TAG_NIL ... TAG_SBR_REF:
            ++o;
            continue;

        case TAG_MOV:
            o += o->car;
            continue;

        case TAG_DLH:
            if (unlikely(dlclose(*o->dlh))) {
                fputs(dlerror(),stderr);
            }
            __attribute__((fallthrough));

        case TAG_SBR:
            o += OBJ_SIZE_PTR >> OBJ_SIZE_BITS;
            continue;

        case TAG_SYM ... TAG$MAX:
            o += nOBJs(sym_len(o->tag));
            continue;
    }

    //printf("gc_end:%ld use\n",hp - hp_top);
}

static OBJ * gc_malloc(size_t const) __attribute__((assume_aligned(sizeof(OBJ),sizeof(OBJ))));

    static OBJ *
gc_malloc(size_t const s)
{
    assert(s);

    if ((COUNTOF(heap->obj) - (size_t)(hp - hp_top)) * sizeof(OBJ) >= s) {
        OBJ * const o = hp;
        _Static_assert(COUNTOF(heap->obj) * sizeof(OBJ) <= SIZE_MAX - OBJ_SIZE_MASK);
        hp += nOBJs(s);
        return o;
    }

    gc();

    assert((COUNTOF(heap->obj) - (size_t)(hp - hp_top)) * sizeof(OBJ) >= s);
    OBJ * const o = hp;
    hp += nOBJs(s);
    return o;
}

    static void
gc_unmalloc(size_t const s)
{
    hp -= nOBJs(s);
}

    static OBJ const *
def_call(
    OBJ       * const o,
    OBJ const *       def)
{
    assert(def->tag == TAG_DEF);

    OBJ * const p = o + 1;

    o->tag = TAG_LST;
    o->sym = IDX_SYM_ATret;
    o->car = IDX(p);
    o->cdr = CAR(def)->cdr;

    p->tag = TAG_LST;
    p->sym = IDX_NIL;
    p->car = IDX(CR);
    p->cdr = IDX(DR);

    DR = o;

    return CAAR(def);
}

    static OBJ const *
find_var(
    OBJ const * i,
    IDX const   s)
{
    while (1) {
        if (s == i->sym) {
            return i;
        }
        if (IDX_SYM_ATret == i->sym) {
            if (PTR_NIL == i) {
                return PTR_NIL;
            }
        }
        i = CDR(i);
    }
}

static const OBJ *SR,*CR,*DR;

static inline OBJ const * Sp(void) { return SR; }
static inline OBJ const * Cp(void) { return CR; }
static inline OBJ const * Dp(void) { return DR; }
static inline IDX         Si(void) { return IDX(SR); }
static inline IDX         Ci(void) { return IDX(CR); }
static inline IDX         Di(void) { return IDX(DR); }
static inline void        pS(OBJ const * p) { SR = p; }
static inline void        pC(OBJ const * p) { CR = p; }
static inline void        pD(OBJ const * p) { DR = p; }
static inline void        iS(IDX         i) { SR = PTR(i); }
static inline void        iC(IDX         i) { CR = PTR(i); }
static inline void        iD(IDX         i) { DR = PTR(i); }
static inline IDX Idx(OBJ const * p) { return IDX(p); }

static STATE const state = {
    gc_malloc,
    Sp,Cp,Dp,
    Si,Ci,Di,
    pS,pC,pD,
    iS,iC,iD,
    PTR,
    Idx,
    CAR,CDR,
};

    static void
eval(void)
{
    static void const * const next[] = {
        [ TAG_NIL         ] = &&I_NIL,
        [ TAG_LST         ] = &&I_cp,
        [ TAG_NUM         ] = &&I_cp,
        [ TAG_VAR         ] = &&I_VAR,
        [ TAG_LEV         ] = &&I_LEV,
        [ TAG_IF_LT       ] = &&I_IF_LT,
        [ TAG_CALL        ] = &&I_CALL,
        [ TAG_KWD_let ...
          TAG_KWD_3let    ] = &&I_KWD_let,
        [ TAG_KWD_call    ] = &&I_KWD_call,
        [ TAG_KWD_def     ] = &&I_KWD_def,
        [ TAG_KWD_PLUS    ] = &&I_KWD_PLUS,
        [ TAG_KWD_DOT     ] = &&I_KWD_DOT,
        [ TAG_KWD_1MINUS  ] = &&I_KWD_1MINUS,
        [ TAG_KWD_MUL     ] = &&I_KWD_MUL,
        [ TAG_KWD_dup     ] = &&I_KWD_dup,
        [ TAG_KWD_dlopen  ] = &&I_KWD_dlopen,
        [ TAG_KWD_dlsym   ] = &&I_KWD_dlsym,
        [ TAG_KWD_gc_dump ] = &&I_KWD_gc_dump,
        [ TAG_KWD_gc      ] = &&I_KWD_gc,
    };

    _Static_assert(COUNTOF(next) == TAG_KWD$MAX + 1);

    goto *next[CR->tag];

    I_NIL: {
        OBJ const * o = DR;
        while (likely(IDX_SYM_ATret != o->sym)) o = CDR(o);
        if (unlikely(PTR_NIL == o)) return;
        DR = CDAR(o);
        CR = CAAR(o);
    } goto *next[(CR = CDR(CR))->tag];

    I_cp: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        o->tag = CR->tag;
        o->cdr = IDX(SR);
        o->i32 = CR->i32;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_VAR: {
        OBJ * const o = gc_malloc(sizeof(OBJ) * 2);
        IDX const s = CR->car;
        assert(PTR(s)->tag >= TAG_SYM);
        OBJ const * i = find_var(DR,s);

        switch (i->tag) {

            case TAG_NUM_REF:
                o->tag = i->tag;
                i = CAR(i);
                assert(i->tag == TAG_NUM);
                __attribute__((fallthrough));

            case TAG_NIL:
            case TAG_LST:
            case TAG_DLH_REF:
                o->tag = i->tag;
                o->cdr = Si();
                o->i32 = i->i32;
                SR = o;
                gc_unmalloc(sizeof(OBJ));
                assert(hp == o+1);
                goto *next[(CR = CDR(CR))->tag];

            case TAG_DEF:
                CR = def_call(o,i);
                goto *next[CR->tag];

            case TAG_SBR_REF:
                (*CAR(i)->sbr)(&state);
                goto *next[CR->tag];

            case TAG_NUM:
            case TAG_VAR:
            case TAG_LEV:
            case TAG_IF_LT:
            case TAG_CALL:
            case TAG_KWD$MIN ... TAG_KWD$MAX:
            case TAG_MOV:
            case TAG_DLH:
            case TAG_SBR:
            case TAG_SYM ... TAG$MAX:
                __builtin_unreachable();
        }
    }

    I_LEV: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        IDX const s = CR->car;
        assert(PTR(s)->tag >= TAG_SYM);
        OBJ const * i = find_var(DR,s);

        switch (i->tag) {
            case TAG_NUM_REF:
                i = CAR(i);
                assert(i->tag == TAG_NUM);
                __attribute__((fallthrough));

            case TAG_NIL:
            case TAG_LST:
            case TAG_DEF:
            case TAG_DLH_REF:
            case TAG_SBR_REF: {
                o->tag = i->tag;
                o->cdr = Si();
                o->i32 = i->i32;
                SR = o;
            } goto *next[(CR = CDR(CR))->tag];

            case TAG_NUM:
            case TAG_VAR:
            case TAG_LEV:
            case TAG_IF_LT:
            case TAG_CALL:
            case TAG_KWD$MIN ... TAG_KWD$MAX:
            case TAG_MOV:
            case TAG_DLH:
            case TAG_SBR:
            case TAG_SYM ... TAG$MAX:
                __builtin_unreachable();
        }
    }

    I_IF_LT: {
        OBJ const * const n0 = SR;
        OBJ const * const n1 = CDR(n0);
        assert(n0->tag == TAG_NUM);
        assert(n1->tag == TAG_NUM);
        SR = CDR(n1);
        if (n0->i32 > n1->i32) {
            goto *next[(CR = CAR(CR))->tag];
        }
    } goto *next[(CR = CDR(CR))->tag];

    I_CALL: {
        OBJ * const o = gc_malloc(sizeof(OBJ) * 2);
        OBJ * const p = o + 1;

        o->tag = TAG_LST;
        o->sym = IDX_SYM_ATret;
        o->car = IDX(p);
        o->cdr = IDX(DR);

        p->tag = TAG_LST;
        p->sym = IDX_NIL;
        p->car = IDX(CR);
        p->cdr = IDX(DR);

        DR = o;
    } goto *next[(CR = CAR(CR))->tag];

    I_KWD_let: {
        unsigned n = CR->tag - TAG_KWD_let + 1;
        OBJ * o = gc_malloc(n * sizeof(OBJ));
        do {
            assert(TAG_VAR == CDR(CR)->tag);
            assert(SR);
            if (TAG_NUM == SR->tag) {
                o->tag = TAG_NUM_REF;
                o->car = Si();
            } else if (TAG_DLH == SR->tag) {
                o->tag = TAG_DLH_REF;
                o->car = Si();
            } else if (TAG_SBR == SR->tag) {
                o->tag = TAG_SBR_REF;
                o->car = Si();
            } else {
                o->tag = SR->tag;
                o->car = SR->car;
            }
            o->sym = (CR = CDR(CR))->car;
            o->cdr = IDX(DR);
            SR = CDR(SR);
            DR = o++;
        } while (--n);
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_call: switch (SR->tag) {
        case TAG_DEF: {
            OBJ * const o = gc_malloc(sizeof(OBJ) * 2);
            OBJ const * const def = SR;
            SR = CDR(SR);
            CR = def_call(o,def);
        } goto *next[CR->tag];

        case TAG_SBR_REF: {
            SBR * const sbr = *CAR(SR)->sbr;
            SR = CDR(SR);
            sbr(&state);
        } goto *next[CR->tag];

        case TAG_SBR: {
            SBR * const sbr = *SR->sbr;
            SR = CDR(SR);
            sbr(&state);
        } goto *next[CR->tag];

        case TAG_NIL ... TAG_NUM_REF:
        case TAG_DLH_REF:
        case TAG_MOV ... TAG_DLH:
        case TAG_SYM ... TAG$MAX:
            __builtin_unreachable();
    }

    I_KWD_def: {
        assert(SR->tag == TAG_LST);
        OBJ * const o = gc_malloc(sizeof(OBJ) * 2);
        OBJ * const p = o + 1;
        o->tag = TAG_DEF;
        o->sym = IDX_NIL;
        o->cdr = SR->cdr;
        o->car = IDX(p);
        p->tag = TAG_LST;
        p->sym = IDX_NIL;
        p->cdr = IDX(DR);
        p->car = SR->car;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_PLUS: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        OBJ const * const n0 = SR;
        OBJ const * const n1 = CDR(n0);
        assert(n0->tag == TAG_NUM);
        assert(n1->tag == TAG_NUM);
        o->tag = TAG_NUM;
        o->i32 = n0->i32 + n1->i32;
        o->cdr = n1->cdr;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_DOT: {
        if (TAG_NUM == SR->tag) {
            putchar(SR->i32);
        } else {
            __builtin_dump_struct(SR,&printf);
        }
        SR = CDR(SR);
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_1MINUS: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        assert(SR->tag == TAG_NUM);
        o->tag = TAG_NUM;
        o->i32 = SR->i32 - 1;
        o->cdr = SR->cdr;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_MUL: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        OBJ const * const n0 = SR;
        OBJ const * const n1 = CDR(n0);
        assert(n0->tag == TAG_NUM);
        assert(n1->tag == TAG_NUM);
        o->tag = TAG_NUM;
        o->i32 = n0->i32 * n1->i32;
        o->cdr = n1->cdr;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_dup: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        if (TAG_MOV > SR->tag) {
            o->tag = SR->tag;
            o->i32 = SR->i32;
        } else if (TAG_DLH == SR->tag) {
            o->tag = TAG_DLH_REF;
            o->car = Si();
            o->sym = IDX_NIL;
        } else if (TAG_SBR == SR->tag) {
            o->tag = TAG_SBR_REF;
            o->car = Si();
            o->sym = IDX_NIL;
        }
        o->cdr = Si();
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_dlopen: {
        OBJ * const o = gc_malloc(OBJ_SIZE_DLH);
        assert(TAG_VAR == CDR(CR)->tag);
        OBJ const * const s = CADR(CR);
        assert(TAG_SYM < s->tag);
        if (unlikely(!(*o->dlh = dlopen(s->str,RTLD_LAZY)))) {
            gc_unmalloc(OBJ_SIZE_DLH);
            fputs(dlerror(),stderr);
            return;
        }
        o->tag = TAG_DLH;
        o->cdr = Si();
        o->sym = IDX(s);
        o->car = IDX_NIL;
        SR = o;
    } goto *next[(CR = CDDR(CR))->tag];

    I_KWD_dlsym: {
        OBJ * const o = gc_malloc(OBJ_SIZE_SBR);
        OBJ const * d = SR;
        SR = CDR(d);
        if (TAG_DLH_REF == d->tag) {
            d = CAR(d);
        }
        assert(TAG_DLH == d->tag);
        assert(TAG_VAR == CDR(CR)->tag);
        OBJ const * const s = CADR(CR);
        assert(TAG_SYM < s->tag);

        *o->ptr /* sbr */ = dlsym(*d->dlh,s->str);

        char const * const err = dlerror();
        if (unlikely(err)) {
            gc_unmalloc(OBJ_SIZE_SBR);
            fputs(err,stderr);
            return;
        }
        o->tag = TAG_SBR;
        o->cdr = Si();
        o->sym = IDX(s);
        o->car = IDX(d);
        SR = o;
    } goto *next[(CR = CDDR(CR))->tag];

    I_KWD_gc_dump: {
        CR = CDR(CR);
        gc();
        printf("#define ORG_HP (%d)\n",IDX(hp));
        OBJ const * p = hp_top;
        int count = 0;
        while (p < hp) {
            OBJ const * const q = p + nOBJs(obj_len(p));
            if (TAG_DLH == p->tag || TAG_SBR == p->tag) {
                fprintf(stderr,"TAG_DLH or TAG_SBR found:count = %d\n",count);
            }
            do {
                printf("0x%016lX,",*(uint64_t const *)p);
                if (!(++count & 7)) puts("");
            } while (++p < q);
        }
    } goto *next[CR->tag];

    I_KWD_gc: {
        CR = CDR(CR);
        gc();
    } goto *next[CR->tag];
}

    int
main(void)
{
    SR = DR = PTR_NIL;
    CR = PTR(ORG_BS);

    eval();

	return EXIT_SUCCESS;
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
