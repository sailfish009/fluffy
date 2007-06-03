#ifndef PARSER_T_H
#define PARSER_T_H

#include "parser.h"

#include <assert.h>

#include "token_t.h"
#include "lexer_t.h"

//#define ABORT_ON_ERROR
//#define PRINT_TOKENS

typedef expression_t* (*parse_expression_function)
                      (parser_env_t *env, unsigned precedence);
typedef expression_t* (*parse_expression_infix_function)
                      (parser_env_t *env, unsigned precedence,
                       expression_t *left);
typedef statement_t*  (*parse_statement_function) (parser_env_t *env);
typedef namespace_entry_t*  (*parse_namespace_entry_function)
                            (parser_env_t *env);

typedef struct expression_parse_function_t {
	unsigned                         precedence;
	parse_expression_function        parser;
	unsigned                         infix_precedence;
	parse_expression_infix_function  infix_parser;
} expression_parse_function_t;

struct parser_env_t {
	token_t                         token;
	source_position_t               source_position;
	expression_parse_function_t    *expression_parsers;
	parse_statement_function       *statement_parsers;
	parse_namespace_entry_function *namespace_parsers;
	lexer_t                         lexer;
	symbol_table_t                  symbol_table;
	int                             error;
	struct obstack                  obst;
};

void register_expression_parser(parser_env_t *env,
                                parse_expression_function parser,
                                int token_type, unsigned precedence);

void register_expression_infix_parser(parser_env_t *env,
                                      parse_expression_infix_function parser,
                                      int token_type, unsigned precedence);

void register_statement_parser(parser_env_t *env,
                               parse_statement_function parser, int token_type);

void register_namespace_parser(parser_env_t *env,
                               parse_namespace_entry_function parser,
                               int token_type);

expression_t *parse_sub_expression(parser_env_t *env, unsigned precedence);

static inline
void next_token(parser_env_t *env)
{
	lexer_next_token(&env->lexer, &env->token);
	env->source_position = env->lexer.source_position;

#ifdef PRINT_TOKENS
	print_token(stderr, & env->token);
	fprintf(stderr, "\n");
#endif
}

static inline
void eat(parser_env_t *env, token_type_t type)
{
	assert(env->token.type == type);
	next_token(env);
}

#endif
