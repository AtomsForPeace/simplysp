#include "mpc.h"
#include <editline/readline.h>
#include <editline/history.h>

/* Use operator string to see which operation to perform */
long eval_op(long x, char* op, long y) {
	if (strcmp(op, "+") == 0) { return x + y; }
	if (strcmp(op, "-") == 0) { return x - y; }
	if (strcmp(op, "*") == 0) { return x * y; }
	if (strcmp(op, "/") == 0) { return x / y; }
	return 0;
}

long eval(mpc_ast_t* t) {

	/* If tagged as number return it directly. */
	if (strstr(t->tag, "number")) {
		return atoi(t->contents);
	}

	/* The operator is always second child. */
	char* op = t->children[1]->contents;

	/* We store the third child in `x` */
	long x = eval(t->children[2]);

	/* Iterate the remaining children and combining. */
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}


int main(int argc, char** argv) {
	/* Create Some Parsers */
	mpc_parser_t* Number   = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr     = mpc_new("expr");
	mpc_parser_t* Simplysp = mpc_new("simplysp");

	/* Define them with the following Language */
	mpca_lang(MPCA_LANG_DEFAULT,
		"                                                     \
			number   : /-?[0-9]+/ ;                             \
			operator : '+' | '-' | '*' | '/' ;                  \
			expr     : <number> | '(' <operator> <expr>+ ')' ;  \
			simplysp : /^/ <operator> <expr>+ /$/ ;             \
		",
		Number, Operator, Expr, Simplysp);

	puts("simplysp version 0.0.0.0.1");
	puts("Press Ctrl+c to Exit\n");

	while (1) {

		/* Now in either case readline will be correctly defined */
		char* input = readline("simplysp> ");
		add_history(input);

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Simplysp, &r)) {
			/* On success print and delete the AST */
			long result = eval(r.output);
			printf("%li\n", result);
			mpc_ast_delete(r.output);
		} else {
			/* Otherwise print and delete the Error */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);

	}

	mpc_cleanup(4, Number, Operator, Expr, Simplysp);

	return 0;
}
