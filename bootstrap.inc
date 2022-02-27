L(sym_x)     	GENSYM
L(sym_f)	GENSYM
L(sym_Z)	GENSYM
L(sym_n)	GENSYM
L(bs_start)	TAG(LST,NEXT,def_Z)
		TAG(KWD_let)
		TAG(DYN,sym_f)
		TAG(LST,NEXT,poo)
		TAG(KWD_let)
		TAG(DYN,sym_x)
		TAG(LST,NEXT,pee)
		TAG(LEL,sym_x)
		TAG(LEX,sym_x)
		TAG(KWD_call,OBJ_IDX_NIL,OBJ_IDX_NIL)
L(pee)		TAG(KWD_def)
		TAG(LEX,sym_f,OBJ_IDX_NIL)
L(poo)		TAG(KWD_def)
		TAG(KWD_dup)
		TAG(KWD_call,OBJ_IDX_NIL,OBJ_IDX_NIL)
L(def_Z)	TAG(KWD_def)
		TAG(KWD_let)
		TAG(DYN,sym_Z)
		I32(5)
		TAG(LST,NEXT,hoge)
		TAG(KWD_let)
		TAG(DYN,sym_f)
		TAG(LST,NEXT,hoo)
		TAG(KWD_let)
		TAG(DYN,sym_n)
		TAG(DYN,sym_n)
		I32(0)
		TAG(KWD_eq)
		TAG(IF,NEXT,NEXT + 1)
		I32(1,OBJ_IDX_NIL)
		TAG(DYN,sym_n)
		TAG(KWD_dup)
		TAG(KWD_1MINUS)
		TAG(LEX,sym_f)
		TAG(KWD_MUL,OBJ_IDX_NIL,OBJ_IDX_NIL)
L(hoo)		TAG(KWD_def,OBJ_IDX_NIL,OBJ_IDX_NIL)
L(hoge)		TAG(KWD_def)
		TAG(DYN,sym_Z)
		TAG(KWD_call)
		TAG(KWD_DOT,OBJ_IDX_NIL,OBJ_IDX_NIL)
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
