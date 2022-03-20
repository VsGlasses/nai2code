#include <stdio.h>

#include "nnc.h"

extern NNC_SBR hoge;
static void destructor(void) __attribute__((destructor));
static void constructor(void) __attribute__((constructor));

    void
hoge(
    NNC_STATE const * const st,
    NNC_OBJ   const * const o)
{
    printf("hoge:o->tag = %d\n",o->tag);
}

    static void
constructor(void)
{
    printf("constructor called\n");
}

    static void
destructor(void)
{
    printf("destructor called\n");
}

// vim:et:ts=4 sw=0 isk+=$ fmr=///,//;
