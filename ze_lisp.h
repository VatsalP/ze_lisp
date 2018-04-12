#include "mpc.h"

typedef struct {
  int type;
  double num;
  int err;
} lval;

double eval_op(double x, char* op, double y);
double eval(mpc_ast_t* t);