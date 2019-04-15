#include "mpc.h"
#include <editline/readline.h>
#include <editline/history.h>


/* A struct for holding our return values in order to deal with errors */
typedef struct lval {
  int type;
  long num;
  /* Error and Symbol types have some string data */
  char* err;
  char* sym;
  /* Count and Pointer to a list of "lval*"; */
  int count;
  struct lval** cell;
} lval;

/* Possible lval Types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

/* Construct a pointer to a new Number lval */
lval* lval_num(long x) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

/* Construct a pointer to a new Error lval */
lval* lval_err(char* m) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
}

/* Construct a pointer to a new Symbol lval */
lval* lval_sym(char* s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

/* A pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

/* function to delete an lval, freeing the memory */
void lval_del(lval* v) {
	switch (v->type) {
		/* Do nothing special for number type */
		case LVAL_NUM: break;

		/* For Err or Sym free the string data */
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;

		/* If Sexp then delete all elements inside */
		case LVAL_SEXPR: 
			for (int i = 0; i< v->count; i++) {
				lval_del(v->cell[i]);
			}

			/* Also free the memory allocated to the pointers */
			free(v->cell);
		break;
	}
	/* Free the memory allocated to the "lval" struct itself */
	free(v);
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_pop(lval* v, int i) {
	/* Find the item at "i" */
	lval* x = v->cell[i];

	/* Shift the memory after the item at "i" over the top */
	memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval*) * (v->count - i - 1));

	/* Decrease the count of itmtes in the list */
	v->count--;

	/* Reallocate the memory used */
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i) {
	lval* x = lval_pop(v, i);
	lval_del(v);
	return x;
}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for (int i = 0; i < v->count; i++) {
		/* Print Value contained within */
		lval_print(v->cell[i]);

		/* Don't print trailing whitespace if last element */
		if (i != (v->count-1)) {
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print(lval* v) {
	switch (v->type)
	{
		case LVAL_NUM:
			printf("%li", v->num);
			break;
		case LVAL_ERR:
			printf("Error: %s", v->err);
			break;
		case LVAL_SYM:
			printf("%s", v->sym);
			break;
		case LVAL_SEXPR:
			lval_expr_print(v, '(', ')');
			break;
	}
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* builtin_op(lval* a, char* op) {

	/* Ensure all arguments are numbers */
	for (int i; i < a->count; i++) {
		if (a->cell[i]->type != LVAL_NUM) 
		{
			lval_del(a);
			return lval_err("Cannt operate on a non-number!");
		}
	}

	/* Pop the first element */
	lval* x = lval_pop(a, 0);

	/* If no arguments and sub then perform unary negation */
	if ((strcmp(op, "-") == 0) && a->count == 0) {
		x->num = -x->num;
	}

	/* While there are still elements remaining */
	while (a->count > 0)
	{
		/* Pop the next element */
		lval* y = lval_pop(a, 0);

		if (strcmp(op, "+") == 0) {
			x->num += y->num;
		}
		if (strcmp(op, "-") == 0) {
			x->num -= y->num;
		}
		if (strcmp(op, "*") == 0) {
			x->num *= y->num;
		}
		if (strcmp(op, "/") == 0) {
			if (y->num == 0) {
				lval_del(x);
				lval_del(y);
				lval_err("Division by zero is not support (not pony)!");
				break;
			}
			x->num /= y->num;
		}
		lval_del(y);
	}
	lval_del(a);
	return x;
}

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v) {

	/* Evaluate the children */
	for (int i = 0; i< v->count; i++) {
		v->cell[i] = lval_eval(v->cell[i]);
	}

	/* Check for errors */
	for (int i = 0; i < v->count; i++) {
		if (v->cell[i]->type ==LVAL_ERR) {
			return lval_take(v, i);
		}
	}

	/* Empty expression */
	if (v->count == 0)
	{
		return v;
	}
	
	/* Simgle expression */
	if (v->count == 1)
	{
		return lval_take(v, 0);
	}
	
	/* Check first element is symbol */
	lval* f = lval_pop(v, 0);
	if (f->type != LVAL_SYM)
	{
		lval_del(f);
		lval_del(v);
		return lval_err("S-expression doesn't start with symbol");
	}
	
	/* Call builtin with operator */
	lval* result = builtin_op(v, f->sym);
	lval_del(f);
	return result;
}

lval* lval_eval(lval* v) {
	/* Evaluate S-expressions */
	if (v->type == LVAL_SEXPR) {
		return lval_eval_sexpr(v);
	}
	return v;
}

lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno !=ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
	/* If Symbol or Number return conversion to that type */
	if (strstr(t->tag, "number")) { return lval_read_num(t); }
	if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

	/* If root (.) or sexpr then create empty list */
	lval* x = NULL;
	if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
	if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }

	/* Add any valid expressions to "x" */
	for (int i = 0; i < t->children_num; i++) {
		if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}

int main(int argc, char** argv) {
	/* Create Some Parsers */
	mpc_parser_t* Number   = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr  = mpc_new("sexpr");
	mpc_parser_t* Expr     = mpc_new("expr");
	mpc_parser_t* Simplysp = mpc_new("simplysp");

	/* Define them with the following Language */
	mpca_lang(MPCA_LANG_DEFAULT,
		"                                                       \
			number   : /-?[0-9]+/ ;                             \
			symbol 	 : '+' | '-' | '*' | '/' ;                  \
			sexpr    : '(' <expr>* ')' ;  						\
			expr     : <number> | <symbol> | <sexpr> ;  		\
			simplysp : /^/ <expr>* /$/ ;             			\
		",
		Number, Symbol, Sexpr, Expr, Simplysp);

	puts("simplysp version 0.0.0.0.1");
	puts("Press Ctrl+c to Exit\n");

	while (1) {

		/* Now in either case readline will be correctly defined */
		char* input = readline("simplysp> ");
		add_history(input);

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Simplysp, &r)) {
			/* On success evaluate and delete the AST */
			lval* x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);
		} else {
			/* Otherwise print and delete the Error */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);

	}

	mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Simplysp);

	return 0;
}
