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

    static OBJ *
i32(int const num)
{
    OBJ * const o = PTR(hi++);
    o->tag = TAG_NUM;
    o->cdr = hi;
    o->i32 = num;
    return o;
}
    int
main(void)
{
    IDX const sym_srm_so        = IDX(gen_sym("srm.so"));
    IDX const sym_Open          = IDX(gen_sym("Open"));
    IDX const sym_GetChar       = IDX(gen_sym("GetChar"));
    IDX const sym_bootstrap_nnc = IDX(gen_sym("bootstrap.nnc"));
    IDX const sym_nextc         = IDX(gen_sym("nextc"));

    printf("#define ORG_BS ((IDX)%d)\n",hi);

        GEN(KWD_dlopen);
        GAR(VAR,sym_srm_so);

        GEN(KWD_dup);

        GEN(KWD_dlsym);
        GAR(VAR,sym_Open);

        GEN(KWD_call);
        GAR(VAR,sym_bootstrap_nnc);

        GEN(KWD_dlsym);
        GAR(VAR,sym_GetChar);

        GEN(KWD_let);
        GAR(VAR,sym_nextc);

   OBJ * const loop = GAR(VAR,sym_nextc);
        GEN(KWD_dup);
        i32(0);
        GAR(IF_LT,IDX_NIL);
        GDR(KWD_DOT,IDX(loop));


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
                CASE(KWD_dup);

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
