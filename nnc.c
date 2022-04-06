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
#pragma GCC diagnostic ignored "-Wvla"

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

#define IDX(P) ((IDX)((P) - hp_top))
#define PTR_NIL PTR(IDX_NIL)

#define CAR(X) _Generic(X,IDX:car_idx,default:car_ptr)(X)
#define CDR(X) _Generic(X,IDX:cdr_idx,default:cdr_ptr)(X)
#define CAAR(P) CAR(CAR(P))
#define CADR(P) CAR(CDR(P))
#define CDAR(P) CDR(CAR(P))
#define CDDR(P) CDR(CDR(P))

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

static inline OBJ const * PTR(IDX I) __attribute__((assume_aligned(sizeof(OBJ),sizeof(OBJ))));
static inline OBJ const * PTR(IDX I) { return &hp_top[I]; }

static const OBJ *SR,*CR,*DR;

static inline OBJ const * car_idx(IDX const         i) { return PTR(hp_top[i].car); }
static inline OBJ const * car_ptr(OBJ const * const p) { return PTR(       p->car); }
static inline OBJ const * cdr_idx(IDX const         i) { return PTR(hp_top[i].cdr); }
static inline OBJ const * cdr_ptr(OBJ const * const p) { return PTR(       p->cdr); }

    static size_t
