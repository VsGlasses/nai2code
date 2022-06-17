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
#   undef assert
#   define assert(t) __builtin_assume(t)
#   pragma GCC diagnostic ignored "-Wassume"
#   define PUTS_LINE
#else
#   define PUTS_LINE (printf(">%d\n",__LINE__))
#endif

#define NIL_SYM \
    TAG_NIL,/* cdr */IDX_NIL,/* sym */IDX_SYM_ATret,/* car */IDX_NIL, \
    TAG_SYM + sizeof("@ret") - sizeof(""),'@' | ('r' << 8),'e' | ('t' << 8),'\0', \
    TAG_SYM,'\0','\0','\0',/* IDX_SYM_CRUMB */

static union {
    OBJ            obj[ IDX$MAX + 1];
    uint16_t const u16[(IDX$MAX + 1) * (sizeof(OBJ) / sizeof(uint16_t))];
} heap[2] = {
    { .u16 = { NIL_SYM

#include "bootstrap.h"

    } },{ .u16 = { NIL_SYM } }
};

#undef NIL_SYM

_Static_assert(sizeof(heap->obj) == sizeof(heap->u16));

#define COUNTOF(array) (sizeof(array) / sizeof(0[array]))

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

static OBJ const *          NIL =  heap->obj;
static OBJ       * restrict hp  = &heap->obj[ORG_HP];

static inline OBJ const * PTR(IDX i) __attribute__((assume_aligned(sizeof(OBJ),sizeof(OBJ))));
static inline OBJ const * PTR(IDX i) { return &NIL[i]; }

#define IDX(p) ((IDX)((p) - NIL))

static const OBJ *SR,*CR,*DR;

static inline OBJ const * CAR(OBJ const * const p) { return PTR(p->car); }
static inline OBJ const * CDR(OBJ const * const p) { return PTR(p->cdr); }
static inline OBJ const * Sp(void) { return     SR; }
static inline OBJ const * Cp(void) { return     CR; }
static inline OBJ const * Dp(void) { return     DR; }
static inline IDX         Si(void) { return IDX(SR); }
static inline IDX         Ci(void) { return IDX(CR); }
static inline IDX         Di(void) { return IDX(DR); }
static inline void        pS(OBJ const * p) {   SR =     p; }
static inline void        pC(OBJ const * p) {   CR =     p; }
static inline void        pD(OBJ const * p) {   DR =     p; }
static inline void        iS(IDX         i) {   SR = PTR(i); }
static inline void        iC(IDX         i) {   CR = PTR(i); }
static inline void        iD(IDX         i) {   DR = PTR(i); }

    static size_t
