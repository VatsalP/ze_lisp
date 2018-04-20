#include "mpc.h"

typedef struct dval {
  int type;
  double num;

  /* Error and Symbols types */
  char* err;
  char* sym;

  /* count and pointer to a list dval* */
  int count;
  struct dval** cell
} dval;


dval* dval_pop(dval* v, int i);
dval* dval_take(dval* v, int i);
dval* builtin_op(dval* a, char* op);
dval* dval_eval(dval* v);
dval* dval_eval_sexpr(dval* v);

dval* dval_num(double x);
dval* dval_err(char* m);
dval* dval_sym(char* s);
dval* dval_sexpr(void);

void dval_expr_print(dval* v, char open, char close);
void dval_print(dval* x);
void dval_println(dval* x);

void dval_del(dval* x);

dval* dval_read_num(mpc_ast_t* t);
dval* dval_read(mpc_ast_t* t);
dval* dval_add(dval* v, dval* x);