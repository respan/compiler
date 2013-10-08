#pragma once
#ifndef COMPILER_EXPRESSION_H_
#define COMPILER_EXPRESSION_H_
#include "symbol.h"

enum SynTypes 
{
   syn_var, syn_array, syn_rec, syn_none, 
};

class SynObj
{
public:
   SynObj() {}
   virtual ~SynObj() {}
   virtual void print(std::ofstream &output, int depth = 0) = 0;
   virtual void generate(const std::shared_ptr<Generator> &gen) = 0;
};

class Expr: public SynObj
{
protected:
   std::shared_ptr<SymType> expr_type_;
public:
   Expr(const std::shared_ptr<SymType> &st): expr_type_(st) {}
   virtual ~Expr() {}
   virtual std::shared_ptr<SymType> get_type() const = 0;
   virtual void pop_val(const std::shared_ptr<Generator> &gen);
   virtual void generate_arg_rec(const std::shared_ptr<Generator> &gen) {}
   virtual void generate_lvalue(const std::shared_ptr<Generator> &gen) {}
   virtual bool is_string() const { return false; }
   virtual bool is_const() const { return false; }
   virtual std::string get_string() const { return ""; };
   virtual std::shared_ptr<Expr> get_right_expr() { return nullptr; }
   virtual void change_right_expr(std::shared_ptr<Expr> e) {}
   virtual void set_higher_priority() {}
   virtual bool is_higher_priority() const { return false; }
   virtual SynTypes get_syn_type() const { return syn_none; }
};

std::shared_ptr<SymType> choose_expr_type(const std::shared_ptr<Expr> &e1, const std::shared_ptr<Expr> &e2, bool is_arithmetic = false);
void print_obj(std::ofstream &output, int depth, const std::string &str);

class UnaryOp: public Expr 
{
   Token sign_;
   std::shared_ptr<Expr> expr_;
   bool is_const_;
public:
   UnaryOp(const std::shared_ptr<SymType> &st, const Token &t, const std::shared_ptr<Expr> &e, bool c = false): Expr(st), sign_(t), expr_(e), is_const_(c) {}
   ~UnaryOp() {}
   void print(std::ofstream &output, int depth = 0);
   std::shared_ptr<SymType> get_type() const { return expr_type_; }
   void generate(const std::shared_ptr<Generator> &gen);
   bool is_const() const { return is_const_; }
   std::string get_string() const { return expr_->get_string(); }
};

class BinaryOp: public Expr 
{
   Token token_;
   std::shared_ptr<Expr> left_, right_;
   bool in_brackets;
public:
   BinaryOp(const std::shared_ptr<SymType> &st, const Token &t, const std::shared_ptr<Expr> &e1, const std::shared_ptr<Expr> &e2):
      Expr(st), token_(t), left_(e1), right_(e2), in_brackets(false) {}
   ~BinaryOp() {}
   void print(std::ofstream &output, int depth = 0);
   std::shared_ptr<SymType> get_type() const { return expr_type_; }
   void generate(const std::shared_ptr<Generator> &gen);
   std::shared_ptr<Expr> get_right_expr() { return right_; }
   void change_right_expr(std::shared_ptr<Expr> e) { right_ = e; expr_type_ = choose_expr_type(left_, right_); }
   void set_higher_priority() { in_brackets = true; }
   bool is_higher_priority() const { return in_brackets; }
};

class SynVar: public Expr
{
protected:
   std::string str_;
   std::shared_ptr<SymVar> var_;
public:
   SynVar(const std::string &s, const std::shared_ptr<SymVar> &v): Expr(std::make_shared<SymType>()), str_(s), var_(v) {}
   virtual ~SynVar() {}
   void print(std::ofstream &output, int depth = 0) { print_obj(output, depth, str_); }
   std::shared_ptr<SymType> get_type() const { return var_->get_type(); }
   std::shared_ptr<SymVar> get_sym_var() const { return var_; }
   SynTypes get_syn_type() const { return syn_var; }
   void generate(const std::shared_ptr<Generator> &gen);
   virtual void generate_base(const std::shared_ptr<Generator> &gen, bool flag = false);
   virtual void generate_index(const std::shared_ptr<Generator> &gen) {};
   void pop_val(const std::shared_ptr<Generator> &gen);
   void generate_arg_rec(const std::shared_ptr<Generator> &gen);
   void generate_lvalue(const std::shared_ptr<Generator> &gen);
   std::string get_string() const { return str_; }
};

