#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <histedit.h>

#include "mpc.h"
#include "ze_lisp.h"

#define prompt "ayy> "


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
			double result = eval(r.output);
			/* Output */
			fprintf(stdout, "%g\n", result);
			mpc_ast_print(r.output);
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
double eval_op(double x, char* op, double y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  return 0;
}

/*
	Evaluates mpc ast

	Returns final value
*/
double eval(mpc_ast_t* t) {

	if (strstr(t->tag, "number")) {
		return atof(t->contents);
	}

	if (strstr(t->tag, "float")) {
		return atof(t->contents);
	}

	char* op = t->children[1]->contents;
	
	double x = eval(t->children[2]);

	int i = 3;
	while(strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}
	return x;
}