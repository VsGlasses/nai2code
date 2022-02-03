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
    OBJ_TAG_HOP,
    OBJ_TAG_SBR,
    OBJ_TAG_IF,
    OBJ_TAG_KWD_let,
    OBJ_TAG_KWD_2let,
    OBJ_TAG_KWD_3let,
    OBJ_TAG_KWD_sbr,
    OBJ_TAG_KWD_PLUS,
    OBJ_TAG_KWD_DOT,
    OBJ_TAG_KWD_LT,
    OBJ_TAG_KWD_1MINUS,
    OBJ_TAG_SYM,
    OBJ_TAG$MAX = UINT16_MAX
} OBJ_TAG;

_Static_assert(sizeof(OBJ_TAG) == sizeof(uint16_t));

typedef enum __attribute__((packed)) {
    OBJ_IDX_NIL = 0,
    OBJ_IDX$SYM_TOP,
    OBJ_IDX_SYM_retAT = OBJ_IDX$SYM_TOP,
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
        OBJ * const o = h + OBJ_IDX_SYM_retAT;
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
    assert((COUNTOF(heap[0]) - (size_t)(hp - hp_top)) * sizeof(OBJ) >= s);
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
            if (NIL == CR) {
                for (OBJ const * o = DR; o != NIL; o = CDR(o)) {
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
            for (OBJ const * i = DR; i != NIL; i = CDR(i)) {
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
            if (OBJ_IDX_NIL != CR->cdr) /* tail call check */ {
                OBJ * const o = gc_malloc(sizeof(OBJ));
                o->tag = OBJ_TAG_LST;
                o->lbl = OBJ_IDX_SYM_retAT;
                o->car = IDX(CR);
                o->cdr = IDX(DR);
                DR = o;
            }
            CR = CAR(CR);
        } goto play;

        case OBJ_TAG_IF: {
            OBJ * const o = SR;
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
            printf("n0(idx:%d tag:%d)=%d,n1(idx:%d tag:%d)=%d\n",IDX(n0),n0->tag,n0->i,IDX(n1),n1->tag,n1->i);
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
            __builtin_unreachable();
    }

    printf("tag:%d\n",CR->tag);
    __builtin_unreachable();
}

    static OBJ_IDX
gensym(void)
{
    OBJ * const o = gc_malloc(sizeof(OBJ));
    o->tag = OBJ_TAG_SYM;
    return IDX(o);
}

    static OBJ *
make_list(
        OBJ_TAG const tag,
        OBJ_IDX const car)
{
    OBJ * const o = gc_malloc(sizeof(OBJ));
    o->tag = tag;
    o->car = car;
    o->lbl = OBJ_IDX_NIL;
    o->cdr = IDX(hp);
    return o;
}

	int
main(void)
{
    heap_init(heap[0]);
    heap_init(heap[1]);

    SR = DR = NIL;

    // make symbols (x,y,z,tarai)
    const OBJ_IDX x     = gensym();
    const OBJ_IDX y     = gensym();
    const OBJ_IDX z     = gensym();
    const OBJ_IDX tarai = gensym();

    /* (      */ CR = make_list(OBJ_TAG_LST,OBJ_IDX_NIL); printf("CR=%d\n",IDX(CR));
    /* 3let   */ CR->car = IDX(make_list(OBJ_TAG_KWD_3let,OBJ_IDX_NIL));
    /* z      */ make_list(OBJ_TAG_HOP,z);
    /* y      */ make_list(OBJ_TAG_HOP,y);
    /* x      */ make_list(OBJ_TAG_HOP,x);
    /* y      */ make_list(OBJ_TAG_HOP,y);
    /* x      */ make_list(OBJ_TAG_HOP,x);
    /* <      */ make_list(OBJ_TAG_KWD_LT,OBJ_IDX_NIL);
    /* if     */ make_list(OBJ_TAG_IF,IDX(hp + 2));
    /* y)     */ make_list(OBJ_TAG_HOP,y)->cdr = OBJ_IDX_NIL;
    /* x      */ make_list(OBJ_TAG_HOP,x);
    /* 1-     */ make_list(OBJ_TAG_KWD_1MINUS,OBJ_IDX_NIL);
    /* y      */ make_list(OBJ_TAG_HOP,y);
    /* z      */ make_list(OBJ_TAG_HOP,z);
    /* tarai  */ make_list(OBJ_TAG_HOP,tarai);
    /* y      */ make_list(OBJ_TAG_HOP,y);
    /* 1-     */ make_list(OBJ_TAG_KWD_1MINUS,OBJ_IDX_NIL);
    /* z      */ make_list(OBJ_TAG_HOP,z);
    /* x      */ make_list(OBJ_TAG_HOP,x);
    /* tarai  */ make_list(OBJ_TAG_HOP,tarai);
    /* z      */ make_list(OBJ_TAG_HOP,z);
    /* 1-     */ make_list(OBJ_TAG_KWD_1MINUS,OBJ_IDX_NIL);
    /* x      */ make_list(OBJ_TAG_HOP,x);
    /* y      */ make_list(OBJ_TAG_HOP,y);
    /* tarai  */ make_list(OBJ_TAG_HOP,tarai);
    /* tarai) */ make_list(OBJ_TAG_HOP,tarai)->cdr = OBJ_IDX_NIL;
    /* sbr    */ CR->cdr = IDX(make_list(OBJ_TAG_KWD_sbr,OBJ_IDX_NIL));
    /* let    */ make_list(OBJ_TAG_KWD_let,OBJ_IDX_NIL);
    /* tarai  */ make_list(OBJ_TAG_HOP,tarai);

    /* 1      */ make_list(OBJ_TAG_NUM,5);
    /* 2      */ make_list(OBJ_TAG_NUM,2);
    /* 3      */ make_list(OBJ_TAG_NUM,0);
    /* tarai  */ make_list(OBJ_TAG_HOP,tarai);
    /* . )    */ make_list(OBJ_TAG_KWD_DOT,OBJ_IDX_NIL)->cdr = OBJ_IDX_NIL;

    eval();

	return EXIT_SUCCESS;
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
