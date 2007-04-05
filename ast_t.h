#ifndef AST_T_H
#define AST_T_H

#include "ast.h"
#include "symbol.h"
#include "semantic.h"
#include "firm/tr/type.h"
#include "firm/tr/entity.h"

typedef enum {
	TYPE_INVALID,
	TYPE_VOID,
	TYPE_ATOMIC,
	TYPE_STRUCT,
	TYPE_METHOD,
	TYPE_REF
} type_type_t;

typedef enum {
	ATOMIC_TYPE_INVALID,
	ATOMIC_TYPE_BYTE,
	ATOMIC_TYPE_UBYTE,
	ATOMIC_TYPE_INT,
	ATOMIC_TYPE_UINT,
	ATOMIC_TYPE_SHORT,
	ATOMIC_TYPE_USHORT,
	ATOMIC_TYPE_LONG,
	ATOMIC_TYPE_ULONG,
	ATOMIC_TYPE_LONGLONG,
	ATOMIC_TYPE_ULONGLONG,
	ATOMIC_TYPE_FLOAT,
	ATOMIC_TYPE_DOUBLE,
} atomic_type_type_t;

struct type_t {
	type_type_t  type;
	ir_type     *firm_type;
};

struct atomic_type_t {
	type_t              type;
	atomic_type_type_t  atype;
};

struct ref_type_t {
	type_t    type;
	symbol_t *symbol;
};

struct method_parameter_type_t {
	type_t                  *type;
	method_parameter_type_t *next;
};

struct method_type_t {
	type_t                   type;
	type_t                  *result_type;
	method_parameter_type_t *parameter_types;
	const char              *abi_style;
};

typedef enum {
	EXPR_INVALID,
	EXPR_INT_CONST,
	EXPR_CAST,
	EXPR_REFERENCE,
	EXPR_REFERENCE_VARIABLE,
	EXPR_REFERENCE_METHOD,
	EXPR_REFERENCE_EXTERN_METHOD,
	EXPR_REFERENCE_GLOBAL_VARIABLE,
	EXPR_CALL,
	EXPR_BINARY
} expresion_type_t;

struct expression_t {
	expresion_type_t  type;
	type_t           *datatype;
};

struct int_const_t {
	expression_t  expression;
	int           value;
};

struct cast_expression_t {
	expression_t  expression;
	expression_t *value;
};

struct reference_expression_t {
	expression_t                      expression;
	symbol_t                         *symbol;
	union {
		variable_declaration_statement_t *variable;
		method_t                         *method;
		extern_method_t                  *extern_method;
		variable_t                       *global_variable;
	};
};

struct call_expression_t {
	expression_t  expression;
	expression_t *method;
	/* TODO arguments */
};

typedef enum {
	BINEXPR_ADD,
	BINEXPR_SUB,
	BINEXPR_MUL,
	BINEXPR_DIV,
	BINEXPR_MOD,
	BINEXPR_EQUAL,
	BINEXPR_NOTEQUAL,
	BINEXPR_LESS,
	BINEXPR_LESSEQUAL,
	BINEXPR_GREATER,
	BINEXPR_GREATEREQUAL,
	BINEXPR_AND,
	BINEXPR_OR,
	BINEXPR_XOR,
	BINEXPR_SHIFTLEFT,
	BINEXPR_SHIFTRIGHT,
	BINEXPR_ASSIGN
} binexpr_type_t;

struct binary_expression_t {
	expression_t    expression;
	binexpr_type_t  binexpr_type;
	expression_t   *left;
	expression_t   *right;
};

typedef enum {
	STATEMENT_INVALID,
	STATEMENT_BLOCK,
	STATEMENT_RETURN,
	STATEMENT_VARIABLE_DECLARATION,
	STATEMENT_IF,
	STATEMENT_EXPRESSION
} statement_type_t;

struct statement_t {
	statement_t      *next;
	statement_type_t  type;
};

struct return_statement_t {
	statement_t   statement;
	expression_t *return_value;
};

struct block_statement_t {
	statement_t  statement;
	statement_t *first_statement;
};

struct variable_declaration_statement_t {
	statement_t  statement;
	type_t      *type;
	symbol_t    *symbol;

	int          value_number; /**< filled in by semantic phase */
	int          refs;
};

struct if_statement_t {
	statement_t   statement;
	expression_t *condition;
	statement_t  *true_statement;
	statement_t  *false_statement;
};

struct expression_statement_t {
	statement_t   statement;
	expression_t *expression;
};

enum namespace_entry_type_t {
	NAMESPACE_ENTRY_METHOD,
	NAMESPACE_ENTRY_VARIABLE,
	NAMESPACE_ENTRY_EXTERN_METHOD
};

struct namespace_entry_t {
	namespace_entry_type_t  type;
	namespace_entry_t      *next;
};

struct method_t {
	namespace_entry_t  namespace_entry;
	symbol_t          *symbol;
	method_type_t     *type;
	statement_t       *statement;
	/* TODO arguments */

	int                n_local_vars;
	ir_entity         *entity;
};

struct extern_method_t {
	namespace_entry_t  namespace_entry;
	symbol_t          *symbol;
	method_type_t     *type;
	/* TODO arguments */

	ir_entity         *entity;
};

struct variable_t {
	namespace_entry_t  namespace_entry;
	symbol_t          *symbol;
	type_t            *type;
};

struct namespace_t {
	namespace_entry_t *first_entry;
};

#endif