obj_len(OBJ const * const o)
{
    TAG const t = o->tag;
    if (TAG_MOV > t) {
        return sizeof(OBJ);
    } else if (TAG_SYM > t) {
        return sizeof(OBJ) * 2;
    }
    return sym_len(t);
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
        case TAG_IF:
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
            o += 2;
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
            o += 2;
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

static OBJ const * get_SR(void) { return SR; }
static OBJ const * get_CR(void) { return CR; }
static OBJ const * get_DR(void) { return DR; }

static STATE const state = {
    gc_malloc,
    get_SR,
    get_CR,
    get_DR,
    PTR,
};

    static void
eval(void)
{
    static void const * const next[] = {
        [ TAG_NIL        ] = &&I_NIL,
        [ TAG_LST        ] = &&I_cp,
        [ TAG_NUM        ] = &&I_cp,
        [ TAG_VAR        ] = &&I_VAR,
        [ TAG_LEV        ] = &&I_LEV,
        [ TAG_IF         ] = &&I_IF,
        [ TAG_CALL       ] = &&I_CALL,
        [ TAG_KWD_let ...
          TAG_KWD_3let   ] = &&I_KWD_let,
        [ TAG_KWD_call   ] = &&I_KWD_call,
        [ TAG_KWD_def    ] = &&I_KWD_def,
        [ TAG_KWD_PLUS   ] = &&I_KWD_PLUS,
        [ TAG_KWD_DOT    ] = &&I_KWD_DOT,
        [ TAG_KWD_LT     ] = &&I_KWD_LT,
        [ TAG_KWD_1MINUS ] = &&I_KWD_1MINUS,
        [ TAG_KWD_MUL    ] = &&I_KWD_MUL,
        [ TAG_KWD_eq     ] = &&I_KWD_eq,
        [ TAG_KWD_dup    ] = &&I_KWD_dup,
        [ TAG_KWD_dlopen ] = &&I_KWD_dlopen,
        [ TAG_KWD_dlsym  ] = &&I_KWD_dlsym,
        [ TAG_KWD_gc     ] = &&I_KWD_gc,
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
                o->cdr = IDX(SR);
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
                goto *next[(CR = CDR(CR))->tag];

            case TAG_NUM:
            case TAG_VAR:
            case TAG_LEV:
            case TAG_IF:
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
                o->cdr = IDX(SR);
                o->i32 = i->i32;
                SR = o;
            } goto *next[(CR = CDR(CR))->tag];

            case TAG_NUM:
            case TAG_VAR:
            case TAG_LEV:
            case TAG_IF:
            case TAG_CALL:
            case TAG_KWD$MIN ... TAG_KWD$MAX:
            case TAG_MOV:
            case TAG_DLH:
            case TAG_SBR:
            case TAG_SYM ... TAG$MAX:
                __builtin_unreachable();
        }
    }

    I_IF: {
        OBJ const * const o = SR;
        SR = CDR(o);
        if (TAG_LST >= o->tag && IDX_NIL == o->car) {
            goto *next[(CR = CDR(CR))->tag];
        }
    } goto *next[(CR = CAR(CR))->tag];

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
                o->car = IDX(SR);
            } else if (TAG_DLH == SR->tag) {
                o->tag = TAG_DLH_REF;
                o->car = IDX(SR);
            } else if (TAG_SBR == SR->tag) {
                o->tag = TAG_SBR_REF;
                o->car = IDX(SR);
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

    I_KWD_call: {
        assert(TAG_DEF     == SR->tag ||
               TAG_SBR     == SR->tag ||
               TAG_SBR_REF == SR->tag);
        if (TAG_DEF == SR->tag) {
            OBJ * const o = gc_malloc(sizeof(OBJ) * 2);
            CR = def_call(o,SR);
            SR = CDR(SR);
        } else {
            if (TAG_SBR == SR->tag) {
                (*SR->sbr)(&state);
                SR = CDR(SR);
            } else if (TAG_SBR_REF == SR->tag) {
                (*CAR(SR)->sbr)(&state);
                SR = CDR(SR);
            }
            CR = CDR(CR);
        }
    } goto *next[CR->tag];

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
        __builtin_dump_struct(SR,&printf);
        SR = CDR(SR);
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_LT: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        OBJ const * const n0 = SR;
        OBJ const * const n1 = CDR(n0);
        assert(n0->tag == TAG_NUM);
        assert(n1->tag == TAG_NUM);
        if (n0->i32 > n1->i32) {
            o->tag = TAG_NUM;
            o->i32 = -1;
        } else {
            o->tag = TAG_LST;
            o->car = IDX_NIL;
        }
        o->cdr = n1->cdr;
        SR = o;
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

    I_KWD_eq: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        OBJ const * const n0 = SR;
        OBJ const * const n1 = CDR(n0);
        assert(n0->tag == TAG_NUM);
        assert(n1->tag == TAG_NUM);
        if (n0->i32 == n1->i32) {
            o->tag = TAG_NUM;
            o->i32 = -1;
        } else {
            o->tag = TAG_NIL;
            o->car =
            o->sym = IDX_NIL;
        }
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
            o->car = IDX(SR);
            o->sym = IDX_NIL;
        } else if (TAG_SBR == SR->tag) {
            o->tag = TAG_SBR_REF;
            o->car = IDX(SR);
            o->sym = IDX_NIL;
        }
        o->cdr = IDX(SR);
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_dlopen: {
        OBJ * const o = gc_malloc(sizeof(OBJ) * 2);
        assert(TAG_VAR == CDR(CR)->tag);
        OBJ const * const s = CADR(CR);
        assert(TAG_SYM < s->tag);
        if (unlikely(!(*o->dlh = dlopen(s->str,RTLD_LAZY)))) {
            gc_unmalloc(sizeof(OBJ) * 2);
            fputs(dlerror(),stderr);
            return;
        }
        o->tag = TAG_DLH;
        o->cdr = IDX(SR);
        o->sym = IDX(s);
        o->car = IDX_NIL;
        SR = o;
    } goto *next[(CR = CDDR(CR))->tag];

    I_KWD_dlsym: {
        OBJ * const o = gc_malloc(sizeof(OBJ) * 2);
        OBJ const * d = SR;
        SR = CDR(d);
        if (TAG_DLH_REF == d->tag) {
            d = CAR(d);
        }
        assert(TAG_DLH == d->tag);
        assert(TAG_VAR == CDR(CR)->tag);
        OBJ const * const s = CADR(CR);
        assert(TAG_SYM < s->tag);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        *o->sbr = dlsym(*d->dlh,s->str);
#pragma GCC diagnostic pop

        char const * const err = dlerror();
        if (unlikely(err)) {
            gc_unmalloc(sizeof(OBJ) * 2);
            fputs(err,stderr);
            return;
        }
        o->tag = TAG_SBR;
        o->cdr = IDX(SR);
        o->sym = IDX(s);
        o->car = IDX(d);
        SR = o;
    } goto *next[(CR = CDDR(CR))->tag];

    I_KWD_gc: {
        gc();
    } goto *next[(CR = CDR(CR))->tag];
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