class SynConstInt: public Expr 
{
   std::string str_;
public:
   SynConstInt(const std::string &s): Expr(std::make_shared<SymType>()), str_(s) {}
   ~SynConstInt() {}
   void SynConstInt::print(std::ofstream &output, int depth = 0) { print_obj(output, depth, str_); }
   std::shared_ptr<SymType> get_type() const { return std::make_shared<Int>("integer"); }
   void SynConstInt::pop_val(const std::shared_ptr<Generator> &gen) { gen->push(Instruction(cmd_pop, op_register, "eax")); }
   void SynConstInt::generate(const std::shared_ptr<Generator> &gen) { gen->push(Instruction(cmd_push, op_immediate, str_)); }
   bool is_const() const { return true; }
   std::string get_string() const { return str_; }
};

class SynConstDouble: public Expr 
{
   std::string str_;
   size_t num_;
public:
   SynConstDouble(const std::string &s, size_t n = 0): Expr(std::make_shared<SymType>()), str_(s), num_(n) {}
   ~SynConstDouble() {}
   void print(std::ofstream &output, int depth = 0) { print_obj(output, depth, str_); }
   std::shared_ptr<SymType> get_type() const { return std::make_shared<Double>("double"); }
   void pop_val(const std::shared_ptr<Generator> &gen) {}
   void generate(const std::shared_ptr<Generator> &gen) { gen->push(Instruction(cmd_push, op_memory, "qword ptr dc_" + boost::lexical_cast<std::string>(num_))); }
   bool is_const() const { return true; }
   std::string get_string() const { return str_; }
};

class SynConstStr: public Expr 
{
   std::string str_;
   size_t num_, len_;
public:
   SynConstStr(const std::string &s, size_t n = 0);
   ~SynConstStr() {}
   void print(std::ofstream &output, int depth);
   std::shared_ptr<SymType> get_type() const { return std::make_shared<Int>("integer"); }
   bool is_string() const { return true; }
   void pop_val(const std::shared_ptr<Generator> &gen) {}
   void generate(const std::shared_ptr<Generator> &gen);
};

class SynRec: public SynVar
{
   std::shared_ptr<SynVar> recn_, field_;
   std::shared_ptr<SymType> stype_;
public:
   SynRec(const std::string &n, const std::shared_ptr<SymVar> &v, const std::shared_ptr<SynVar> &e1, const std::shared_ptr<SynVar> &e2, const std::shared_ptr<SymType> &st):
      SynVar(n, v), recn_(e1), field_(e2), stype_(st) {}
   ~SynRec() {}
   void print(std::ofstream &output, int depth);
   SynTypes get_syn_type() const { return syn_rec; }
   std::shared_ptr<SymType> get_type() const { return stype_; }
   void generate(const std::shared_ptr<Generator> &gen);
   void generate_base(const std::shared_ptr<Generator> &gen, bool flag = false);
   void pop_val(const std::shared_ptr<Generator> &gen);
   void generate_arg_rec(const std::shared_ptr<Generator> &gen) {}
   void generate_lvalue(const std::shared_ptr<Generator> &gen);
};

class SynArray: public SynVar
{
   std::shared_ptr<SynVar> lp_;
   std::shared_ptr<SymType> el_type_;
   size_t dim_;
   std::list<std::shared_ptr<Expr>> indexes_;
public:
   SynArray(const std::string &nm, const std::shared_ptr<SynVar> &e, const std::list<std::shared_ptr<Expr>> &l,
      const std::shared_ptr<SymVar> &v, const std::shared_ptr<SymType> &st);
   ~SynArray() {}
   void print(std::ofstream &output, int depth);
   std::shared_ptr<SymType> get_type() const { return el_type_; }
   size_t get_size_k(size_t k);
   SynTypes get_syn_type() const { return syn_array; }
   void generate(const std::shared_ptr<Generator> &gen);
   void generate_base(const std::shared_ptr<Generator> &gen, bool flag = false);
   void generate_index(const std::shared_ptr<Generator> &gen);
   void pop_val(const std::shared_ptr<Generator> &gen);
   void generate_arg_rec(const std::shared_ptr<Generator> &gen) {}
   void generate_lvalue(const std::shared_ptr<Generator> &gen);
};

class EmptyExpr: public Expr
{
public:
   EmptyExpr(): Expr(std::make_shared<SymType>()) {}
   ~EmptyExpr() {}
   void print(std::ofstream &output, int depth) {};
   std::shared_ptr<SymType> get_type() const { return expr_type_; }
   void generate(const std::shared_ptr<Generator> &gen) {};
};

#endif