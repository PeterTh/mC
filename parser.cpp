#include "parser.h"

#include <type_traits>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "string_utils.h"

// helper functions ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
	using namespace parser;

	void consume_whitespace(parser_state& p) {
		auto& s = p.s; auto e = p.e;
		while(s != e && (*s == ' ' || *s == '\t' || *s == '\n')) ++s;
	}

	bool is_identifier_start(char c) {
		return isalpha(c) || c=='_';
	}
	bool is_identifier_char(char c) {
		return is_identifier_start(c) || isdigit(c);
	}

	string consume_identifier(parser_state& p) {
		auto try_p = p;
		consume_whitespace(try_p);
		auto ident_start = try_p.s;
		if(try_p.s == try_p.e || !is_identifier_start(*try_p.s++)) return {};
		while(try_p.s != try_p.e && is_identifier_char(*try_p.s)) try_p.s++;
		string ret(ident_start, try_p.s);
		p = try_p;
		consume_whitespace(p);
		return ret;
	}

	template<class Last>
	string try_token(parser_state& p, Last last_in) {
		auto try_p = p;
		consume_whitespace(try_p);
		string last{last_in};
		auto string_it = last.cbegin();
		while(try_p.s != try_p.e && string_it != last.cend() && *try_p.s == *string_it) {
			try_p.s++;
			string_it++;
		}
		if(string_it == last.cend()) {
			p = try_p;
			consume_whitespace(p);
			return last;
		}
		return {};
	}
	template<class First, class ...Tries>
	string try_token(parser_state& p, First first, Tries... tries) {
		auto match = try_token(p, first);
		if(!match.empty()) return match;
		return try_token(p, tries...);
	}

	template<class Production, class Last>
	Production try_match(parser_state& p, Last last) {
		auto try_p = p;
		consume_whitespace(try_p);
		auto last_res = last(try_p);
		if(last_res) {
			p = try_p;
			consume_whitespace(p);
			return last_res;
		}
		return {};
	}
	template<class Production, class First, class ...Tries>
	Production try_match(parser_state& p, First first, Tries... tries) {
		auto match = try_match<Production, First>(p, first);
		if(match) return match;
		return try_match<Production>(p, tries...);
	}

	template<class Parser, class RetType = typename std::result_of<Parser(parser_state&)>::type>
	RetType expect(Parser pp, parser_state& p) {
		RetType ret = pp(p);
		if(!ret) {
			std::stringstream ss;
			ss << "Expected " << typeid(RetType).name();
			throw parser_error(p, ss.str());
		}
		return ret;
	}

	void report_parser_error(const parser_error& err) {
		auto p = err.state;
		// find beginning and end of error line
		auto ls = p.s;
		size_t char_index = 0;
		while(ls != p.beginning && *ls != '\n') {
			char_index += (*ls == '\t') ? 4 : 1;
			ls--;
		}
		auto le = p.s;
		while(le != p.e && *le != '\n') {
			le++;
		}
		// find line number of error
		auto line_num = std::count(p.beginning, ls, '\n') + 2;
		std::cerr << "Parsing error on line number " << line_num << ", column " << char_index << ":" << std::endl;
		auto line = string(ls+1, le);
		replace_all(line, "\t", "    ");
		std::cerr << "Context:\n" << line << std::endl;
		for(size_t i=0; i < char_index; ++i) std::cerr << " ";
		std::cerr << "^" << std::endl;
		std::cerr << "Message: " << err.message << std::endl;
	}
}


namespace parser {

