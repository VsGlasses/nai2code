#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma GCC diagnostic ignored "-Wc2x-extensions"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wgnu-case-range"
#pragma GCC diagnostic ignored "-Wgnu-label-as-value"
#pragma GCC diagnostic ignored "-Wgnu-designator"

typedef enum __attribute__((packed)) {
    OBJ_TAG_NIL = 0,
    OBJ_TAG_LST,
    OBJ_TAG_NUM,
    OBJ_TAG_DYN,
    OBJ_TAG_LEX,
    OBJ_TAG_LED,    // Load Effective DYN
    OBJ_TAG_LEL,    // Load Effective LEX
    OBJ_TAG_IF,
    OBJ_TAG_KWD$MIN,
    OBJ_TAG_KWD_let = OBJ_TAG_KWD$MIN,
    OBJ_TAG_KWD_2let,
    OBJ_TAG_KWD_3let,
    OBJ_TAG_KWD_call,
    OBJ_TAG_KWD_def,
    OBJ_TAG_KWD_PLUS,
    OBJ_TAG_KWD_DOT,
    OBJ_TAG_KWD_LT,
    OBJ_TAG_KWD_1MINUS,
    OBJ_TAG_KWD_MUL,
    OBJ_TAG_KWD_eq,
    OBJ_TAG_KWD_dup,
    OBJ_TAG_KWD$MAX = OBJ_TAG_KWD_dup,
    OBJ_TAG_NUM_REF,
    OBJ_TAG_DEF,
    OBJ_TAG_MOV,
    OBJ_TAG_SYM,
    OBJ_TAG$MAX = UINT16_MAX
} OBJ_TAG;

_Static_assert(sizeof(OBJ_TAG) == sizeof(uint16_t));

/// PASS-1
typedef enum __attribute__((packed)) {

    OBJ_IDX_NIL = 0,
    OBJ_IDX$GC_TOP,

#define TAG(...)
#define I32(...)
#define L(N) N = (NEXT - 1),
#define GENSYM
#define NEXT (OBJ_IDX$GC_TOP + __LINE__)

#include "bootstrap.inc"

#undef TAG
#undef I32
#undef L
#undef GENSYM
#undef NEXT

    OBJ_IDX$MAX = UINT16_MAX
} OBJ_IDX;

_Static_assert(sizeof(OBJ_IDX) == sizeof(uint16_t));

//;

#define OBJ_SIZE_BITS (3)
#define OBJ_SIZE (1 << OBJ_SIZE_BITS)
#define OBJ_SIZE_MASK (sizeof(OBJ) - 1)

typedef struct {
    OBJ_TAG tag;
    union {
        uint8_t str[0];
        struct __attribute__((packed)) {
            OBJ_IDX cdr;
            union {
                struct {
                    OBJ_IDX sym;
                    OBJ_IDX car;
                };
                 int32_t i32;
                uint32_t u32;
                float    f32;
            };
        };
    };
    void * mem[0];
} OBJ;

_Static_assert(sizeof(OBJ) == OBJ_SIZE);
#undef OBJ_SIZE

///  PASS-2
#define TAG3(TAG,CAR,CDR) { OBJ_TAG_ ## TAG,.car = (OBJ_IDX)(CAR),.cdr = (OBJ_IDX)(CDR),.sym = OBJ_IDX_NIL },
#define TAG2(TAG,CAR) TAG3(TAG,CAR,NEXT)
#define TAG1(TAG) TAG2(TAG,OBJ_IDX_NIL)
#define TAG_OPT(TAG,CAR,CDR,TAGn,...) TAGn
#define TAG(...) TAG_OPT(__VA_ARGS__,TAG3,TAG2,TAG1)(__VA_ARGS__)
#define I32_2(NUM,CDR) { OBJ_TAG_NUM,.cdr = (OBJ_IDX)(CDR),.i32 = (NUM) },
#define I32_1(NUM) I32_2(NUM,NEXT)
#define I32_OPT(NUM,CDR,I32n,...) I32n
#define I32(...) I32_OPT(__VA_ARGS__,I32_2,I32_1)(__VA_ARGS__)
#define L(...)
#define GENSYM { .tag = OBJ_TAG_SYM },

static OBJ heap[2][OBJ_IDX$MAX + 1] = {
    {
        TAG(NIL,OBJ_IDX_NIL,OBJ_IDX_NIL)

#define NEXT (OBJ_IDX$GC_TOP + __LINE__)
#include "bootstrap.inc"
#undef NEXT
    },{
        TAG(NIL,OBJ_IDX_NIL,OBJ_IDX_NIL)
    }
};