obj_len(OBJ const * const o)
{
    switch (o->tag) {
        case TAG_NIL ... TAG_MOV-1:
            return sizeof(OBJ);

        case TAG_MOV:
            return (size_t)o->car << OBJ_SIZE_BITS;

        case TAG_MOV+1 ... TAG_SYM-1:
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
gc(IDX * const gc_objs)
{
    OBJ * const prev_top = heap[NIL != heap->obj].obj,
        * const next_top = heap[NIL == heap->obj].obj,
        * const restrict prev_hp = hp,
        * o = hp = next_top + IDX$GC_TOP;

    assert(NIL == prev_top);
    NIL = next_top;

    SR = PTR(gc_copy(prev_top,(IDX)(SR - prev_top)));
    CR = PTR(gc_copy(prev_top,(IDX)(CR - prev_top)));
    DR = PTR(gc_copy(prev_top,(IDX)(DR - prev_top)));

    assert(!gc_objs || *gc_objs);
    if (gc_objs) {
        int const len = 1 + *gc_objs;
        int i = 1;
        do { gc_objs[i] = gc_copy(prev_top,gc_objs[i]); } while (++i < len);
    }

    while (unlikely(o < hp)) switch (o->tag) {
        case TAG_NIL:
        case TAG_LST:
        case TAG_VAR ... TAG_ANY:
        case TAG_KWD$MAX+1 ... TAG_MOV-1:
            o->sym = gc_copy(prev_top,o->sym);
            __attribute__((fallthrough));

        case TAG_LEV:
        case TAG_IF_LT:
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

        case TAG_NIL ... TAG_MOV-1:
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

    //printf("gc_end:%ld use\n",hp - NIL);
}

    static OBJ *
gc_malloc(
    size_t const s,
    IDX *  const gc_objs)
{
    assert(s);

    if ((COUNTOF(heap->obj) - (size_t)(hp - NIL)) * sizeof(OBJ) >= s) {
        OBJ * const o = hp;
        _Static_assert(COUNTOF(heap->obj) * sizeof(OBJ) <= SIZE_MAX - OBJ_SIZE_MASK);
        hp += nOBJs(s);
        return o;
    }

    gc(gc_objs);

    assert((COUNTOF(heap->obj) - (size_t)(hp - NIL)) * sizeof(OBJ) >= s);
    OBJ * const o = hp;
    hp += nOBJs(s);
    return o;
}

    static void
gc_unmalloc(size_t const s)
{
    hp -= nOBJs(s);
}

/*
    (DR) -> TAG_LST -> (prev DR)
            | :@ret
            v
           (saved CR)
*/
    static void
call(OBJ * const o)
{
    o->tag = TAG_LST;
    o->sym = IDX_SYM_ATret;
    o->car = Ci();
    o->cdr = Di();

    DR = o;
}

    static OBJ const *
find_var(OBJ const * i)
{
    IDX const s = CR->sym;
    assert(PTR(s)->tag >= TAG_SYM);

    while (true) {
        if (s == i->sym) {
            return i;
        }
        if (IDX_SYM_ATret == i->sym) {
            if (NIL == i) {
                return NIL;
            }
        }
        i = CDR(i);
    }
}

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

typedef enum __attribute__((packed)) {
    UNF_ETC = 0,
    UNF_NIL,
    UNF_LST,
    UNF_NUM,
    UNF_VAR, // atom,functor
    UNF_LGQ, // e.g. X Y
    UNF_ANY, // _

    UNF_LNE,    // Logiq No Enviroment
    UNF_LUD,    // Logiq UnDefined
    UNF$MAX
} UNF;

    static UNF
unf_tag(IDX const i)
{
    static UNF const unf[TAG_SYM] = {
        [TAG_NIL]     = UNF_NIL,
        [TAG_LST]     = UNF_LST,
        [TAG_NUM]     = UNF_NUM,
        [TAG_VAR]     = UNF_VAR,
        [TAG_LGQ]     = UNF_LGQ,
        [TAG_ANY]     = UNF_ANY,
        [TAG_NUM_REF] = UNF_NUM,
    };
    return unf[PTR(i)->tag];
}

    static UNF
deref(
    IDX       * const lgq,
    OBJ const *       env,
    IDX       * const gc_objs)
{
    OBJ * const o = gc_malloc(sizeof(OBJ),gc_objs);

    {
        OBJ const * const l = PTR(*lgq);

        assert(TAG_LGQ == l->tag);

        *lgq = IDX(o);
        o->cdr = l->cdr;

        IDX const s = l->sym;

        while (IDX_SYM_ATret != env->sym) {
            if (s != env->sym) {
                env = CDR(env);
                continue;
            }
            while (1) {
                if (TAG_LGQ != env->tag) {
                    o->i32 = env->i32;
                    return unf_tag(o->tag = env->tag);
                }

                OBJ const * const prev_env = env;

                if (prev_env == (env = CAR(env))) {
                    o->tag = env->tag;
                    o->i32 = env->i32;
                    return UNF_LUD;
                }
            }
        }
        o->sym = s;
    }
    o->tag = TAG_LGQ;
    o->car = *lgq;
    return UNF_LNE;
}

    static void
relgq(
    env,
    lgq,
    lgq)
{
    while (1) {
        if (TAG_LGQ == env->id && env->car = lgq) {
            if (env == lgq) return;
            env->car = lgq;
        }
        env = CDR(env);
    }
}

    static OBJ *
rebind(
    IDX   const v,
    IDX * const gc_objs)
{
    gc_objs[0] += 2;
    X = v;
    Y = Di();

    *(DR = gc_malloc(sizeof(OBJ),gc_objs)) = *PTR(Y);
    if (X == Y) {
        gc_objs[0] -= 2;
        return DR;
    }

    Y = Di();

    while (1) {
        OBJ       * const o = gc_malloc(sizeof(OBJ),gc_objs);
        OBJ const * const p = CDR(Y);
        Y = PTR(Y)->cdr = IDX(o);
        *o = *p;
        if (PTR(X) == p) {
            gc_objs[0] -= 2;
            return o;
        }
    }
}

    static void
eval(void)
{
    static void const * const next[] = {
        [ TAG_NIL         ] = &&I_NIL,
        [ TAG_LST         ] = &&I_cp,
        [ TAG_NUM         ] = &&I_cp,
        [ TAG_VAR ...
          TAG_ANY         ] = &&I_VAR,
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
        [ TAG_KWD_QUES    ] = &&I_KWD_QUES,
        [ TAG_KWD_RULE    ] = &&I_KWD_RULE,
        [ TAG_KWD_gc_dump ] = &&I_KWD_gc_dump,
        [ TAG_KWD_gc      ] = &&I_KWD_gc,
    };

    _Static_assert(COUNTOF(next) == TAG_KWD$MAX + 1);

    goto *next[CR->tag];

    I_NIL: {
        while (likely(IDX_SYM_ATret != DR->sym)) DR = CDR(DR);
        if (unlikely(NIL == DR)) return;
        CR = CDAR(DR);
        DR = CDR(DR);
    } goto *next[CR->tag];

    I_cp: {
        OBJ * const o = gc_malloc(sizeof(OBJ),NULL);
        o->tag = CR->tag;
        o->cdr = Si();
        o->i32 = CR->i32;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_VAR: {
        OBJ       * const o = gc_malloc(sizeof(OBJ),NULL);
        OBJ const * const i = find_var(DR);

        switch (i->tag) {
            case TAG_NUM_REF:
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
                goto *next[(CR = CDR(CR))->tag];

            case TAG_DEF:
                call(o);
                goto *next[(CR = CAR(i))->tag];

            /*
                > hoo(X Y)

                (CR) -> :hoo -> ...
                        |
                        v
                        :X -> :Y

                > :- (hoo(X Y) bar(Z))

                (DR)  (i)
                 |     |
                 v     v
                ... -> TAG_PRED -> ...
                       |   :hoo
                       v
                       TAG_LST -> :bar
                       |          |
                       v          v
                       :X -> :Y   :Z
            */
            case TAG_PRED: {
#define L(X,Y) unf ## X ## Y
#define U(X,Y) &&L(X,Y)
                static void const * const FAILURE = U(FAIL,URE);
                static void const * const _ANY_ = U(ANY,ANY);
                static void const * const unify[][UNF$MAX] = {
                               // UNF_ETC   UNF_NIL    UNF_LST    UNF_NUM    UNF_VAR    UNF_LGQ    UNF_ANY    UNF_LNE    UNF_LUD
                    [UNF_ETC] = { FAILURE,  FAILURE ,  FAILURE ,  FAILURE ,  FAILURE ,  FAILURE ,  FAILURE ,  FAILURE ,  FAILURE , },
                    [UNF_NIL] = { FAILURE,U(NIL,NIL),  FAILURE ,  FAILURE ,  FAILURE ,  FAILURE ,  FAILURE ,  FAILURE ,  FAILURE , },
                    [UNF_LST] = { FAILURE,  FAILURE ,U(LST,LST),  FAILURE ,  FAILURE ,U(LST,LGQ),   _ANY_  ,U(LST,LNE),U(LST,LUD), },
                    [UNF_NUM] = { FAILURE,  FAILURE ,  FAILURE ,U(NUM,NUM),  FAILURE ,U(NUM,LGQ),   _ANY_  ,U(NUM,LNE),U(NUM,LUD), },
                    [UNF_VAR] = { FAILURE,  FAILURE ,  FAILURE ,  FAILURE ,U(VAR,VAR),U(VAR,LGQ),   _ANY_  ,U(VAR,LNE),U(VAR,LUD), },
                    [UNF_LGQ] = { FAILURE,  FAILURE ,U(LGQ,LST),U(LGQ,NUM),U(LGQ,VAR),U(LGQ,LGQ),   _ANY_  ,          ,          , },
                    [UNF_ANY] = { FAILURE,  FAILURE ,   _ANY_  ,   _ANY_  ,    ANY_  ,   _ANY_  ,   _ANY_  ,          ,          , },
                    [UNF_LNE] = { FAILURE,  FAILURE ,U(LNE,LST),U(LNE,NUM),U(LNE_VAR),          ,          ,U(LNE,LNE),U(LNE,LUD), },
                    [UNF_LUD] = { FAILURE,  FAILURE ,U(LUD,LST),U(LUD,NUM),U(LUD,VAR),          ,          ,U(LUD,LNE),U(LUD,LUD), },
                };
#undef U
                o->tag = TAG_LST;
                o->sym = IDX_SYM_ATret;
                o->car = IDX_NIL;
                o->cdr = Di();

                IDX gc_objs[1024] = {
                    5,
                    IDX(i),      // I
                    IDX(o),      // Y_ENV
                    IDX(o),      // Y_ENV_TAIL
                    CAR(i)->car, // X
                    CR->car,     // Y
                };

#define I          (gc_objs[1])
#define Y_ENV      (gc_objs[2])
#define Y_ENV_TAIL (gc_objs[3])
#define X_ENV      PTR(Y_ENV_TAIL)->cdr
#define W          (gc_objs[gc_objs[0] - 2])
#define X          (gc_objs[gc_objs[0] - 1])
#define Y          (gc_objs[gc_objs[0]])

#define UNIFY(X,Y) (unify[unf_tag(X)][unf_tag(Y)])

                goto *UNIFY(X,Y);

    L(ANY,ANY): goto *UNIFY(X = PTR(X)->cdr,
                            Y = PTR(Y)->cdr);


    L(NUM,NUM):
        if ((TAG_NUM == PTR(X)->tag ? PTR(X) : CAR(X))->i32 ==
            (TAG_NUM == PTR(Y)->tag ? PTR(Y) : CAR(Y))->i32) goto *UNIFY(X,Y);
        goto false;

    L(VAR,VAR):
        if (PTR(X)->sym == PTR(Y)->sym && P)


    L(LST,LGQ): goto *unify[UNF_LST][deref(&Y,PTR(Y_ENV),gc_objs)];
    L(NUM,LGQ): goto *unify[UNF_NUM][deref(&Y,PTR(Y_ENV),gc_objs)];
    L(VAR,LGQ): goto *unify[UNF_VAR][deref(&Y,PTR(Y_ENV),gc_objs)];

    L(LGQ,LST): goto *unify[deref(&X,PTR(X_ENV),gc_objs)][UNF_LST];
    L(LGQ,NUM): goto *unify[deref(&X,PTR(X_ENV),gc_objs)][UNF_NUM];
    L(LGQ,VAR): goto *unify[deref(&X,PTR(X_ENV),gc_objs)][UNF_VAR];

    L(LGQ,LGQ): goto *unify[deref(&X,PTR(X_ENV),gc_objs)]
                           [deref(&Y,PTR(Y_ENV),gc_objs)];

    L(LNE,NUM):{
        OBJ * const o = gc_malloc(sizeof(OBJ),gc_objs);
        o->tag = TAG_NUM_REF;
        o->car = (TAG_NUM == PTR(Y)->tag) ? Y : PTR(Y)->car;
        o->sym = PTR(Y)->sym;
        o->cdr = Di();
        DR = o;
    } goto *UNIFY(X,Y);

    L(LNE,LST):
    L(LNE,VAR):{
        OBJ * const o = gc_malloc(sizeof(OBJ),gc_objs);
        *o = *PTR(Y);
        o->cdr = Di();
        DR = o;
    } goto *UNIFY(X,Y);

    L(LUD,NUM):{
        OBJ * const o = rebind(X,gc_objs);
        o->tag = TAG_NUM_REF;
        o->car = (TAG_NUM == PTR(Y)->tag) ? Y : PTR(Y)->car;
    } goto *UNIFY(X,Y);

    L(LNE,LNE):{
        IDX const ix = X; OBJ * const x = PTR(ix); X = x->cdr;
        IDX const iy = Y; OBJ * const y = PTR(iY); Y = y->cdr;

        x->cdr = Di();
        DR = x;

        y->car = ix;
        y->cdr = Y_ENV;
        Y_ENV = iy;

    } goto *UNIFY(X,Y);

    L(LNE,LUD):{
        IDX const ix = X; OBJ * const x = PTR(ix); X = x->cdr;
        IDX const iy = Y; OBJ * const y = PTR(iY); Y = y->cdr;

        x->cdr = Di();
        DR = x;
        x->car = y->car;

    } goto *UNIFY(X,Y);

    L(LUD,LNE):{
        IDX const ix = X; OBJ * const x = PTR(ix); X = x->cdr;
        IDX const iy = Y; OBJ * const y = PTR(iY); Y = y->cdr;

        y->cdr = Y_ENV;
        Y_ENV = iy;
        y->car = x->car;

    } goto *UNIFY(X,Y);

    L(LUD,LUD):{
        {
            IDX const ix = X; OBJ * const x = PTR(ix); X = x->cdr;
            IDX const iy = Y; OBJ * const y = PTR(iY); Y = y->cdr;

            if (x == y) goto *UNIFY(X,Y);

            gc_objs[0] += 3;

            X = ix;
            Y = iy;
        }

        W = Y_ENV_TAIL;

        while (1) {
            OBJ       * const o = gc_malloc(sizeof(OBJ),gc_objs);
            OBJ const * const p = CDR(W);
            W = PTR(W)->cdr = IDX(o);
            if (TAG_LGQ && car == p) {
                relgq(Y_ENV,p,o);
                if (X == p || Y == p) goto *UNIFY(X,Y);
            }
            *o = *p;
        }
    }

    unf_failure:;
                    PUTS_LINE;
                while (1) {
                    do {
                        i = find_var(CDR(i));
                        if (TAG_PRED == i->tag) goto *UNIFY_X_Y;
                    } while (NIL != i);

                    // backtrack
                    for (i = DR; IDX_SYM_CRUMB != i->sym; i = CDR(i))
                        assert(NIL != i);
                    DR = CDR(i);
                    if (NIL == (i = CAR(i))) goto I_NIL;
                    CR = CDR(i);
                    i = CAR(i);
                }
    unf_success:;
                PUTS_LINE;
                // next CR is CDR(CR).
                i = CDAR(i);
//                gc_vars_push(&i);
                OBJ * const t = gc_malloc(sizeof(OBJ) * ((NIL == i) ? 2 : 3),NULL);

                    PUTS_LINE;
                // trail
                 t   ->tag = TAG_LST;
                 t   ->cdr = Di();
                 t   ->sym = IDX_SYM_CRUMB;
                 t   ->car = IDX(t+1);
                (t+1)->tag = TAG_LST;
                (t+1)->cdr = Ci();
                (t+1)->sym = IDX_NIL;
                (t+1)->car = IDX(i);

                    PUTS_LINE;
                if (NIL == i) {
                    DR = t;
                    PUTS_LINE;
                    goto *next[(CR = CDR(CR))->tag];
                }

                (t+2)->tag = TAG_LST;
                (t+2)->cdr = IDX(t);
                (t+2)->sym = IDX_SYM_ATret;
                (t+2)->car = CR->cdr;

                DR = (t+2);

                    PUTS_LINE;
                goto *next[(CR = i)->tag];
            }

            case TAG_SBR_REF:
                (*CAR(i)->sbr)(&state);
                goto *next[CR->tag];

            case TAG_NUM ... TAG_KWD$MAX:
            case TAG_MOV ... TAG$MAX:
                __builtin_unreachable();
        }
    }

    I_LEV: {
        OBJ * const o = gc_malloc(sizeof(OBJ),NULL);
        OBJ const * i = find_var(DR);

        switch (i->tag) {
            case TAG_NUM_REF:
                i = CAR(i);
                assert(i->tag == TAG_NUM);
                __attribute__((fallthrough));

            case TAG_NIL     ... TAG_LST:
            case TAG_DEF:
            case TAG_DLH_REF ... TAG_SBR_REF: {
                o->tag = i->tag;
                o->cdr = Si();
                o->i32 = i->i32;
                SR = o;
            } goto *next[(CR = CDR(CR))->tag];

            case TAG_NUM ... TAG_KWD$MAX:
            case TAG_PRED:
            case TAG_MOV ... TAG$MAX:
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
        call(gc_malloc(sizeof(OBJ),NULL));
    } goto *next[(CR = CAR(CR))->tag];

    I_KWD_let: {
        unsigned n = CR->tag - TAG_KWD_let + 1;
        OBJ * o = gc_malloc(n * sizeof(OBJ),NULL);
        do {
            assert(TAG_VAR <= CDR(CR)->tag && TAG_ANY >= CDR(CR)->tag);
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
            o->sym = (CR = CDR(CR))->sym;
            o->cdr = Di();
            SR = CDR(SR);
            DR = o++;
        } while (--n);
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_call: switch (SR->tag) {
        case TAG_DEF: {
            call(gc_malloc(sizeof(OBJ),NULL));
            CR = CAR(SR);
            SR = CDR(SR);
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

        case TAG_NIL  ... TAG_KWD$MAX:
        case TAG_PRED ... TAG_DLH_REF:
        case TAG_MOV  ... TAG_DLH:
        case TAG_SYM  ... TAG$MAX:
            __builtin_unreachable();
    }

    /*
        (SR) -> TAG_DEF -> ...
                |
                v
               ...
    */
    I_KWD_def: {
        assert(SR->tag == TAG_LST);
        OBJ * const o = gc_malloc(sizeof(OBJ),NULL);
        o->tag = TAG_DEF;
        o->sym = IDX_NIL;
        o->car = SR->car;
        o->cdr = SR->cdr;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_PLUS: {
        OBJ * const o = gc_malloc(sizeof(OBJ),NULL);
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
        OBJ * const o = gc_malloc(sizeof(OBJ),NULL);
        assert(SR->tag == TAG_NUM);
        o->tag = TAG_NUM;
        o->i32 = SR->i32 - 1;
        o->cdr = SR->cdr;
        SR = o;
    } goto *next[(CR = CDR(CR))->tag];

    I_KWD_MUL: {
        OBJ * const o = gc_malloc(sizeof(OBJ),NULL);
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
        OBJ * const o = gc_malloc(sizeof(OBJ),NULL);
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
        OBJ * const o = gc_malloc(OBJ_SIZE_DLH,NULL);
        assert(TAG_VAR <= CDR(CR)->tag && TAG_ANY >= CDR(CR)->tag);
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
        OBJ * const o = gc_malloc(OBJ_SIZE_SBR,NULL);
        OBJ const * d = SR;
        SR = CDR(d);
        if (TAG_DLH_REF == d->tag) {
            d = CAR(d);
        }
        assert(TAG_DLH == d->tag);
        assert(TAG_VAR <= CDR(CR)->tag && TAG_ANY >= CDR(CR)->tag);
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

    /*
        ?- (hoo(X Y) poo(Z))
           ^
           |
           +---------------------+
                                 |
        (DR) -> TAG_LST -------> TAG_LST -> ...
                | :IDX_SYM_CRUMB :@ret
                v
                NIL
    */
    I_KWD_QUES: {
        OBJ * const r = gc_malloc(sizeof(OBJ) * 2,NULL);
        OBJ * const t = r + 1;

        PUTS_LINE;
        r->tag = TAG_LST;
        r->cdr = Di();
        r->sym = IDX_SYM_ATret;
        r->car = CR->cdr;

        t->tag = TAG_LST;
        t->cdr = IDX(r);
        t->sym = IDX_SYM_CRUMB;
        t->car = IDX_NIL;

        DR = t;
    } goto *next[(CR = CADR(CR))->tag];

    /*
        > :- (hoo(X Y) bar(Z))

        (CR) -> :- -> TAG_LST -> ...
                      |
                      v
               (b) -> :hoo ---> :bar
                      |         |
                      v         v
                      :X -> :Y  :Z
    */
    I_KWD_RULE: {
        OBJ       * const o = gc_malloc(sizeof(OBJ) * 2,NULL);
        OBJ       * const p = o + 1;
        OBJ const * const b = CADR(CR);

        PUTS_LINE;
        o->tag = TAG_PRED;
        o->cdr = Di();
        o->sym = b->sym;
        printf("sym:%s\n",PTR(o->sym)->str);
        o->car = IDX(p);

        p->tag = TAG_LST;
        p->cdr = b->cdr;
        p->sym = IDX_NIL;
        p->car = b->car;

        DR = o;
    } goto *next[(CR = CDDR(CR))->tag];

    I_KWD_gc_dump: {
        CR = CDR(CR);
        gc(NULL);
        printf("#define ORG_HP (%d)\n",IDX(hp));
        OBJ const * p = NIL;
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
        gc(NULL);
    } goto *next[CR->tag];
}

    int
main(void)
{
    SR = DR = NIL;
    CR = PTR(ORG_BS);

    PUTS_LINE;
    eval();

	return EXIT_SUCCESS;
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