	// state ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void scope::declare(const parser_state& p, string name, sptr<ast::variable> var) {
		if(storage.find(name) != storage.end()) throw parser_error(p, "Declaring variable with name which already exists in this scope");
		storage[name] = var;
	}

	sptr<ast::variable> scope::lookup(string name) const {
		auto it = storage.find(name);
		if(it == storage.end()) return{};
		return it->second;
	}

	sptr<ast::variable> lookup_variable(const parser_state& state, string name) {
		for(auto it = state.scopes.crbegin(); it != state.scopes.crend(); ++it) {
			auto var = it->lookup(name);
			if(var) return var;
		}
		return {};
	}

	parser_state::parser_state(string::const_iterator s, string::const_iterator e) : beginning(s), s(s), e(e) {
		scopes.push_back(scope());
	}

	void parser_state::set_string(const string& to_parse) {
		s = to_parse.cbegin();
		e = to_parse.cend();
	}

	// parsing /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	sptr<ast::node> parse(const string& input) {
		parser_state s{input.cbegin(), input.cend()};
		try {
			return try_match<sptr<ast::node>>(s, statement, expression);
		} catch(const parser_error& err) {
			report_parser_error(err);
			return {};
		}
	}

	sptr<ast::expression> parse_expr(const string& input) {
		parser_state s{input.cbegin(), input.cend()};
		return expression(s);
	}

	sptr<ast::type> type(parser_state& p) {
		auto t = try_token(p, "int", "float");
		if(!t.empty()) {
			if(t == "int") return std::make_shared<ast::int_type>();
			if(t == "float") return std::make_shared<ast::float_type>();
		}
		return {};
	}

	sptr<ast::int_literal> int_literal(parser_state& p) {
		auto try_p = p;
		while(try_p.s != try_p.e && isdigit(*try_p.s)) ++try_p.s;
		if(try_p.s != p.s) {
			auto ret = std::make_shared<ast::int_literal>(atoi(string{p.s,try_p.s}.c_str()));
			p = try_p;
			return ret;
		}
		return {};
	}

	sptr<ast::float_literal> float_literal(parser_state& p) {
		auto try_p = p;
		while(try_p.s != try_p.e && isdigit(*try_p.s)) ++try_p.s;
		if(try_p.s != try_p.e && *try_p.s == '.') ++try_p.s;
		else return {};
		while(try_p.s != try_p.e && isdigit(*try_p.s)) ++try_p.s;
		if(try_p.s != p.s) {
			auto ret = std::make_shared<ast::float_literal>(atof(string{p.s,try_p.s}.c_str()));
			p = try_p;
			return ret;
		}
		return {};
	}

	sptr<ast::variable> variable(parser_state& p) {
		auto try_p = p;
		auto identifier = consume_identifier(try_p);
		if(identifier.empty()) return {};
		auto var = lookup_variable(try_p, identifier);
		if(!var) return {};
		p = try_p;
		return var;
	}

	sptr<ast::literal> literal(parser_state& p) {
		return try_match<sptr<ast::literal>>(p, float_literal, int_literal);
	}

	sptr<ast::binary_operand> binary_operand(parser_state& p) {
		auto t = try_token(p, "+", "-", "*", "/", "==", "!=", "<=", ">=", "<", ">", "=");
		if(!t.empty()) {
			return std::make_shared<ast::binary_operand>(ast::binary_operand_map.find(t)->second);
		}
		return {};
	}

	sptr<ast::unary_operand> unary_operand(parser_state& p) {
		if(p.s == p.e) return{};
		sptr<ast::unary_operand> ret;
		switch(*p.s) {
		case '-': ret = std::make_shared<ast::unary_operand>(ast::unary_operand::MINUS); break;
		case '!': ret = std::make_shared<ast::unary_operand>(ast::unary_operand::NOT); break;
		}
		if(ret) p.s++;
		return ret;
	}

	sptr<ast::binary_operation> binary_operation(parser_state& p) {
		auto try_p = p;
		auto lhs = single_expression(try_p);
		if(!lhs) return {};
		consume_whitespace(try_p);
		auto op = binary_operand(try_p);
		if(!op) return {};
		consume_whitespace(try_p);
		auto rhs = expect(expression, try_p);
		if(!rhs) return {};
		p = try_p;
		return std::make_shared<ast::binary_operation>(op, lhs, rhs);
	}

	sptr<ast::unary_operation> unary_operation(parser_state& p) {
		auto try_p = p;
		auto op = unary_operand(try_p);
		if(!op) return {};
		consume_whitespace(try_p);
		auto sub = expect(expression, try_p);
		if(!sub) return {};
		p = try_p;
		return std::make_shared<ast::unary_operation>(op, sub);
	}

	sptr<ast::paren_expr> paren_expr(parser_state& p) {
		if(try_token(p, string("(")).empty()) return {};
		auto sub = expect(expression, p);
		if(try_token(p, string(")")).empty()) throw parser_error(p, "Expected ')' at the end of parenthesized expression");
		return std::make_shared<ast::paren_expr>(sub);
	}

	sptr<ast::expression> single_expression(parser_state& p) {
		return try_match<sptr<ast::expression>>(p, literal, variable, unary_operation, paren_expr);
	}

	sptr<ast::expression> expression(parser_state& p) {
		return try_match<sptr<ast::expression>>(p, binary_operation, single_expression);
	}

	sptr<ast::expr_stmt> expr_stmt(parser_state& p) {
		auto expr = expression(p);
		if(expr) {
			if(try_token(p, ";").empty()) throw parser_error(p, "Expected ';' at end of statement");
			return std::make_shared<ast::expr_stmt>(expr);
		}
		return {};
	}

	sptr<ast::compound_stmt> compound_stmt(parser_state& p) {
		if(try_token(p, "{").empty()) return {};
		p.scopes.push_back(scope());
		ast::stmt_list statements;
		while(auto stmt = statement(p)) statements.push_back(stmt);
		p.scopes.pop_back();
		if(try_token(p, "}").empty()) throw parser_error(p, "Expected '}' at end of compound statement");
		return std::make_shared<ast::compound_stmt>(statements);
	}

	sptr<ast::if_stmt> if_stmt(parser_state& p) {
		if(try_token(p, "if").empty()) return {};
		auto condition = expect(paren_expr, p)->sub;
		auto then_stmt = expect(statement, p);
		if(!try_token(p, "else").empty()) {
			auto else_stmt = expect(statement, p);
			return std::make_shared<ast::if_stmt>(condition, then_stmt, else_stmt);
		}
		return std::make_shared<ast::if_stmt>(condition, then_stmt);
	}

	sptr<ast::decl_stmt> decl_stmt(parser_state& p) {
		static int id = 0;
		auto var_type = type(p);
		if(!var_type) return {};
		auto ident = consume_identifier(p);
		if(ident.empty()) throw parser_error(p, "Expected identifier in variable declaration");
		auto var = std::make_shared<ast::variable>(var_type, ident, id++);
		auto init_eq = try_token(p, "=");
		sptr<ast::expression> init_expr;
		if(!init_eq.empty()) {
			init_expr = expect(expression, p);
		}
		if(try_token(p, ";").empty()) throw parser_error(p, "Expected ';' at end of statement");
		// store variable in current scope
		p.scopes.back().declare(p, ident, var);
		return std::make_shared<ast::decl_stmt>(var, init_expr);
	}

	sptr<ast::statement> statement(parser_state& p) {
		return try_match<sptr<ast::statement>>(p, if_stmt, decl_stmt, compound_stmt, expr_stmt);
	}

}
