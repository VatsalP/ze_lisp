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
enum { DVAL_NUM, DVAL_ERR };
enum { DERR_DIV_ZERO, DERR_BAD_OP, DERR_BAD_NUM };


int main(int argc, char** argv) {
	/* Ze parser */
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Float = mpc_new("float");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");

	/* Define the parser */
	mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     			\
      number   : /-?[0-9]+/ ;                             			\
	  float    : /-?[0-9]+\\.[0-9]+/;                      			\
      operator : '+' | '-' | '*' | '/' | '%';             			\
      expr     : <float> | <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             			\
    ",
    Float, Number, Operator, Expr, Lispy);

	puts("Ze lisp version 0.0.1");
	puts("Ctrl+c to Exit\n");

	while (1) {
		/* prompt user */
		char* input = readline(prompt);

		/* Lets parse */
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			dval result = eval(r.output);
			/* Output */
			dval_println(result);
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

	mpc_cleanup(5, Float, Number, Operator, Expr, Lispy);
	return 0;
}

/*
	Evaluates expression based on operator used

	returns result of exp
*/
dval eval_op(dval x, char* op, dval y) {
	if (x.type == DVAL_ERR) { return x; }
	if (x.type == DVAL_ERR) { return y; }

	if (strcmp(op, "+") == 0) { return dval_num(x.num + y.num); }
	if (strcmp(op, "-") == 0) { return dval_num(x.num - y.num); }
	if (strcmp(op, "*") == 0) { return dval_num(x.num * y.num); }
	if (strcmp(op, "/") == 0) { 
		return y.num == 0
      		? dval_err(DERR_DIV_ZERO)
      		: dval_num(x.num / y.num); 
	}
	if (strcmp(op, "%") == 0) { 
		double  ret = (int)x.num % (int)y.num;
		return dval_num(ret);
	}
	return dval_err(DERR_BAD_OP);
}

/*
	Evaluates mpc ast

	Returns final value
*/
dval eval(mpc_ast_t* t) {

	if (strstr(t->tag, "number") || strstr(t->tag, "float")) {
		return dval_num(atof(t->contents));
	}

	char* op = t->children[1]->contents;
	
	dval x = eval(t->children[2]);

	int i = 3;
	while(strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}
	return x;
}


/*
	Create a new number type dval
*/
dval dval_num(double x) {
	dval v;
	v.type = DVAL_NUM;
	v.num = x;
	return v;
}


/*
	Create a new error type dval
*/
dval dval_err(int x) {
	dval v;
	v.type = DVAL_ERR;
	v.err = x;
	return v;
}


/*
	Print dval
*/
void dval_print(dval x) {
	switch(x.type) {
		case DVAL_NUM: printf("%f\n", x.num); break;

		case DVAL_ERR:
			if (x.err == DERR_DIV_ZERO) {
				printf("Error: Division by Zero");
			}
			if (x.err == DERR_BAD_OP) {
				printf("Error: Invalid Operator");
			}
			if (x.err == DERR_BAD_NUM) {
				printf("Error: Invalid Number");
			}
			break;
	}
}

void dval_println(dval x) { dval_print(x); putchar('\n'); }