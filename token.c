#include <config.h>

#include "token_t.h"

#include <stdio.h>

#include "known_symbols.h"

void print_token_type(FILE *f, token_type_t token_type)
{
	if(token_type >= 0 && token_type < 256) {
		fprintf(f, "'%c'", token_type);
		return;
	} 

	switch(token_type) {
	case T_EQUALEQUAL:
		fputs("'=='", f);
		break;
	case T_ASSIGN:
		fputs("'<-'", f);
		break;
	case T_SLASHEQUAL:
		fputs("'/='", f);
		break;
	case T_LESSEQUAL:
		fputs("'<='", f);
		break;
	case T_GREATEREQUAL:
		fputs("'>='", f);
		break;
	case T_GREATERGREATER:
		fputs("'>>'", f);
		break;
	case T_DOTDOT:
		fputs("'..'", f);
		break;
	case T_DOTDOTDOT:
		fputs("'...'", f);
		break;
	case T_IDENTIFIER:
		fprintf(f, "identifier");
		break;
	case T_INTEGER:
		fprintf(f, "integer number");
		break;
	case T_STRING_LITERAL:
		fprintf(f, "string literal");
		break;
	case T_EOF:
		fprintf(f, "end of file");
		break;
	case T_ERROR:
		fprintf(f, "malformed token");
		break;
#define T(x)                                  \
	case T_##x:                               \
		fprintf(f, "'" #x "'");               \
		break;
#include "known_symbols.inc"
#undef T
	default:
		fprintf(f, "unknown token");
		break;
	}
}

void print_token(FILE *f, const token_t *token)
{
	switch(token->type) {
	case T_IDENTIFIER:
		fprintf(f, "symbol '%s'", token->symbol->string);
		break;
	case T_INTEGER:
		fprintf(f, "integer number %d", token->intvalue);
		break;
	case T_STRING_LITERAL:
		fprintf(f, "string '%s'", token->string);
		break;
	default:
		print_token_type(f, token->type);
		break;
	}
}