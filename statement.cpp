#include "statement.h"

void Block::push_stmt(const std::shared_ptr<Statement> &s)
{
   body_.push_back(s);
}

void Block::print(std::ofstream &output, int depth)
{
   int i;
   for(i = 0; i < depth; i++)
      output << ' ';
   output << "begin" << std::endl;
   for each(const auto& it in body_)
      it->print(output, depth + 5);
   for(i = 0; i < depth; i++) 
      output << ' ';
   output << "end\n";
}

void Block::generate(const std::shared_ptr<Generator> &gen)
{
   for each(const auto& it in body_)
      it->generate(gen);
}

void ExprStmt::print(std::ofstream &output, int depth)
{
   et_->print(output, depth);
}

void ExprStmt::generate(const std::shared_ptr<Generator> &gen)
{
   et_->generate(gen);
}

void BreakStmt::print(std::ofstream &output, int depth)
{
   for(int i = 0; i < depth; i++)
      output << ' ';
   output << "break\n";
}

void BreakStmt::generate(const std::shared_ptr<Generator> &gen)
{
   if (gen->is_cycle())
      gen->push(Instruction(cmd_jmp, op_label, gen->get_end_of_cycle()));
}

void ContinueStmt::print(std::ofstream &output, int depth)
{
   for(int i = 0; i < depth; i++)
      output << ' ';
   output << "continue\n";
}

void ContinueStmt::generate(const std::shared_ptr<Generator> &gen)
{
   if (gen->is_cycle())
      gen->push(Instruction(cmd_jmp, op_label, gen->get_begin_of_cycle()));
}

void WhileStmt::print(std::ofstream &output, int depth)
{
   for(int i = 0; i < depth; i++)
      output << ' ';

   output << "while\n";
   expr_->print(output, depth);

   for(int i = 0; i < depth; i++)
      output<<' ';

   output << "do\n";
   stmt_->print(output, depth + 5);
}

void WhileStmt::generate(const std::shared_ptr<Generator> &gen)
{
   std::string label_begin = gen->generate_label();
   std::string label_end = gen->generate_label();
   std::string tmp1, tmp2;

	gen->push_label(label_begin);

   if (gen->is_cycle())
   {
      tmp1 = gen->get_begin_of_cycle();
      tmp2 = gen->get_end_of_cycle();
   }

   gen->set_cycle(true);
   gen->set_cycle_begin(label_begin);
   gen->set_cycle_end(label_end);

   expr_->generate(gen);
   gen->generate_pop_test();
   gen->push(Instruction(cmd_jz, op_label, label_end));

   stmt_->generate(gen);   
   gen->push(Instruction(cmd_jmp, op_label, label_begin));
   gen->push_label(label_end);

   if (tmp1.length() && tmp2.length())
   {
      gen->set_cycle_begin(tmp1);
      gen->set_cycle_end(tmp2);
   }
   else
      gen->set_cycle(false);
}

void RepeatStmt::print(std::ofstream &output, int depth)
{
   for(int i = 0; i < depth; ++i) 
      output << ' ';

   output << "repeat\n";
   stmt_->print(output, depth + 5);

   for(int i = 0; i < depth; ++i) 
      output << ' ';

   output << "until\n";
   expr_->print(output, depth);
}

void RepeatStmt::generate(const std::shared_ptr<Generator> &gen)
{
	std::string label_begin = gen->generate_label();
	std::string label_condition = gen->generate_label();
   std::string label_end = gen->generate_label();
   std::string tmp1, tmp2;

	gen->push_label(label_begin);

   if (gen->is_cycle())
   {
      tmp1 = gen->get_begin_of_cycle();
      tmp2 = gen->get_end_of_cycle();
   }

	gen->set_cycle(true);
	gen->set_cycle_begin(label_begin);
	gen->set_cycle_end(label_end);

	stmt_->generate(gen);
	gen->push_label(label_condition);
	expr_->generate(gen);

	gen->generate_pop_test();
	gen->push(Instruction(cmd_jz, op_label, label_begin));
	gen->push_label(label_end);

   if (tmp1.length() && tmp2.length())
   {
      gen->set_cycle_begin(tmp1);
      gen->set_cycle_end(tmp2);
   }
   else
      gen->set_cycle(false);
}

