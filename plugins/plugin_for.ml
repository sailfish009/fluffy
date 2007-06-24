struct ForStatement:
	statement       : Statement
	pre_expression  : Expression*
	loop_control    : Expression*
	step_expression : Expression*
	loop_body       : Statement*

var token_for     : int
var for_statement : unsigned int

instance AllocateOnAst ForStatement:
	func allocate() : ForStatement*:
		var res <- allocate_zero<$ForStatement>()
		res.statement.type <- for_statement
		return res

func parse_for_statement() : Statement*:
	puts("parsing a for...")
	var statement <- allocate<$ForStatement>()

	//assert(token.type = token_for)
	next_token()

	expect('(')
	if token.type /= ';':
		statement.pre_expression <- parse_expression()
	expect(';')
	statement.loop_control <- parse_expression()
	expect(';')
	if token.type /= ')':
		statement.step_expression <- parse_expression()
	expect(')')
	expect(':')

	statement.loop_body <- parse_statement()

	return cast<Statement* > statement

func lower_for_statement(statement : Statement*) : Statement*:
	var for_statement <- cast<ForStatement* > statement
	var loop_body     <- for_statement.loop_body

	/* lower&check semantics of inner expressions and statements */
	loop_body <- check_statement(loop_body)
	for_statement.pre_expression \
		<- check_expression(for_statement.pre_expression)
	for_statement.loop_control \
		<- check_expression(for_statement.loop_control)
	for_statement.step_expression \
		<- check_expression(for_statement.step_expression)

	var expression        <- allocate<$ExpressionStatement>()
	expression.expression <- for_statement.pre_expression

	var label             <- allocate<$LabelStatement>()

	var if_statement            <- allocate<$IfStatement>()
	if_statement.condition      <- for_statement.loop_control
	if_statement.true_statement <- loop_body

	var loop_body_block        <- cast<BlockStatement* > loop_body

	var step_expression        <- allocate<$ExpressionStatement>()
	step_expression.expression <- for_statement.step_expression
	block_append(loop_body_block, cast<Statement* > step_expression) 

	var goto_statement   <- allocate<$GotoStatement>()
	goto_statement.label <- label
	block_append(loop_body_block, cast<Statement* > goto_statement)

	var block                 <- allocate<$BlockStatement>()
	block.statements          <- cast<Statement* > expression
	expression.statement.next <- cast<Statement* > label
	label.statement.next      <- cast<Statement* > if_statement

	return cast<Statement* > block

func init_plugin():
	puts("init_plugin is here")
	token_for     <- register_new_token("for")
	for_statement <- register_statement()
	register_statement_parser(parse_for_statement, token_for)
	register_statement_lowerer(lower_for_statement, for_statement)

