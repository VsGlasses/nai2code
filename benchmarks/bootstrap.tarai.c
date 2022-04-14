#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define NNC_(n) n
#include "nnc.h"

#pragma GCC diagnostic ignored "-Wgnu-case-range"
#pragma GCC diagnostic ignored "-Wlanguage-extension-token"

static OBJ heap[IDX$MAX + 1];
static IDX hi = IDX$GC_TOP;

#define PTR(I) (&heap[I])
#define IDX(P) ((IDX)((P) - heap))
#define NumberOfU16InOBJ (4)
_Static_assert(NumberOfU16InOBJ == sizeof(OBJ) / sizeof(uint16_t));

    static OBJ *
gen_sym(char const s[])
{
    size_t const len = __builtin_strlen(s);
    assert((TAG$MAX - TAG_SYM) >= len);
    OBJ * const o = PTR(hi);
    hi += nOBJs(sym_len(o->tag = TAG_SYM + (TAG)len));
    __builtin_memcpy(o->str,s,len + sizeof(""));
    return o;
}

    static OBJ *
gen(
    TAG const tag,
    IDX const cdr,
    IDX const car)
{
    OBJ * const o = PTR(hi++);
    o->tag = tag;
    o->cdr = cdr;
    o->car = car;
    o->sym = IDX_NIL;
    return o;
}

#define GEN(tag        ) gen(TAG_ ## tag,hi + 1,IDX_NIL)
#define GDR(tag,cdr    ) gen(TAG_ ## tag,(cdr) ,IDX_NIL)
#define GAR(tag,car    ) gen(TAG_ ## tag,hi + 1,(car)  )
#define GDA(tag,cdr,car) gen(TAG_ ## tag,(cdr) ,(car)  )

    int
main(void)
{
    OBJ *o;

    IDX const sym_x     = IDX(gen_sym(""));
    IDX const sym_y     = IDX(gen_sym(""));
    IDX const sym_z     = IDX(gen_sym(""));
    IDX const sym_tarai = IDX(gen_sym(""));

    printf("#define ORG_BS ((IDX)%d)\n",hi);

    o = GAR(LST,hi + 1);

        IDX const tarai = hi;
        GEN(KWD_3let); GAR(VAR,sym_z); GAR(VAR,sym_y); GAR(VAR,sym_x);

        GAR(VAR,sym_y);
        GAR(VAR,sym_x);
        GAR(IF_LT,hi + 2);
        GDA(VAR,IDX_NIL,sym_y);

        GAR(VAR,sym_x); GEN(KWD_1MINUS); GAR(VAR,sym_y); GAR(VAR,sym_z); GAR(CALL,tarai);
        GAR(VAR,sym_y); GEN(KWD_1MINUS); GAR(VAR,sym_z); GAR(VAR,sym_x); GAR(CALL,tarai);
        GAR(VAR,sym_z); GEN(KWD_1MINUS); GAR(VAR,sym_x); GAR(VAR,sym_y); GDA(CALL,tarai,tarai);

    o->cdr = hi;

        GEN(KWD_def); GEN(KWD_let); GAR(VAR,sym_tarai);

        GEN(NUM)->i32 = 12;
        GEN(NUM)->i32 = 6;
        GEN(NUM)->i32 = 0;
        GAR(VAR,sym_tarai);
        GDA(KWD_DOT,IDX_NIL,IDX_NIL);

    printf("#define ORG_HP (%d)\n",hi);

    {
        uint16_t const * u = (typeof(u))PTR(IDX$GC_TOP);
        char const * tag = NULL;
        while (u < (typeof(u))PTR(hi)) {
            switch (*u) {
#define CASE(t) case TAG_ ## t: tag = #t; break
                CASE(NUM);
                CASE(IF_LT);
                CASE(VAR);
                CASE(LST);
                CASE(CALL);
                CASE(KWD_def);
                CASE(KWD_1MINUS);
                CASE(KWD_let);
                CASE(KWD_3let);
                CASE(KWD_DOT);
                CASE(KWD_dlopen);
                CASE(KWD_dlsym);
                CASE(KWD_call);

                case TAG_SYM ... TAG$MAX: {
                    TAG const t = *u++;
                    printf("TAG_SYM + %u,",t - TAG_SYM);
                    int count = NumberOfU16InOBJ * nOBJs(sym_len(t)) - 1;
                    do {
                        uint16_t const n = *u++;
                        printf("0x%04X /* %c%c */,",n,n & 0xFF,n >> 8);
                    } while (--count);
                    puts("");
                } continue;

                default:
                    fprintf(stderr,"%u\n",*u);
                    assert(0);
            }
            printf("TAG_%s,0x%04X,0x%04X,0x%04X,\n",tag,u[1],u[2],u[3]);
            u += NumberOfU16InOBJ;
        }
    }

	return EXIT_SUCCESS;
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
