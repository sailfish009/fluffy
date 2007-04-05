#include <config.h>

#include <assert.h>
#include <firm/common/firm.h>
#include <firm/be/be.h>

#include "ast_t.h"
#include "semantic.h"
#include "adt/error.h"
#include "adt/xmalloc.h"

static variable_declaration_statement_t **value_numbers = NULL;

static
ir_node *uninitialized_local_var(ir_graph *irg, ir_mode *mode, int pos)
{
	fprintf(stderr, "Warning: variable '%s' might be used uninitialized\n",
			value_numbers[pos]->symbol->string);
	return new_r_Unknown(irg, mode);
}

void initialize_firm(void)
{
	const backend_params *be_params;
	firm_parameter_t params;
	memset(&params, 0, sizeof(params));

#if 0
	/* read firm options */
	firm_init_options("mlang", 0, NULL);
#endif

	params.size = sizeof(params);
	params.enable_statistics = 0;
	params.initialize_local_func = uninitialized_local_var;
	params.cc_mask = 0;
	params.builtin_dbg = NULL;

	/* initialize backend */
	be_params = be_init();
	params.arch_op_settings = be_params->arch_op_settings;

	/* intialize firm itself */
	init_firm(&params);
}

void exit_firm(void)
{
}

static
ident *unique_id(const char *tag)
{
	static unsigned id = 0;
	char            buf[256];

	snprintf(buf, sizeof(buf), tag, id);
	id++;
	return new_id_from_str(buf);
}

static
ir_type *get_ir_type(type_t *type);

static
ir_mode *get_atomic_mode(const atomic_type_t* atomic_type)
{
	switch(atomic_type->atype) {
	case ATOMIC_TYPE_BYTE:
		return mode_Bs;
	case ATOMIC_TYPE_UBYTE:
		return mode_Bu;
	case ATOMIC_TYPE_SHORT:
		return mode_Hs;
	case ATOMIC_TYPE_USHORT:
		return mode_Hu;
	case ATOMIC_TYPE_INT:
		return mode_Is;
	case ATOMIC_TYPE_UINT:
		return mode_Iu;
	case ATOMIC_TYPE_LONG:
		return mode_Ls;
	case ATOMIC_TYPE_ULONG:
		return mode_Lu;
	case ATOMIC_TYPE_LONGLONG:
		return mode_LLs;
	case ATOMIC_TYPE_ULONGLONG:
		return mode_LLu;
	case ATOMIC_TYPE_FLOAT:
		return mode_F;
	case ATOMIC_TYPE_DOUBLE:
		return mode_D;
	default:
		panic("Encountered unknown atomic type");
	}
}

static
ir_type *get_atomic_type(const atomic_type_t *type)
{
	ir_mode *mode   = get_atomic_mode(type);
	ident *id       = get_mode_ident(mode);
	ir_type *irtype = new_type_primitive(id, mode);

	return irtype;
}

static
ir_type *get_method_type(const method_type_t *method_type)
{
	type_t  *result_type  = method_type->result_type;

	ident   *id           = unique_id("methodtype");
	int      n_parameters = 0;
	int      n_results    = result_type->type == TYPE_VOID ? 0 : 1;
	ir_type *irtype       = new_type_method(id, n_parameters, n_results);

	if(result_type->type != TYPE_VOID) {
		ir_type *restype = get_ir_type(result_type);
		set_method_res_type(irtype, 0, restype);
	}

	return irtype;
}

static
ir_type *get_ir_type(type_t *type)
{
	assert(type != NULL);

	if(type->firm_type != NULL) {
		return type->firm_type;
	}

	ir_type *firm_type = NULL;
	switch(type->type) {
	case TYPE_ATOMIC:
		firm_type = get_atomic_type((const atomic_type_t*) type);
		break;
	case TYPE_METHOD:
		firm_type = get_method_type((const method_type_t*) type);
		break;
	default:
		panic("unknown type");
	}

	type->firm_type = firm_type;
	return firm_type;
}

static inline
ir_mode *get_ir_mode(type_t *type)
{
	ir_type *irtype = get_ir_type(type);
	ir_mode *mode   = get_type_mode(irtype);
	assert(mode != NULL);
	return mode;
}

static
ir_entity* get_method_entity(method_t *method)
{
	if(method->entity != NULL)
		return method->entity;

	method_type_t  *method_type    = method->type;

	if(method_type->abi_style != NULL) {
		fprintf(stderr, "Warning: ABI Style '%s' unknown\n",
		        method_type->abi_style);
	}

	/* first create an entity */
	ir_type *global_type    = get_glob_type();
	ident   *id             = new_id_from_str(method->symbol->string);
	ir_type *ir_method_type = get_ir_type((type_t*) method_type);

	ir_entity *entity       = new_entity(global_type, id, ir_method_type);
	set_entity_ld_ident(entity, id);
	set_entity_visibility(entity, visibility_external_visible);

	method->entity = entity;
	return entity;
}

