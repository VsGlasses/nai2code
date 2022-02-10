#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wc2x-extensions"
#pragma GCC diagnostic ignored "-Wgnu-case-range"
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

typedef enum __attribute__((packed)) {
    OBJ_TAG_LST = 0,
    OBJ_TAG_NUM,
    OBJ_TAG_HOP,
    OBJ_TAG_SBR,
    OBJ_TAG_IF,
    OBJ_TAG_MOV,
    OBJ_TAG_KWD$MIN,
    OBJ_TAG_KWD_let = OBJ_TAG_KWD$MIN,
    OBJ_TAG_KWD_2let,
    OBJ_TAG_KWD_3let,
    OBJ_TAG_KWD_sbr,
    OBJ_TAG_KWD_PLUS,
    OBJ_TAG_KWD_DOT,
    OBJ_TAG_KWD_LT,
    OBJ_TAG_KWD_1MINUS,
    OBJ_TAG_KWD$MAX = OBJ_TAG_KWD_1MINUS,
    OBJ_TAG_SYM,
    OBJ_TAG$MAX = UINT16_MAX
} OBJ_TAG;

_Static_assert(sizeof(OBJ_TAG) == sizeof(uint16_t));

/// PASS-1
#define SYM(...)
#define TAG(...)
#define L(N) N = (NEXT - 1),

typedef enum __attribute__((packed)) {

#define NEXT __LINE__
#include "heap_top.inc"
#undef NEXT

#define NEXT OBJ_IDX$GC_TOP + __LINE__
#include "bootstrap.inc"
#undef NEXT

    OBJ_IDX$MAX = UINT16_MAX
} OBJ_IDX;

_Static_assert(sizeof(OBJ_IDX) == sizeof(uint16_t));

#undef SYM
#undef TAG
#undef L
//;

typedef struct {
    uint64_t len;
    void *   ptr;
} OBJ_MEM;

#define OBJ_SIZE_BITS (3)
#define OBJ_SIZE (1 << OBJ_SIZE_BITS)
#define OBJ_SIZE_MASK (sizeof(OBJ) - 1)

typedef struct {
    OBJ_TAG tag;
    union {
        uint8_t const str_ini[OBJ_SIZE - sizeof(OBJ_TAG)];
        uint8_t       str[0];
        struct __attribute__((packed)) {
            OBJ_IDX lbl;
            OBJ_IDX cdr;
            union {
                OBJ_IDX  car;
                 int16_t   i;
                uint16_t   u;
            };
            OBJ_MEM mem[0];
        };
    };
} OBJ;

_Static_assert(sizeof(OBJ) == OBJ_SIZE);
#undef OBJ_SIZE

#define COUNTOF(array) (sizeof(array) / sizeof(0[array]))

#define IDX(P) ((OBJ_IDX)((P) - hp_top))
#define PTR(I) (&hp_top[I])
#define NIL PTR(OBJ_IDX_NIL)

#define CAR(P) PTR((P)->car)
#define CDR(P) PTR((P)->cdr)
#define LBL(P) PTR((P)->lbl)

#if 1
#define likely(x)   __builtin_expect(!!(x),1)
#define unlikely(x) __builtin_expect(!!(x),0)
#else
#if 1
#define likely(x)   (!!(x))
#define unlikely(x) (!!(x))
#else
#define likely(x)   __builtin_expect(!!(x),0)
#define unlikely(x) __builtin_expect(!!(x),1)
#endif
#endif

///  PASS-2
#define SYM(STR) { .tag = (OBJ_TAG)(OBJ_TAG_SYM + sizeof(STR) - sizeof("")),.str_ini = STR },
#define TAG3(TAG,CAR,CDR) { OBJ_TAG_ ## TAG,.car = (OBJ_IDX)(CAR),.cdr = (OBJ_IDX)(CDR),.lbl = OBJ_IDX_NIL },
#define TAG2(TAG,CAR) TAG3(TAG,CAR,NEXT)
#define TAG1(TAG) TAG2(TAG,OBJ_IDX_NIL)
#define TAG_OPT(TAG,CAR,CDR,TAGn,...) TAGn
#define TAG(...) TAG_OPT(__VA_ARGS__,TAG3,TAG2,TAG1)(__VA_ARGS__)
#define L(...)

static OBJ heap[2][OBJ_IDX$MAX + 1] = {
    {
#include "heap_top.inc"

#define NEXT OBJ_IDX$GC_TOP + __LINE__
#include "bootstrap.inc"
#undef NEXT
    },{
#include "heap_top.inc"
    }
};

#undef TAG
#undef TAG_OPT
#undef TAG1
#undef TAG2
#undef TAG3
#undef SYM
#undef L
//;

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
        case OBJ_TAG_LST:
        case OBJ_TAG_HOP:
        case OBJ_TAG_SBR:
        case OBJ_TAG_IF:
            o->car = gc_copy(prev_top,o->car);
            __attribute__((fallthrough));

        case OBJ_TAG_NUM:
            o->lbl = gc_copy(prev_top,o->lbl);
            __attribute__((fallthrough));

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

static uint64_t sbr_cnt = 0;

    static void