#undef TAG3
#undef TAG2
#undef TAG1
#undef TAG_OPT
#undef TAG
#undef I32_2
#undef I32_1
#undef I32_OPT
#undef I32
#undef L
#undef GENSYM
//;

#define COUNTOF(array) (sizeof(array) / sizeof(0[array]))

#define IDX(P) ((OBJ_IDX)((P) - hp_top))
#define PTR(I) (&hp_top[I])
#define NIL PTR(OBJ_IDX_NIL)

#define CAR(P) PTR((P)->car)
#define CDR(P) PTR((P)->cdr)
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

static OBJ const *          hp_top =  heap[0];
static OBJ       * restrict hp     = &heap[0][hp_start];

static const OBJ *SR,*CR,*DR;

    static OBJ_IDX
gc_copy(
        OBJ *   const prev_top,
        OBJ_IDX const i)
{
    if (i < OBJ_IDX$GC_TOP) return i;

    OBJ * const o = &prev_top[i];

    if (OBJ_TAG_MOV == o->tag) return o->cdr;

    if (OBJ_TAG_SYM >= o->tag) {
        __builtin_mempcpy(hp,o,sizeof(OBJ));
        o->cdr = IDX(hp++);
    } else {
        const size_t len = __builtin_offsetof(OBJ,str) + (o->tag - OBJ_TAG_SYM);
        __builtin_mempcpy(hp,o,len);
        o->cdr = IDX(hp);
        hp += (len + OBJ_SIZE_MASK) >> OBJ_SIZE_BITS;
    }

    o->tag = OBJ_TAG_MOV;
    return o->cdr;
}

    static OBJ *
gc_malloc(size_t const s)
{
    assert(s);

    if ((COUNTOF(heap[0]) - (size_t)(hp - hp_top)) * sizeof(OBJ) >= s) {
        OBJ * const o = hp;
        _Static_assert(COUNTOF(heap[0]) * sizeof(OBJ) <= SIZE_MAX - OBJ_SIZE_MASK);
        hp += (s + OBJ_SIZE_MASK) >> OBJ_SIZE_BITS;
        return o;
    }

    OBJ * const prev_top = heap[hp_top != heap[0]];
    OBJ * const next_top = heap[hp_top == heap[0]];

    OBJ * o = hp = next_top + OBJ_IDX$GC_TOP;
    hp_top = next_top;

    SR = PTR(gc_copy(prev_top,(OBJ_IDX)(SR - prev_top)));
    CR = PTR(gc_copy(prev_top,(OBJ_IDX)(CR - prev_top)));
    DR = PTR(gc_copy(prev_top,(OBJ_IDX)(DR - prev_top)));

    while (unlikely(o < hp)) switch (o->tag) {
        case OBJ_TAG_NIL:
        case OBJ_TAG_LST:
        case OBJ_TAG_IF:
        case OBJ_TAG_NUM_REF:
        case OBJ_TAG_DEF:
            o->sym = gc_copy(prev_top,o->sym);
            __attribute__((fallthrough));

        case OBJ_TAG_DYN:
        case OBJ_TAG_LEX:
        case OBJ_TAG_LED:
        case OBJ_TAG_LEL:
            o->car = gc_copy(prev_top,o->car);
            __attribute__((fallthrough));

        case OBJ_TAG_NUM:
        case OBJ_TAG_KWD$MIN ... OBJ_TAG_KWD$MAX:
             o->cdr = gc_copy(prev_top,o->cdr);
             ++o;
             continue;

        case OBJ_TAG_SYM ... OBJ_TAG$MAX:
            o += (__builtin_offsetof(OBJ,str) + (o->tag - OBJ_TAG_SYM) + OBJ_SIZE_MASK) >> OBJ_SIZE_BITS;
            continue;

        case OBJ_TAG_MOV:
             __builtin_unreachable();
    }

    //printf("gc_end:%ld use\n",hp - hp_top);

    assert((COUNTOF(heap[0]) - (size_t)(hp - hp_top)) * sizeof(OBJ) >= s);
    o = hp;
    hp += (s + OBJ_SIZE_MASK) >> OBJ_SIZE_BITS;
    return o;
}

static OBJ const *
call(
    OBJ       * const o,
    OBJ const *       i)
{
    o->tag = OBJ_TAG_NIL;
    o->cdr = IDX(DR);
    o->car = IDX(CR);
    DR = o;
    i = CAR(i);
    assert(OBJ_TAG_LST == i->tag);
    o->sym = i->car;
    return CDR(i);
}

    static void
