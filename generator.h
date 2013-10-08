#pragma once
#ifndef COMPILER_GENERATOR_H_
#define COMPILER_GENERATOR_H_
#include <map>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include "token.h"

enum AsmCommands 
{
   cmd_push, cmd_pop, cmd_add, cmd_sub, cmd_mul, cmd_div, cmd_mov, cmd_idiv,
   cmd_ret, cmd_faddp, cmd_fsubp, cmd_fdivp, cmd_fmulp, cmd_fxch, cmd_fabs, cmd_fchs,
   cmd_fstp, cmd_fld, cmd_jz, cmd_jnz, cmd_jne, cmd_jg, cmd_jge, cmd_jl,
   cmd_jle, cmd_je, cmd_test, cmd_fstsw, cmd_lea, cmd_call, cmd_jmp, cmd_cmp,
   cmd_invoke, cmd_movsd, cmd_or, cmd_xor, cmd_and, cmd_imul, cmd_neg, cmd_inc,
   cmd_dec, cmd_fcompp, cmd_sahf, cmd_setg, cmd_setl, cmd_sete, cmd_setne, cmd_setle,
   cmd_setge, cmd_cdq, cmd_fild, cmd_sal, cmd_sar, cmd_seta, cmd_setb, cmd_setae,
   cmd_setbe, cmd_setz, cmd_wrlab, cmd_const_decl
};

enum AsmOperands
{
   op_null, op_register, op_memory, op_immediate, op_label,
};

class Operand
{
   AsmOperands type_;
   std::string name_;
public:
   Operand(AsmOperands t = op_null, const std::string &s = ""): type_(t), name_(s) {}
   ~Operand() {}
   const std::string get_name() const { return name_; }
   AsmOperands get_operand_type() const { return type_; }
   friend bool operator ==(std::shared_ptr<Operand> op1, std::shared_ptr<Operand> op2) { return (op1->type_ == op2->type_ && op1->name_ == op2->name_); }
   friend bool operator ==(std::shared_ptr<Operand> op1, AsmOperands t) { return op1->type_ == t; }
   friend bool operator ==(std::shared_ptr<Operand> op1, const std::string &n) { return op1->name_ == n; }
   friend bool operator !=(std::shared_ptr<Operand> op1, std::shared_ptr<Operand> op2) { return !(op1 == op2); }
   friend bool operator !=(std::shared_ptr<Operand> op1, AsmOperands t) { return !(op1 == t); }
   friend bool operator !=(std::shared_ptr<Operand> op1, const std::string &n) { return !(op1 == n); }
   void set_name(const std::string &n) { name_ = n; }
};

class Instruction
{
   AsmCommands cmd_;
   std::shared_ptr<Operand> first_op_, second_op_;
public:
   Instruction(AsmCommands c, AsmOperands t1 = op_null, const std::string &o1 = "", AsmOperands t2 = op_null, const std::string &o2 = "");
   Instruction(AsmCommands c, std::shared_ptr<Operand> op1, std::shared_ptr<Operand> op2 = nullptr);
   Instruction(AsmCommands c, AsmOperands t1, const std::string &o1, AsmOperands t2, size_t o2);
   ~Instruction() {}
   void write_command(std::ofstream &output) const;
   AsmCommands get_cmd() const { return cmd_; }
   std::shared_ptr<Operand> get_first() const { return first_op_; }
   std::shared_ptr<Operand> get_second() const { return second_op_; }
   friend bool operator ==(Instruction instr, AsmCommands c) { return instr.cmd_ == c; }
   friend bool operator !=(Instruction instr, AsmCommands c) { return !(instr == c); }
};

class Generator
{
   std::list<Instruction> commands_;
   size_t label_counter_;
   bool cycle_;
   std::string cycle_begin_, cycle_end_;
   void optimize();
   void delete_instr(std::list<Instruction>::iterator &it1, std::list<Instruction>::iterator &it2);
   void delete_instr(std::list<Instruction>::iterator &it1);
   bool is_jump(AsmCommands c);
public:
   Generator(): label_counter_(0), cycle_(false) {}
   ~Generator() {};
   void generate();
   void write_to_file(std::ofstream &output, bool opt);
   void push(Instruction i) { commands_.push_back(i); }
   void push_label(const std::string &s) { push(Instruction(cmd_wrlab, op_label, s)); }
   void push_string(const std::string &s) { push(Instruction(cmd_wrlab, op_null, s)); }
   void push_const_decl(const std::string &s1, const std::string &s2) { push(Instruction(cmd_const_decl, op_null, s1, op_null, s2)); }
   std::string generate_label() { return "l_" + boost::lexical_cast<std::string>(label_counter_++); }
   bool is_cycle() const { return cycle_; }
   const std::string &get_end_of_cycle() const { return cycle_end_; }
   const std::string &get_begin_of_cycle() const { return cycle_begin_; }
   void set_cycle(bool val) { cycle_ = val; }
   void set_cycle_begin(const std::string &lab_beg) { cycle_begin_ = lab_beg; }
   void set_cycle_end(const std::string &lab_end) { cycle_end_ = lab_end; }
   const Instruction& get_last_instr() const { return commands_.back(); }
   void pop_last_instr() { commands_.pop_back(); }
   void generate_double_arithmetic(LexemeType t);
   int generate_int_arithmetic(LexemeType t, bool use_bitwise_op, int i);
   void generate_setcc(LexemeType t, bool is_unsigned_cmp = false);
   void generate_pop_test();
};

#endif