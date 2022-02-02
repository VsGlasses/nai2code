#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wc2x-extensions"
#pragma GCC diagnostic ignored "-Wgnu-case-range"

typedef enum __attribute__((packed)) {
    OBJ_TAG_LST = 0,
    OBJ_TAG_NUM,
    OBJ_TAG_KWD_PLUS,
    OBJ_TAG_KWD_DOT,
    OBJ_TAG_SYM,
    OBJ_TAG$MAX = UINT16_MAX
} OBJ_TAG;

_Static_assert(sizeof(OBJ_TAG) == sizeof(uint16_t));

typedef enum __attribute__((packed)) {
    OBJ_IDX_NIL = 0,
    OBJ_IDX$SYM_TOP,
    OBJ_IDX_SYM_RETat = OBJ_IDX$SYM_TOP,
    OBJ_IDX$GC_TOP,
    OBJ_IDX$MAX = UINT16_MAX
} OBJ_IDX;

_Static_assert(sizeof(OBJ_IDX) == sizeof(uint16_t));

#define OBJ_SIZE_BITS (3)
#define OBJ_SIZE (1 << OBJ_SIZE_BITS)
#define OBJ_SIZE_MASK (sizeof(OBJ) - 1)

typedef struct {
    uint64_t len;
    void *   ptr;
} OBJ_MEM;

typedef struct {
    OBJ_TAG tag;
    union {
        uint8_t str[0];
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

static OBJ heap[2][OBJ_IDX$MAX + 1];

static OBJ *hp_top =  heap[0],
           *hp     = &heap[0][OBJ_IDX$GC_TOP];

    static void
heap_init(OBJ * const h)
{
    {
        OBJ * const o = h + OBJ_IDX_NIL;
        o->tag = OBJ_TAG_LST;
        o->car = o->cdr = o->lbl = OBJ_IDX_NIL;
    }

    {
        OBJ * const o = h + OBJ_IDX_SYM_RETat;
        o->tag = (OBJ_TAG)(OBJ_TAG_SYM + sizeof("ret@") - sizeof(""));
        o->str[0] = 'r';
        o->str[1] = 'e';
        o->str[2] = 't';
        o->str[3] = '@';
    }
}

    static OBJ *
gc_malloc(size_t const s)
{
    assert((COUNTOF(heap[0]) - IDX(hp)) * sizeof(OBJ) >= s);
    OBJ * const o = hp;
    _Static_assert(COUNTOF(heap[0]) * sizeof(OBJ) <= SIZE_MAX - OBJ_SIZE_MASK);
    hp += (s + OBJ_SIZE_MASK) >> OBJ_SIZE_BITS;
    return o;
}

static OBJ *SR,*CR,*DR;

    static void
eval(void)
{
    goto play;

    next: CR = CDR(CR);

    play: switch (CR->tag) {

        case OBJ_TAG_LST: {
            assert(CR == NIL);
            return;
        }

        case OBJ_TAG_NUM: {
            OBJ * const p = gc_malloc(sizeof(OBJ));
            p->tag = OBJ_TAG_NUM;
            p->i   = CR->i;
            p->cdr = IDX(SR);
            p->lbl = OBJ_IDX_NIL;
            SR = p;
        } goto next;

        case OBJ_TAG_KWD_PLUS: {
            OBJ * const p = gc_malloc(sizeof(OBJ));
            OBJ const * const n0 = SR;
            OBJ const * const n1 = CDR(n0);
            assert(n0->tag == OBJ_TAG_NUM);
            assert(n1->tag == OBJ_TAG_NUM);
            p->tag = OBJ_TAG_NUM;
            p->i   = n0->i + n1->i;
            p->cdr = IDX(CDR(n1));
            p->lbl = OBJ_IDX_NIL;
            SR = p;
        } goto next;

        case OBJ_TAG_KWD_DOT: {
            assert(SR->tag == OBJ_TAG_NUM);
            printf("%d",SR->i);
            SR = CDR(SR);
        } goto next;

        case OBJ_TAG_SYM ... OBJ_IDX$MAX:
            __builtin_unreachable();
    }
    __builtin_unreachable();
}
	int
main(void)
{
    heap_init(heap[0]);
    heap_init(heap[1]);

    SR = DR = NIL;
    CR = hp;

    // 1 2 + .

    OBJ * o = gc_malloc((1+1+1+1) * sizeof(OBJ));

    // '1'
    o->tag = OBJ_TAG_NUM;
    o->i   = 1;
    o->cdr = IDX(o) + 1;
    o->lbl = OBJ_IDX_NIL;
    ++o;

    // '2'
    o->tag = OBJ_TAG_NUM;
    o->i   = 2;
    o->cdr = IDX(o) + 1;
    o->lbl = OBJ_IDX_NIL;
    ++o;

    // '+'
    o->tag = OBJ_TAG_KWD_PLUS;
    o->car = OBJ_IDX_NIL;
    o->cdr = IDX(o) + 1;
    o->lbl = OBJ_IDX_NIL;
    ++o;

    // '.'
    o->tag = OBJ_TAG_KWD_DOT;
    o->car =
    o->cdr =
    o->lbl = OBJ_IDX_NIL;

    eval();

	return EXIT_SUCCESS;
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
