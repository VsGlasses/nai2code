#include <stdio.h>

#include "nnc.h"

extern NNC_SBR hoge;

    void
hoge(
    NNC_STATE const * const st,
    NNC_OBJ   const * const o)
{
    printf("hoge:o->tag = %d\n",o->tag);
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
