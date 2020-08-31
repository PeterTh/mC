#pragma once

#include "basics.h"
#include "ast.h"

namespace parser {

	// main interfaces

	sptr<ast::node> parse(const string& input);
	sptr<ast::expression> parse_expr(const string& input);

	// state

	struct parser_state;

	class scope {
		std::map<string, sptr<ast::variable>> storage;
	public:
		void declare(parser_state& p, string name, sptr<ast::variable> var);
		sptr<ast::variable> lookup(string name) const;
	};

	struct parser_state {
		string::const_iterator beginning;
		string::const_iterator s;
		string::const_iterator e;
		std::vector<scope> scopes;
		parser_state(string::const_iterator s, string::const_iterator e);
		void set_string(const string& to_parse);
	};

	struct parser_error {
		parser_state state;
		string message;
		parser_error(parser_state&& state, const string& message) : state(std::move(state)), message(message) {}
	};

	// TYPES ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	sptr<ast::type> type(parser_state& p);

	// EXPRESSIONS /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// terminals

	sptr<ast::int_literal> int_literal(parser_state& p);
	sptr<ast::float_literal> float_literal(parser_state& p);
	sptr<ast::variable> variable(parser_state& p);
	sptr<ast::binary_operand> binary_operand(parser_state& p);
	sptr<ast::unary_operand> unary_operand(parser_state& p);

	// non-terminals

	sptr<ast::literal> literal(parser_state& p);

	sptr<ast::binary_operation> binary_operation(parser_state& p);
	sptr<ast::unary_operation> unary_operation(parser_state& p);

	sptr<ast::paren_expr> paren_expr(parser_state& p);

	sptr<ast::expression> single_expression(parser_state& p);
	sptr<ast::expression> expression(parser_state& p);

	// STATEMENTS //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	sptr<ast::expr_stmt> expr_stmt(parser_state& p);
	sptr<ast::compound_stmt> compound_stmt(parser_state& p);
	sptr<ast::if_stmt> if_stmt(parser_state& p);
	sptr<ast::decl_stmt> decl_stmt(parser_state& p);
	sptr<ast::statement> statement(parser_state& p);
}