static
ir_entity* get_extern_method_entity(extern_method_t *method)
{
	if(method->entity != NULL)
		return method->entity;

	method_type_t *method_type = method->type;

	if(method_type->abi_style != NULL) {
		fprintf(stderr, "Warning: ABI Style '%s' unknown\n",
		        method_type->abi_style);
	}

	/* first create an entity */
	ir_type *global_type    = get_glob_type();
	ident   *id             = new_id_from_str(method->symbol->string);
	ir_type *ir_method_type = get_ir_type((type_t*) method_type);

	ir_entity *entity       = new_entity(global_type, id, ir_method_type);
	set_entity_ld_ident(entity, id);
	set_entity_visibility(entity, visibility_external_allocated);

	method->entity = entity;
	return entity;
}

static
ir_node *expression_to_firm(const expression_t *expression);

static
ir_node *int_const_to_firm(const int_const_t *cnst)
{
	ir_mode *mode = get_ir_mode(cnst->expression.datatype);
	tarval *tv    = new_tarval_from_long(cnst->value, mode);

	return new_Const(mode, tv);
}

static
ir_node *variable_reference_to_firm(const reference_expression_t *ref)
{
	variable_declaration_statement_t *variable = ref->variable;
	ir_mode                          *mode     = get_ir_mode(variable->type);

	value_numbers[variable->value_number] = variable;
	
	return get_value(variable->value_number, mode);
}

static
ir_node *assign_expression_to_firm(const binary_expression_t *assign)
{
	const expression_t *left  = assign->left;
	const expression_t *right = assign->right;

	assert(left->type == EXPR_REFERENCE_VARIABLE);
	const reference_expression_t *ref = (const reference_expression_t*) left;
	variable_declaration_statement_t *variable = ref->variable;

	value_numbers[variable->value_number] = variable;

	ir_node *val = expression_to_firm(right);
	
	set_value(variable->value_number, val);

	return val;
}

static
ir_op *binexpr_type_to_op(binexpr_type_t type)
{
	switch(type) {
	case BINEXPR_ADD:
		return op_Add;
	case BINEXPR_SUB:
		return op_Sub;
	case BINEXPR_MUL:
		return op_Mul;
	case BINEXPR_AND:
		return op_And;
	case BINEXPR_OR:
		return op_Or;
	case BINEXPR_XOR:
		return op_Eor;
	case BINEXPR_SHIFTLEFT:
		return op_Shl;
	case BINEXPR_SHIFTRIGHT:
		return op_Shr;
	default:
		return NULL;
	}
}

static
ir_node *binary_expression_to_firm(const binary_expression_t *binexpr)
{
	ir_op   *irop;

	switch(binexpr->binexpr_type) {
	case BINEXPR_ASSIGN:
		return assign_expression_to_firm(binexpr);
	/* TODO construct Div,Mod nodes (they need special treatment with memory
	 * edges */
	default:
		irop = binexpr_type_to_op(binexpr->binexpr_type);
		if(irop != NULL) {
			ir_node *in[2] = {
				expression_to_firm(binexpr->left),
				expression_to_firm(binexpr->right)
			};
			ir_mode *mode  = get_ir_mode(binexpr->expression.datatype);
			ir_node *block = get_irg_current_block(current_ir_graph);
			ir_node *node  = new_ir_node(NULL, current_ir_graph, block,
			                             irop, mode, 2, in);
			return node;
		}
	}

	return NULL;
}

static
ir_node *cast_expression_to_firm(const cast_expression_t *cast)
{
	ir_node *node      = expression_to_firm(cast->value);
	ir_type *dest_type = get_ir_type(cast->expression.datatype);
	ir_node *cast_node = new_Cast(node, dest_type);

	return cast_node;
}

static
ir_node *method_reference_to_firm(const reference_expression_t *ref)
{
	ir_entity *entity = get_method_entity(ref->method);
	ir_node *symconst = new_SymConst((union symconst_symbol) entity,
	                                 symconst_addr_ent);

	return symconst;
}

static
ir_node *extern_method_reference_to_firm(const reference_expression_t *ref)
{
	ir_entity *entity = get_extern_method_entity(ref->extern_method);
	ir_node *symconst = new_SymConst((union symconst_symbol) entity,
	                                 symconst_addr_ent);

	return symconst;
}

