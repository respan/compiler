#pragma once
#ifndef COMPILER_PARSER_H_
#define COMPILER_PARSER_H_
#include "statement.h"
#include <sstream>

class Parser
{
   Scanner scan_;
   std::ofstream &output_;
   std::shared_ptr<SymTable> table_;
   int double_count_, string_count_;

   std::string get_message(const std::shared_ptr<Symbol> &t);
   std::shared_ptr<Expr> constant_folding(const std::shared_ptr<Expr> &e1, const std::shared_ptr<Expr> &e2, int sign, bool is_unary = false);
   void type_match_error(const std::shared_ptr<SymType> &t1, const std::shared_ptr<SymType> &t2, size_t line);
   void is_type_equals(const std::shared_ptr<Expr> &e1, const std::shared_ptr<Expr> &e2, size_t line, bool is_arithmetic = false);
   void logical_op_error(const Token &op);
   void make_node(std::shared_ptr<Expr> &left, const std::shared_ptr<Expr> &right, const Token &op);
   void parse_stmt_and_push_to_block(const std::shared_ptr<SymTable> &sym_table, const std::shared_ptr<Block> &block, LexemeType type);

   std::shared_ptr<Expr> parse_expr(const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Expr> parse_term(const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Expr> parse_factor(const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Expr> parse_unary(const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Expr> parse_rel(const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Expr> parse_assignment(const Token &ident, const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Expr> parse_ident(std::shared_ptr<Symbol> ident, const std::shared_ptr<SymTable> &sym_table, const std::shared_ptr<SymTable> &param_table, bool is_par = false);
   std::shared_ptr<Expr> parse_function_call(const Token &ident, const std::shared_ptr<SymTable> &sym_table);

   std::shared_ptr<Statement> parse_block(const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Statement> parse_stmt(const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Statement> parse_while(const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Statement> parse_repeat(const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Statement> parse_if(const std::shared_ptr<SymTable> &sym_table);
   std::shared_ptr<Statement> parse_for(const std::shared_ptr<SymTable> &sym_table);   
   std::shared_ptr<Statement> parse_write_read(const Token &ident, const std::shared_ptr<SymTable> &sym_table);

   std::shared_ptr<SymType> parse_type();
   void parse_declaration();
   void parse_type_declaration();
   int parse_var_decalration(const std::shared_ptr<SymTable> &sym_table, bool is_proc);
   std::list<std::shared_ptr<SynVar>> parse_params(const std::shared_ptr<SymTable> &sym_table, int &os);
   
   
public:
   Parser(Scanner &s, std::ofstream &o);
   ~Parser() {}
   std::shared_ptr<SynObj> parse();
   void print_table();
   void generate(const std::shared_ptr<Generator> &gen);
};

#endif