void IfStmt::print(std::ofstream &output, int depth)
{
   for(int i = 0; i < depth; ++i) 
      output << ' ';

   output << "if\n";
   condition_->print(output, depth);

   for(int i = 0; i < depth; ++i)
      output << ' ';

   output << "then\n";
   if_stmt_->print(output, depth + 5);

   for(int i = 0; i < depth; ++i) 
      output << ' ';

   if (else_stmt_) 
      output << "else\n";

   else_stmt_->print(output, depth + 5);
}

void IfStmt::generate(const std::shared_ptr<Generator> &gen)
{
	condition_->generate(gen);

	std::string label_else = gen->generate_label();
	std::string label_exit = gen->generate_label();

	gen->generate_pop_test();
	gen->push(Instruction(cmd_jz, op_label, label_else));

	if_stmt_->generate(gen);

	gen->push(Instruction(cmd_jmp, op_label, label_exit));
	gen->push_label(label_else);

	else_stmt_->generate(gen);

	gen->push_label(label_exit);
}

void ForStmt::print(std::ofstream &output, int depth)
{
   for(int i = 0; i < depth; ++i)
      output << ' ';

   output << "for\n";
   expr1_->print(output, depth);

   for(int i = 0; i < depth; ++i)
      output << ' ';

   output << t_.get_string() << '\n';
   expr2_->print(output, depth);

   for(int i = 0; i < depth; ++i)
      output << ' ';

   output << "do\n";
   stmt_->print(output, depth);
}

void ForStmt::generate(const std::shared_ptr<Generator> &gen)
{
   std::string	label_begin = gen->generate_label();
   std::string label_end = gen->generate_label();
   std::string label_condition = gen->generate_label();
   std::string label_iter = gen->generate_label();
   std::string tmp1, tmp2;

   expr2_->generate(gen);
   expr1_->generate(gen);

   gen->push(Instruction(cmd_push, op_register, "esi"));
   gen->push(Instruction(cmd_jmp, op_label, label_condition));

   if (gen->is_cycle())
   {
      tmp1 = gen->get_begin_of_cycle();
      tmp2 = gen->get_end_of_cycle();
   }

   gen->push_label(label_begin);
   gen->set_cycle(true);
   gen->set_cycle_begin(label_iter);
   gen->set_cycle_end(label_end);

   stmt_->generate(gen);

   gen->push_label(label_iter);
   gen->push(Instruction(cmd_mov, op_register, "esi", op_memory, "[esp]"));
   
   if (t_.type() == to_stmt)
      gen->push(Instruction(cmd_inc, op_memory, "dword ptr [esi]"));
   else 
      gen->push(Instruction(cmd_dec, op_memory, "dword ptr [esi]"));
   
   gen->push_label(label_condition);
   gen->push(Instruction(cmd_mov, op_register, "eax", op_memory, "dword ptr [esi]"));
   gen->push(Instruction(cmd_cmp, op_register, "eax", op_memory, "dword ptr [esp + 4]"));

   if (t_.type() == to_stmt)
      gen->push(Instruction(cmd_jle, op_label, label_begin));
   else 
      gen->push(Instruction(cmd_jge, op_label, label_begin));

   gen->push_label(label_end);
   gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "8"));

   if (tmp1.length() && tmp2.length())
   {
      gen->set_cycle_begin(tmp1);
      gen->set_cycle_end(tmp2);
   }
   else
      gen->set_cycle(false);
}

WriteCall::WriteCall(const std::list<std::shared_ptr<Expr>> &expr_list, bool l) : ln_(l)
{
   for each(const auto& it in expr_list)
      args_.push_back(it);
}

void WriteCall::print(std::ofstream &output, int depth) 
{
   for(int i = 0; i < depth; ++i)
      output << ' ';
   output << "write" << (ln_ ? "ln" : "") << "\n"; 
   for each(const auto& it in args_)
      it->print(output, depth + 5);
}