eval(void)
{
    static void const * const next[] = {
        [ OBJ_TAG_NIL        ] = &&I_NIL,
        [ OBJ_TAG_LST        ] = &&I_cp,
        [ OBJ_TAG_NUM        ] = &&I_cp,
        [ OBJ_TAG_DYN        ] = &&I_var,
        [ OBJ_TAG_LEX        ] = &&I_var,
        [ OBJ_TAG_LED        ] = &&I_lea,
        [ OBJ_TAG_LEL        ] = &&I_lea,
        [ OBJ_TAG_IF         ] = &&I_IF,
        [ OBJ_TAG_KWD_let ...
          OBJ_TAG_KWD_3let   ] = &&I_KWD_let,
        [ OBJ_TAG_KWD_call   ] = &&I_KWD_call,
        [ OBJ_TAG_KWD_def    ] = &&I_KWD_def,
        [ OBJ_TAG_KWD_PLUS   ] = &&I_KWD_PLUS,
        [ OBJ_TAG_KWD_DOT    ] = &&I_KWD_DOT,
        [ OBJ_TAG_KWD_LT     ] = &&I_KWD_LT,
        [ OBJ_TAG_KWD_1MINUS ] = &&I_KWD_1MINUS,
        [ OBJ_TAG_KWD_MUL    ] = &&I_KWD_MUL,
        [ OBJ_TAG_KWD_eq     ] = &&I_KWD_eq,
        [ OBJ_TAG_KWD_dup    ] = &&I_KWD_dup,
    };

    _Static_assert(COUNTOF(next) == OBJ_TAG_KWD$MAX + 1);

    goto *next[CR->tag];

    I_NIL: {
        for (OBJ const * o = DR; likely(o != NIL); o = CDR(o)) {
            if (OBJ_TAG_NIL == o->tag) {
                DR = CDR(o);
                goto *next[(CR = CDAR(o))->tag];
            }
        }
    } return;

    I_cp: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        o->tag = CR->tag;
        o->cdr = IDX(SR);
        o->i32 = CR->i32;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_var: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        OBJ_IDX const s = CR->car;
        assert(PTR(s)->tag >= OBJ_TAG_SYM);
        OBJ const * i = DR;
        if (OBJ_TAG_LEX == CR->tag) {
            while (likely(i != NIL)) {
                if (OBJ_TAG_NIL == i->tag) {
                    i = PTR(i->sym);
                    break;
                }
                i = CDR(i);
            }
        }

        while (likely(i != NIL)) {
            if (i->sym == s) switch (i->tag) {
                case OBJ_TAG_NUM_REF:
                    i = CAR(i);
                    assert(i->tag == OBJ_TAG_NUM);
                    __attribute__((fallthrough));

                case OBJ_TAG_NIL:
                case OBJ_TAG_LST: {
                    o->tag = i->tag;
                    o->cdr = IDX(SR);
                    o->i32 = i->i32;
                    SR = o;
                } goto *next[(CR = CDR(CR))->tag];

                case OBJ_TAG_DEF: {
                    CR = call(o,i);
                } goto *next[CR->tag];

                case OBJ_TAG_NUM:
                case OBJ_TAG_DYN:
                case OBJ_TAG_LEX:
                case OBJ_TAG_LED:
                case OBJ_TAG_LEL:
                case OBJ_TAG_IF:
                case OBJ_TAG_KWD$MIN ...  OBJ_TAG_KWD$MAX:
                case OBJ_TAG_MOV:
                case OBJ_TAG_SYM ... OBJ_TAG$MAX:
                    assert(0);
                    __builtin_unreachable();
            }
            i = CDR(i);
        }
        // no found.
        o->tag = OBJ_TAG_NIL;
        o->car = o->sym = OBJ_IDX_NIL;
        o->cdr = IDX(SR);
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_lea: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        OBJ_IDX const s = CR->car;
        assert(PTR(s)->tag >= OBJ_TAG_SYM);
        OBJ const * i = DR;
        if (OBJ_TAG_LEL == CR->tag) {
            while (likely(i != NIL)) {
                if (OBJ_TAG_NIL == i->tag) {
                    i = PTR(i->sym);
                    break;
                }
                i = CDR(i);
            }
        }

        while (likely(i != NIL)) {
            if (i->sym == s) switch (i->tag) {
                case OBJ_TAG_NUM_REF:
                    i = CAR(i);
                    assert(i->tag == OBJ_TAG_NUM);
                    __attribute__((fallthrough));

                case OBJ_TAG_NIL:
                case OBJ_TAG_LST:
                case OBJ_TAG_DEF: {
                    o->tag = i->tag;
                    o->cdr = IDX(SR);
                    o->i32 = i->i32;
                    SR = o;
                } goto *next[(CR = CDR(CR))->tag];

                case OBJ_TAG_NUM:
                case OBJ_TAG_DYN:
                case OBJ_TAG_LEX:
                case OBJ_TAG_LED:
                case OBJ_TAG_LEL:
                case OBJ_TAG_IF:
                case OBJ_TAG_KWD$MIN ...  OBJ_TAG_KWD$MAX:
                case OBJ_TAG_MOV:
                case OBJ_TAG_SYM ... OBJ_TAG$MAX:
                    assert(0);
                    __builtin_unreachable();
            }
            i = CDR(i);
        }
        // no found.
        o->tag = OBJ_TAG_NIL;
        o->car = o->sym = OBJ_IDX_NIL;
        o->cdr = IDX(SR);
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_let: {
        unsigned n = CR->tag - OBJ_TAG_KWD_let + 1;
        OBJ * o = gc_malloc(n * sizeof(OBJ));
        do {
            assert(OBJ_TAG_DYN == CDR(CR)->tag);
            assert(SR);
            if (OBJ_TAG_NUM == SR->tag) {
                o->tag = OBJ_TAG_NUM_REF;
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

    I_IF: {
        OBJ const * const o = SR;
        SR = CDR(o);
        if (OBJ_TAG_LST >= o->tag && OBJ_IDX_NIL == o->car) {
            goto *next[(CR = CDR(CR))->tag];
        }
    } goto *next[(CR = CAR(CR))->tag];

    I_KWD_call: {
        assert(SR->tag == OBJ_TAG_DEF);
        OBJ * const o = gc_malloc(sizeof(OBJ));
        CR = call(o,SR);
        SR = CDR(SR);
    }  goto *next[CR->tag];

    I_KWD_def: {
        assert(SR->tag == OBJ_TAG_LST);
        OBJ * const o = gc_malloc(sizeof(OBJ) * 2);
        OBJ * const p = o + 1;
        o->tag = OBJ_TAG_DEF;
        o->sym = OBJ_IDX_NIL;
        o->cdr = SR->cdr;
        o->car = IDX(p);
        p->tag = OBJ_TAG_LST;
        p->sym = OBJ_IDX_NIL;
        p->cdr = SR->car;
        p->car = IDX(DR);
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_PLUS: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        OBJ const * const n0 = SR;
        OBJ const * const n1 = CDR(n0);
        assert(n0->tag == OBJ_TAG_NUM);
        assert(n1->tag == OBJ_TAG_NUM);
        o->tag = OBJ_TAG_NUM;
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
        assert(n0->tag == OBJ_TAG_NUM);
        assert(n1->tag == OBJ_TAG_NUM);
        if (n0->i32 > n1->i32) {
            o->tag = OBJ_TAG_NUM;
            o->i32 = -1;
        } else {
            o->tag = OBJ_TAG_LST;
            o->car = OBJ_IDX_NIL;
        }
        o->cdr = n1->cdr;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_1MINUS: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        assert(SR->tag == OBJ_TAG_NUM);
        o->tag = OBJ_TAG_NUM;
        o->i32 = SR->i32 - 1;
        o->cdr = SR->cdr;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_MUL: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        OBJ const * const n0 = SR;
        OBJ const * const n1 = CDR(n0);
        assert(n0->tag == OBJ_TAG_NUM);
        assert(n1->tag == OBJ_TAG_NUM);
        o->tag = OBJ_TAG_NUM;
        o->i32 = n0->i32 * n1->i32;
        o->cdr = n1->cdr;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_eq: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        OBJ const * const n0 = SR;
        OBJ const * const n1 = CDR(n0);
        assert(n0->tag == OBJ_TAG_NUM);
        assert(n1->tag == OBJ_TAG_NUM);
        if (n0->i32 == n1->i32) {
            o->tag = OBJ_TAG_NUM;
            o->i32 = -1;
        } else {
            o->tag = OBJ_TAG_NIL;
            o->car =
            o->sym = OBJ_IDX_NIL;
        }
        o->cdr = n1->cdr;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_dup: {
        OBJ * const o = gc_malloc(sizeof(OBJ));
        o->tag = SR->tag;
        o->cdr = IDX(SR);
        o->i32 = SR->i32;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];
}

	int
main(void)
{
    SR = DR = NIL;
    CR = PTR(bs_start);

    eval();

	return EXIT_SUCCESS;
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
