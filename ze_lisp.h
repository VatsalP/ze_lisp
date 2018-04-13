#include "mpc.h"

typedef struct {
  int type;
  double num;
  int err;
} dval;

dval eval_op(dval x, char* op, dval y);
dval eval(mpc_ast_t* t);

dval dval_num(double x);
dval dval_err(int x);

void dval_print(dval x);
void dval_println(dval x);