void WriteCall::generate(const std::shared_ptr<Generator> &gen)
{
   for each(const auto& it in args_)
      switch(it->get_type()->get_sym_type())
      {
         case sym_int:
            if (it->get_syn_type() == syn_var)
            {
               gen->push(Instruction(cmd_call, op_memory, "printf, offset int_frmt, v_" + it->get_string()));
               gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "8"));
            }
            else
            {
               it->generate(gen);
               it->pop_val(gen);
               if (it->is_string())
               {
                  gen->push(Instruction(cmd_call, op_memory, "printf, esi"));
                  gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "4"));
               }
               else 
               {
                  gen->push(Instruction(cmd_call, op_memory, "printf, offset int_frmt, eax"));
                  gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "8"));
               }
            }
            break;
         default:
            if (it->get_syn_type() == syn_var)
               gen->push(Instruction(cmd_call, op_memory, "printf, offset double_frmt, v_" + it->get_string()));
            else
            {
               it->generate(gen);
               gen->push(Instruction(cmd_mov, op_register, "eax", op_immediate, "offset double_buff"));
               gen->push(Instruction(cmd_pop, op_immediate, "qword ptr [eax]"));
               gen->push(Instruction(cmd_call, op_memory, "printf, offset double_frmt, double_buff"));
            }
            gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "12"));
            break;
      }
   if (ln_)
   {
      gen->push(Instruction(cmd_call, op_memory, "printf, offset new_line"));
      gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "4"));
   }
}

ReadCall::ReadCall(const std::list<std::shared_ptr<Expr>> &expr_list, bool l) : ln_(l)
{
   for each(const auto& it in expr_list)
      args_.push_back(it);
}

void ReadCall::print(std::ofstream &output, int depth)
{
   for(int i = 0; i < depth; ++i)
      output << ' ';
   output << "read" << (ln_ ? "ln" : "") << "\n";
   for each(const auto& it in args_)
      it->print(output, depth + 5);
}

size_t SymProc::get_size_args()
{
   size_t argsize = 0;
   for each(auto it in args)
   {
      if (it->get_sym_var()->is_var_arg())
         argsize += sizeof(size_t);
      else
         argsize += it->get_type()->get_size();
   }
   return argsize;
}

std::shared_ptr<SynVar> SymProc::get_arg(size_t num)
{
   if(num < args.size())
   {
      auto it = args.begin();
      for(size_t count = 0; it != args.end(), count != num; ++it, ++count);
      return *it;
   }
   return nullptr;
}

const std::list<std::shared_ptr<SynVar>> &SymProc::get_arg_list()
{
   return args;
}

void SymProc::set_block(const std::shared_ptr<Statement> &pb)
{
   block_ = pb;
}

void SymProc::generate(const std::shared_ptr<Generator> &gen)
{
   gen->push_string("\npr_" + name_ + " proc near\n");
   gen->push(Instruction(cmd_push, op_register, "ebp"));
   gen->push(Instruction(cmd_mov, op_register, "ebp", op_register, "esp"));
   gen->push(Instruction(cmd_sub, op_register, "esp", op_immediate, get_size_local_args()));

   block_->generate(gen);

   gen->push(Instruction(cmd_mov, op_register, "esp", op_register, "ebp"));
   gen->push(Instruction(cmd_pop, op_register, "ebp"));
   gen->push(Instruction(cmd_ret));
   gen->push_string("pr_" + name_ + " endp\n");
}

FunCall::FunCall(const std::list<std::shared_ptr<Expr>> &lar, const Token &n, const std::shared_ptr<SymProc> &st): Expr(std::make_shared<SymType>()), name_(n), type_(st)
{
   arg_ = lar;
}

void FunCall::print(std::ofstream &output, int depth)
{
   for(int i = 0; i < depth; ++i)
      output << ' ';
   output << name_.get_string() << '\n';
   for each(const auto& it in arg_)
      it->print(output, depth + 5);
}

void FunCall::generate_base(const std::shared_ptr<Generator> &gen)
{
   gen->push(Instruction(cmd_sub, op_register,"esp", op_immediate, type_->get_size_ret_value()));
   auto i = type_->get_arg_list().begin();   
   for each (auto j in arg_)
   {
      if ((*i)->get_sym_var()->is_var_arg())
         j->generate_lvalue(gen);
      else 
         j->generate(gen);
      ++i;
   }
   gen->push(Instruction(cmd_call, op_memory, "pr_" + name_.get_string()));
}

void FunCall::pop_val(const std::shared_ptr<Generator> &gen)
{
   if (type_->get_size_ret_value() == 4)
      gen->push(Instruction(cmd_pop, op_register, "eax"));
}

void FunCall::generate(const std::shared_ptr<Generator> &gen)
{
   generate_base(gen);
   gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, type_->get_size_args()));
}