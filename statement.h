#pragma once
#ifndef COMPILER_STATEMENT_H_
#define COMPILER_STATEMENT_H_
#include "expression.h"

class Statement: public SynObj
{
protected:
   std::shared_ptr<Statement> stmt_;
public:
   Statement(const std::shared_ptr<Statement> &s): stmt_(s) {}
   Statement() {}
   virtual ~Statement() {}
   virtual bool is_break_or_continue() { return false; }
};

class Block: public Statement
{
   std::list<std::shared_ptr<Statement>> body_;
public:
   Block(): Statement() {}
   ~Block() {}
   void push_stmt(const std::shared_ptr<Statement> &s);
   void print(std::ofstream &output, int depth);
   void generate(const std::shared_ptr<Generator> &gen);
};

class ExprStmt: public Statement 
{
   std::shared_ptr<Expr> et_;
public:
   ExprStmt(const std::shared_ptr<Expr> &e): et_(e) {}
   ~ExprStmt() {}
   void print(std::ofstream &output, int depth);
   void generate(const std::shared_ptr<Generator> &gen);
};

class BreakStmt: public Statement 
{
public:
   BreakStmt() {}
   ~BreakStmt() {}
   void print(std::ofstream &output, int depth);
   void generate(const std::shared_ptr<Generator> &gen);
   bool is_break_or_continue() { return true; }
};

class ContinueStmt: public Statement 
{
public:
   ContinueStmt() {}
   ~ContinueStmt() {}
   void print(std::ofstream &output, int depth);
   void generate(const std::shared_ptr<Generator> &gen);
   bool is_break_or_continue() { return true; }
};

class WhileStmt: public Statement 
{
   std::shared_ptr<Expr> expr_;
   std::shared_ptr<Statement> stmt_;
public:
   WhileStmt(const std::shared_ptr<Expr> &e, const std::shared_ptr<Statement> &s): expr_(e), stmt_(s) {}
   ~WhileStmt() {}
   void print(std::ofstream &output, int depth);
   void generate(const std::shared_ptr<Generator> &gen);
};

class RepeatStmt: public Statement 
{
   std::shared_ptr<Expr> expr_;
   std::shared_ptr<Statement> stmt_;
public:
   RepeatStmt(const std::shared_ptr<Expr> &e, const std::shared_ptr<Statement> &s): expr_(e), stmt_(s) {}
   ~RepeatStmt() {}
   void print(std::ofstream &output, int depth);
   void generate(const std::shared_ptr<Generator> &gen);
};

class IfStmt: public Statement 
{
   std::shared_ptr<Expr> condition_;
   std::shared_ptr<Statement> if_stmt_, else_stmt_;
public:
   IfStmt(const std::shared_ptr<Expr> &e, const std::shared_ptr<Statement> &s1, const std::shared_ptr<Statement> &s2):
      condition_(e), if_stmt_(s1), else_stmt_(s2) {}
   ~IfStmt() {}
   void print(std::ofstream &output, int depth);
   void generate(const std::shared_ptr<Generator> &gen);
};

class EmptyStmt: public Statement 
{
public:
   EmptyStmt() {}
   void print(std::ofstream &output, int depth) {}
   ~EmptyStmt() {}
   void generate(const std::shared_ptr<Generator> &gen) {}

};

class ForStmt: public Statement 
{
   std::shared_ptr<Expr> expr1_, expr2_;
   std::shared_ptr<Statement> stmt_;
   Token t_, it_;
   void generate_iter(const std::shared_ptr<Generator> &gen);
   void generate_cond(const std::shared_ptr<Generator> &gen);
public:
   ForStmt(const std::shared_ptr<Expr> &e1, const std::shared_ptr<Expr> &e2, const Token &t, const Token &it, const std::shared_ptr<Statement> &s):
      expr1_(e1), expr2_(e2), t_(t), it_(it), stmt_(s) {}
   ~ForStmt() {}
   void print(std::ofstream &output, int depth);
   void generate(const std::shared_ptr<Generator> &gen);
};

class WriteCall: public Statement 
{
   std::list<std::shared_ptr<Expr>> args_;
   bool ln_;
public:
   WriteCall(const std::list<std::shared_ptr<Expr>> &expr_list, bool l);
   ~WriteCall() {}
   void print(std::ofstream &output, int depth);
   void generate(const std::shared_ptr<Generator> &gen);
};

class ReadCall: public Statement
{
   std::list<std::shared_ptr<Expr> > args_;
   bool ln_;
public:
   ReadCall(const std::list<std::shared_ptr<Expr>> &expr_list, bool l);
   ~ReadCall() {}
   void print(std::ofstream &output, int depth);
   void generate(const std::shared_ptr<Generator> &gen) {}
};

class SymProc: public Symbol
{
protected:
   std::shared_ptr<Statement> block_;
   std::shared_ptr<SymTable> params_;
   std::list<std::shared_ptr<SynVar>> args;
   int lsize;
public:
   SymProc(const std::string &n, const std::shared_ptr<SymTable> &st, std::list<std::shared_ptr<SynVar>> farg = std::list<std::shared_ptr<SynVar>>::list(), int sz = 0):
      Symbol(n), params_(st), args(farg), lsize(sz) {}
   ~SymProc() {}
   size_t get_size_args();
   virtual size_t get_size_ret_value() { return 0; }
   int get_size_local_args() { return lsize; }
   std::shared_ptr<SynVar> get_arg(size_t num);
   virtual const std::list<std::shared_ptr<SynVar>> &get_arg_list();
   virtual std::shared_ptr<SymType> get_type() const { return std::make_shared<SymType>(); }
   bool is_proc() const { return true; }
   void set_block(const std::shared_ptr<Statement> &pb);
   void print_block(std::ofstream &output) { block_->print(output, 5); }
   void print_local_table(std::ofstream &output) { params_->print_var_table(output, true); }
   void generate(const std::shared_ptr<Generator> &gen);
   SymTypes get_sym_type() const { return sym_proc; }
};

class SymFunc: public SymProc 
{
   std::shared_ptr<SymType> type_;
public:
   SymFunc(const std::string &n, const std::shared_ptr<SymTable> &smt, const std::list<std::shared_ptr<SynVar>> &farg,
      const std::shared_ptr<SymType> &st, int sz = 0): SymProc(n, smt, farg, sz), type_(st) {}
   size_t get_size_ret_value() { return type_->get_size(); }
   const std::list<std::shared_ptr<SynVar>> &get_arg_list() { return args; }
   std::shared_ptr<SymType> get_type() const { return type_; }
   ~SymFunc() {}
};

class FunCall: public Expr 
{
   std::list<std::shared_ptr<Expr>> arg_;
   Token name_;
   std::shared_ptr<SymProc> type_;
   void generate_base(const std::shared_ptr<Generator> &gen);
public:
   FunCall(const std::list<std::shared_ptr<Expr>> &lar, const Token &n, const std::shared_ptr<SymProc> &st);
   ~FunCall() {}
   void print(std::ofstream &output, int depth);
   std::shared_ptr<SymType> get_type() const { return type_->get_type(); }
   void generate(const std::shared_ptr<Generator> &gen);
   void pop_val(const std::shared_ptr<Generator> &gen);
};

#endif
