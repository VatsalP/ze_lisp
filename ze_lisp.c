#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "mpc.h"
#include "ze_lisp.h"

#define prompt "ayy> "

/*
	Enums
*/
enum { DVAL_NUM, DVAL_ERR, DVAL_SYM, DVAL_SEXPR };
enum { DERR_DIV_ZERO, DERR_BAD_OP, DERR_BAD_NUM };


int main(int argc, char** argv) {
	/* Ze parser */
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Float = mpc_new("float");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr = mpc_new("sexpr"); 
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");

	/* Define the parser */
	mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     			\
      number   : /-?[0-9]+/ ;                             			\
	  float    : /-?[0-9]+\\.[0-9]+/;                      			\
      symbol   : '+' | '-' | '*' | '/' | '%';             			\
	  sexpr    : '(' <expr>* ')' ;                                  \                                 
      expr     : <float> | <number> | <symbol> | <sexpr> ;          \
      lispy    : /^/ <expr>* /$/ ;             						\
    ",
    Float, Number, Symbol, Sexpr, Expr, Lispy);

	puts("Ze lisp version 0.0.1");
	puts("Ctrl+c to Exit\n");

	while (1) {
		/* prompt user */
		char* input = readline(prompt);

		/* Lets parse */
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			dval* x = dval_eval(dval_read(r.output));
			dval_println(x);
			dval_del(x);
			mpc_ast_delete(r.output);
		} else {
			/* Print syntax error */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		/* history */
		add_history(input);

		/* echo back */
		fprintf(stdout, "%s\n", input);

		/* free */
		free(input);
	}

	mpc_cleanup(6, Float, Number, Symbol, Sexpr, Expr, Lispy);
	return 0;
}

dval* dval_eval_sexpr(dval* v) {

  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = dval_eval(v->cell[i]);
  }

  /* Error Checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == DVAL_ERR) { return dval_take(v, i); }
  }

  /* Empty Expression */
  if (v->count == 0) { return v; }

  /* Single Expression */
  if (v->count == 1) { return dval_take(v, 0); }

  /* Ensure First Element is Symbol */
  dval* f = dval_pop(v, 0);
  if (f->type != DVAL_SYM) {
    dval_del(f); dval_del(v);
    return dval_err("S-expression Does not start with symbol!");
  }

  /* Call builtin with operator */
  dval* result = builtin_op(v, f->sym);
  dval_del(f);
  return result;
}


dval* dval_eval(dval* v) {
	if (v->type == DVAL_SEXPR) {return dval_eval_sexpr(v); }
	return v;
}


/*
	Construct a pointer to a new Number dval 
*/
dval* dval_num(double x) {
	dval* v = malloc(sizeof(dval));
	v->type = DVAL_NUM;
	v->num = x;
	return v;
}


/*
	Construct a pointer to a new Error dval
*/
dval* dval_err(char* m) {
	dval* v = malloc(sizeof(dval));
	v->type = DVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
}

/* 
	Construct a pointer to a new Symbol dval 
*/
dval* dval_sym(char* s) {
	dval* v = malloc(sizeof(dval));
	v->type = DVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}


/*
	A pointer to a new empty Sexpr dval
*/
dval* dval_sexpr(void) {
	dval* v = malloc(sizeof(dval));
	v->type = DVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

void dval_expr_print(dval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {

    /* Print Value contained within */
    dval_print(v->cell[i]);

    /* Don't print trailing space if last element */
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}


/*
	Print dval
*/
void dval_print(dval* x) {
	switch(x->type) {
		case DVAL_NUM: printf("%f", x->num); break;

		case DVAL_ERR:
			printf("%s", x->err);
			break;

		case DVAL_SYM: printf("%s", x->sym); break;
		case DVAL_SEXPR: dval_expr_print(x, '(', ')'); break;
	}
}

void dval_println(dval* x) { dval_print(x); putchar('\n'); }


/*
	DESTROY DVAL*
*/
void dval_del(dval* v) {
	switch(v->type) {
		/* no malloc for double c takes care for it */
		case DVAL_NUM: break;

		case DVAL_ERR: free(v->err); break;
		case DVAL_SYM: free(v->sym); break;

		/* If Sexpr then del all elems inside */
		case DVAL_SEXPR:
			for (int i = 0; i < v->count; i++) {
				dval_del(v->cell[i]);
			}
			/* for mem allocate to contain dval pointers */
			free(v->cell);
		break;
	}

        free(v); /* free dval struct */
}


dval* dval_read_num(mpc_ast_t* t) {
	double x = atof(t->contents);
	return dval_num(x);
}

dval* dval_read(mpc_ast_t* t) {
	if (strstr(t->tag, "number") || strstr(t->tag, "float")) {
		return dval_read_num(t);
	}
	if (strstr(t->tag, "symbol")) {
		return dval_sym(t->contents);
	}

	/* If root(>) or sexpr then create empty list */
	dval* x = NULL;
	if (strcmp(t->tag, ">") == 0) {
		x = dval_sexpr();
	}
	if (strstr(t->tag, "sexpr")) {
		x = dval_sexpr();
	}

	/* Fill this list with any valid expression contained within */
	for (int i = 0; i < t->children_num; i++) {
		if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    	if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    	if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
		x = dval_add(x, dval_read(t->children[i]));
	}
	
	return x;
}


dval* dval_add(dval* v, dval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(dval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}


dval* dval_pop(dval* v, int i) {
  /* Find the item at "i" */
  dval* x = v->cell[i];

  /* Shift memory after the item at "i" over the top */
  memmove(&v->cell[i], &v->cell[i+1],
    sizeof(dval*) * (v->count-i-1));

  /* Decrease the count of items in the list */
  v->count--;

  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(dval*) * v->count);
  return x;
}

dval* dval_take(dval* v, int i) {
  dval* x = dval_pop(v, i);
  dval_del(v);
  return x;
}

dval* builtin_op(dval* a, char* op) {

  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != DVAL_NUM) {
      dval_del(a);
      return dval_err("Cannot operate on non-number!");
    }
  }

  /* Pop the first element */
  dval* x = dval_pop(a, 0);

  /* If no arguments and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  /* While there are still elements remaining */
  while (a->count > 0) {

    /* Pop the next element */
    dval* y = dval_pop(a, 0);

    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        dval_del(x); dval_del(y);
        x = dval_err("Division By Zero!"); break;
      }
      x->num /= y->num;
    }
    if (strcmp(op, "%") == 0) { x->num = (int)x->num % (int)y->num; }

    dval_del(y);
  }

  dval_del(a); return x;
}