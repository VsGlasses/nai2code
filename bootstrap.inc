L(sym_x)     	GENSYM
L(sym_f)	GENSYM
L(sym_Z)	GENSYM
L(sym_n)	GENSYM
L(sym_srm_so)	STR("srm.so")
L(sym_hoge)	STR("hoge")
L(bs_start)	ASM(LST,NEXT,def_Z)
		ASM(KWD_let)
		ASM(VAR,sym_f)
		ASM(LST,NEXT,poo)
		ASM(KWD_let)
		ASM(VAR,sym_x)
		ASM(LST,NEXT,pee)
		ASM(LEV,sym_x)
		ASM(VAR,sym_x)
		ASM(KWD_call,0,0)
L(pee)		ASM(KWD_def)
		ASM(VAR,sym_f,0)
L(poo)		ASM(KWD_def)
		ASM(KWD_dup)
		ASM(KWD_call,0,0)
L(def_Z)	ASM(KWD_def)
		ASM(KWD_let)
		ASM(VAR,sym_Z)
		I32(5)
		ASM(LST,NEXT,hoge)
		ASM(KWD_let)
		ASM(VAR,sym_f)
		ASM(LST,NEXT,hoo)
		ASM(KWD_let)
		ASM(VAR,sym_n)
		ASM(VAR,sym_n)
		I32(0)
		ASM(KWD_eq)
		ASM(IF,NEXT,NEXT + 1)
		I32(1,0)
		ASM(VAR,sym_n)
		ASM(KWD_dup)
		ASM(KWD_1MINUS)
		ASM(VAR,sym_f)
		ASM(KWD_MUL,0,0)
L(hoo)		ASM(KWD_def,0,0)
L(hoge)		ASM(KWD_def)
		ASM(VAR,sym_Z)
		ASM(KWD_call)
		ASM(KWD_DOT)
		ASM(KWD_dlopen)
		ASM(VAR,sym_srm_so)
		ASM(KWD_dlsym)
		ASM(VAR,sym_hoge)
		ASM(KWD_call)
		ASM(KWD_gc,0,0)
L(hp_start)

/*
	Z combinator,JavaScript:
	const Z = (f) => ((x) => f((y) => x(x)(y))) ((x) => f((y) => x(x)(y)))

	nnc:
	(let f (let x (&x x call) def f) def dup call) def let Z

	5 factorial,JavaScript:
	Z(f => n => n == 0 ? 1 : n * f(n - 1))(5)

	nnc:
	5 (let f (let n n 0 eq if (1) n dup 1- f *) def) def Z call
*/