eval(void)
{
    goto play;

    next: CR = CDR(CR);

    play: switch (__builtin_expect(CR->tag,OBJ_TAG_HOP)) {

        case OBJ_TAG_LST: {
            if (unlikely(NIL == CR)) {
                for (OBJ const * o = DR; likely(o != NIL); o = CDR(o)) {
                    if (OBJ_IDX_SYM_retAT == o->lbl) {
                        assert(OBJ_TAG_LST == o->tag);
                        DR = CDR(o);
                        CR = CDR(CAR(o));
                        goto play;
                    }
                }
                return;
            }

            OBJ * const o = gc_malloc(sizeof(OBJ));
            o->tag = OBJ_TAG_LST;
            o->cdr = IDX(SR);
            o->lbl = OBJ_IDX_NIL;
            o->car = CR->car;
            SR = o;
        } goto next;

        case OBJ_TAG_NUM: {
            OBJ * const o = gc_malloc(sizeof(OBJ));
            o->tag = OBJ_TAG_NUM;
            o->i   = CR->i;
            o->cdr = IDX(SR);
            o->lbl = OBJ_IDX_NIL;
            SR = o;
        } goto next;

        case OBJ_TAG_HOP: {
            OBJ * const o = gc_malloc(sizeof(OBJ));
            OBJ_IDX const s = CR->car;
            assert(PTR(s)->tag >= OBJ_TAG_SYM);
            for (OBJ const * i = DR; likely(i != NIL); i = CDR(i)) {
                if (i->lbl != s) continue;
                o->tag = i->tag;
                o->cdr = CR->cdr;
                o->lbl = OBJ_IDX_NIL;
                o->car = i->car;
                CR = o;
                goto play;
            }
            // no found.
            o->tag = OBJ_TAG_LST;
            o->lbl = o->car = OBJ_IDX_NIL;
            o->cdr = IDX(SR);
            SR = o;
        } goto next;

        case OBJ_TAG_SBR: {
            if (likely(OBJ_IDX_NIL != CR->cdr)) /* tail call check */ {
                OBJ * const o = gc_malloc(sizeof(OBJ));
                o->tag = OBJ_TAG_LST;
                o->lbl = OBJ_IDX_SYM_retAT;
                o->car = IDX(CR);
                o->cdr = IDX(DR);
                DR = o;
            }
            CR = CAR(CR);
            ++sbr_cnt;
        } goto play;

        case OBJ_TAG_IF: {
            OBJ const * const o = SR;
            SR = CDR(o);
            if (OBJ_TAG_LST == o->tag && OBJ_IDX_NIL == o->car) {
                goto next;
            }
            CR = CAR(CR);
        } goto play;

        case OBJ_TAG_KWD_let ... OBJ_TAG_KWD_3let: {
            unsigned n = CR->tag - OBJ_TAG_KWD_let + 1;
            OBJ * o = gc_malloc(n * sizeof(OBJ));
            do {
                assert(OBJ_TAG_HOP == CDR(CR)->tag);
                assert(SR);
                o->tag = SR->tag;
                o->cdr = IDX(DR);
                o->lbl = (CR = CDR(CR))->car;
                o->car = SR->car;
                SR = CDR(SR);
                DR = o++;
            } while (--n);
        } goto next;

        case OBJ_TAG_KWD_PLUS: {
            OBJ * const o = gc_malloc(sizeof(OBJ));
            OBJ const * const n0 = SR;
            OBJ const * const n1 = CDR(n0);
            assert(n0->tag == OBJ_TAG_NUM);
            assert(n1->tag == OBJ_TAG_NUM);
            o->tag = OBJ_TAG_NUM;
            o->i   = n0->i + n1->i;
            o->cdr = n1->cdr;
            o->lbl = OBJ_IDX_NIL;
            SR = o;
        } goto next;

        case OBJ_TAG_KWD_DOT: {
            assert(SR->tag == OBJ_TAG_NUM);
            printf("%d\n",SR->i);
            SR = CDR(SR);
        } goto next;

        case OBJ_TAG_KWD_sbr: {
            assert(OBJ_TAG_LST == SR->tag);
            OBJ * const o = gc_malloc(sizeof(OBJ));
            o->tag = OBJ_TAG_SBR;
            o->cdr = SR->cdr;
            o->lbl = OBJ_IDX_NIL;
            o->car = SR->car;
            SR = o;
        } goto next;

        case OBJ_TAG_KWD_LT: {
            OBJ * const o = gc_malloc(sizeof(OBJ));
            OBJ const * const n0 = SR;
            OBJ const * const n1 = CDR(n0);
            assert(n0->tag == OBJ_TAG_NUM);
            assert(n1->tag == OBJ_TAG_NUM);
            if (n0->i > n1->i) {
                o->tag = OBJ_TAG_NUM;
                o->i   = -1;
            } else {
                o->tag = OBJ_TAG_LST;
                o->car = OBJ_IDX_NIL;
            }
            o->lbl = OBJ_IDX_NIL;
            o->cdr = n1->cdr;
            SR = o;
        } goto next;

        case OBJ_TAG_KWD_1MINUS: {
            OBJ * const o = gc_malloc(sizeof(OBJ));
            assert(SR->tag == OBJ_TAG_NUM);
            o->tag = OBJ_TAG_NUM;
            o->i = SR->i - 1;
            o->lbl = OBJ_IDX_NIL;
            o->cdr = SR->cdr;
            SR = o;
        } goto next;

        case OBJ_TAG_SYM ... OBJ_IDX$MAX:
        case OBJ_TAG_MOV:
            __builtin_unreachable();
    }

    printf("tag:%d\n",CR->tag);
    __builtin_unreachable();
}

	int
main(void)
{
    SR = DR = NIL;
    CR = PTR(bs_start);

    eval();

    printf("sbr_cnt = %ld\n",sbr_cnt);

	return EXIT_SUCCESS;
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