static
ir_node *call_expression_to_firm(const call_expression_t *call)
{
	ir_node       *result         = NULL;
	expression_t  *method         = call->method;
	assert(method->datatype->type == TYPE_METHOD);
	method_type_t *method_type    = (method_type_t*) method->datatype;
	ir_type       *ir_method_type = get_ir_type((type_t*) method_type);
	
	ir_node       *callee         = expression_to_firm(method);
	int            n_parameters   = 0;

	ir_node *store = get_store();
	ir_node *node  = new_Call(store, callee, n_parameters, NULL,
	                          ir_method_type);
	ir_node *mem   = new_Proj(node, mode_M, pn_Call_M_regular);
	set_store(mem);

	type_t        *result_type    = method_type->result_type;
	if(result_type->type != TYPE_VOID) {
		ir_mode *mode    = get_ir_mode(result_type);
		ir_node *resproj = new_Proj(node, mode_T, pn_Call_T_result);
		         result  = new_Proj(resproj, mode, 0);
	}

	return result;
}

static
ir_node *expression_to_firm(const expression_t *expression)
{
	switch(expression->type) {
	case EXPR_INT_CONST:
		return int_const_to_firm((const int_const_t*) expression);
	case EXPR_REFERENCE_VARIABLE:
		return variable_reference_to_firm(
				(const reference_expression_t*) expression);
	case EXPR_REFERENCE_METHOD:
		return method_reference_to_firm(
		        (const reference_expression_t*) expression);
	case EXPR_REFERENCE_EXTERN_METHOD:
		return extern_method_reference_to_firm(
		        (const reference_expression_t*) expression);
	case EXPR_BINARY:
		return binary_expression_to_firm(
				(const binary_expression_t*) expression);
	case EXPR_CAST:
		return cast_expression_to_firm(
				(const cast_expression_t*) expression);
	case EXPR_CALL:
		return call_expression_to_firm((const call_expression_t*) expression);
	default:
		assert(0);
	}
	return NULL;
}

static
void return_statement_to_firm(const return_statement_t *statement)
{
	ir_node *ret;
	if(statement->return_value != NULL) {
		ir_node *retval = expression_to_firm(statement->return_value);
		ir_node *in[1];

		in[0] = retval;
		ret = new_Return(get_store(), 1, in);
	} else {
		ret = new_Return(get_store(), 0, NULL);
	}
	ir_node *end_block = get_irg_end_block(current_ir_graph);
	add_immBlock_pred(end_block, ret);
}

static
void expression_statement_to_firm(const expression_statement_t *statement)
{
	expression_to_firm(statement->expression);
}

static
void statement_to_firm(const statement_t *statement);

static
void block_statement_to_firm(const block_statement_t *block)
{
	const statement_t *statement = block->first_statement;
	while(statement != NULL) {
		statement_to_firm(statement);
		statement = statement->next;
	}
}

static
void statement_to_firm(const statement_t *statement)
{
	switch(statement->type) {
	case STATEMENT_BLOCK:
		block_statement_to_firm((const block_statement_t*) statement);
		return;
	case STATEMENT_RETURN:
		return_statement_to_firm((const return_statement_t*) statement);
		return;
	case STATEMENT_VARIABLE_DECLARATION:
		/* nothing to do */
		break;
	case STATEMENT_EXPRESSION:
		expression_statement_to_firm((const expression_statement_t*) statement);
		break;
	default:
		assert(0);
	}
}

static
void create_method(method_t *method)
{
	ir_entity *entity = get_method_entity(method);

	ir_graph *irg = new_ir_graph(entity, method->n_local_vars);

	assert(value_numbers == NULL);
	value_numbers = xmalloc(method->n_local_vars * sizeof(value_numbers[0]));

	statement_to_firm(method->statement);

	mature_immBlock(get_irg_current_block(irg));
	mature_immBlock(get_irg_end_block(irg));

	irg_finalize_cons(irg);

	irg_vrfy(irg);

	free(value_numbers);
	value_numbers = NULL;

	dump_ir_block_graph(irg, "-test");
}

/**
 * Build a firm representation of an AST programm
 */
void ast2firm(namespace_t *namespace)
{
	/* scan compilation unit for functions */
	namespace_entry_t *entry = namespace->first_entry;
	while(entry != NULL) {
		switch(entry->type) {
		case NAMESPACE_ENTRY_METHOD:
			create_method((method_t*) entry);
			break;
		case NAMESPACE_ENTRY_VARIABLE:
			fprintf(stderr, "Global vars not handled yet\n");
			break;
		case NAMESPACE_ENTRY_EXTERN_METHOD:
			break;
		default:
			panic("Unknown namespace entry type found");
		}

		entry = entry->next;
	}
}

