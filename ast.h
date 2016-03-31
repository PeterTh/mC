#pragma once

#include "basics.h"

#include <vector>
#include <map>
#include <iostream>

namespace ast {

	// abstract bases //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct node {
		virtual ~node() {}
		virtual bool operator==(const node& other) const = 0;
		virtual std::ostream& print_to(std::ostream& stream) const = 0;

		bool operator!=(const node& other) const;
	};

	struct type : public node {};

	struct expression : public node {};
	struct literal : public expression {};

	struct statement : public node {};

	// lists ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	using node_list = std::vector<sptr<node>>;
	using expr_list = std::vector<sptr<expression>>;
	using stmt_list = std::vector<sptr<statement>>;

	bool operator==(const node_list& lhs, const node_list& rhs);
	bool operator==(const expr_list& lhs, const expr_list& rhs);
	bool operator==(const stmt_list& lhs, const stmt_list& rhs);

	// TYPES ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct int_type : public type {
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct float_type : public type {
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	// EXPRESSIONS /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// terminals ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct int_literal : public literal {
		int value;
		int_literal(int value) : value(value) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};
	struct float_literal : public literal {
		double value;
		float_literal(double value) : value(value) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct variable : public expression {
		sptr<type> var_type;
		string name;
		int id;
		variable(sptr<type> var_type, string name, int id) : var_type(var_type), name(name), id(id) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	enum class binary_operand {
		ASSIGN, ADD, SUB, MUL, DIV,
		EQ, NE, LT, GT, LE, GE
	};

	static const std::map<string, ast::binary_operand> binary_operand_map {
		{"+", ast::binary_operand::ADD}, {"-", ast::binary_operand::SUB},
		{"*", ast::binary_operand::MUL}, {"/", ast::binary_operand::DIV},
		{"==", ast::binary_operand::EQ}, {"!=", ast::binary_operand::NE},
		{"<=", ast::binary_operand::LE}, {">=", ast::binary_operand::GE},
		{"<" , ast::binary_operand::LT}, {">" , ast::binary_operand::GT},
		{"=", ast::binary_operand::ASSIGN}
	};

	enum class unary_operand {
		MINUS,
		NOT
	};

	// non-terminals ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct binary_operation : public expression {
		sptr<binary_operand> op;
		sptr<expression> lhs;
		sptr<expression> rhs;
		binary_operation(sptr<binary_operand> op, sptr<expression> lhs, sptr<expression> rhs) : op(op), lhs(lhs), rhs(rhs) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct unary_operation : public expression {
		sptr<unary_operand> op;
		sptr<expression> sub;
		unary_operation(sptr<unary_operand> op, sptr<expression> sub) : op(op), sub(sub) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct paren_expr : public expression {
		sptr<expression> sub;
		paren_expr(sptr<expression> sub) : sub(sub) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	// STATEMENTS //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct expr_stmt : public statement {
		sptr<expression> sub;
		expr_stmt(sptr<expression> sub) : sub(sub) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct compound_stmt : public statement {
		stmt_list statements;
		compound_stmt(const stmt_list& statements) : statements(statements) {}
		compound_stmt() {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct if_stmt : public statement {
		sptr<expression> condition;
		sptr<statement> then_stmt;
		sptr<statement> else_stmt;
		if_stmt(sptr<expression> condition, sptr<statement> then_stmt, sptr<statement> else_stmt)
			: condition(condition), then_stmt(then_stmt), else_stmt(else_stmt) {}
		if_stmt(sptr<expression> condition, sptr<statement> then_stmt) : if_stmt(condition, then_stmt, std::make_shared<compound_stmt>()) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct decl_stmt : public statement {
		sptr<variable> var;
		sptr<expression> init_expr;
		decl_stmt(sptr<variable> var, sptr<expression> init_expr) : var(var), init_expr(init_expr) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};
}

bool operator==(const sptr<ast::node>& lhs, const sptr<ast::node>& rhs);

std::ostream& operator<<(std::ostream& stream, const ast::node& node);

template<class T>
std::ostream& operator<<(std::ostream& stream, const sptr<T>& node) {
	if(!node) return stream << "NULL";
	return stream << *node;
}
