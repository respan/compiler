#include "expression.h"

std::shared_ptr<SymType> choose_expr_type(const std::shared_ptr<Expr> &e1, const std::shared_ptr<Expr> &e2, bool is_arithmetic)
{
   return e1->get_type()->get_sym_type() == sym_double ? e1->get_type() : e2->get_type();
}

inline void print_obj(std::ofstream &output, int depth, const std::string &str)
{
   for(int i = 0; i < depth; ++i) 
      output << " ";
   output << str << std::endl;
}

void Expr::pop_val(const std::shared_ptr<Generator> &gen)
{
   if (expr_type_->get_sym_type() == sym_int)
      gen->push(Instruction(cmd_pop, op_register, "eax"));
}

void BinaryOp::print(std::ofstream &output, int depth)
{
   if (left_)
      left_->print(output, depth + 5);
   for(int i = 0; i < depth; ++i) 
      output << " ";
   output << token_.get_string() << std::endl;
   if (right_) 
      right_->print(output, depth + 5);
}

void BinaryOp::generate(const std::shared_ptr<Generator> &gen)
{
   switch(token_.type())
   {
      case assignment:
         right_->generate(gen);
         left_->generate_lvalue(gen);
         left_->generate_arg_rec(gen);         
         switch(get_type()->get_sym_type())
         {
            case sym_int:
               gen->push(Instruction(cmd_pop, op_register, "esi"));
               gen->push(Instruction(cmd_pop, op_memory, "dword ptr [esi]"));
               break;
            case sym_double:
               gen->push(Instruction(cmd_pop, op_register, "esi"));
               if (right_->get_type()->get_sym_type() == sym_int)
               {
                  gen->push(Instruction(cmd_fild, op_memory, "dword ptr [esp]"));
                  gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "4"));
               }
               else
               {
                  gen->push(Instruction(cmd_fld, op_memory, "qword ptr [esp]"));
                  gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "8"));
               }
               gen->push(Instruction(cmd_fstp, op_memory, "qword ptr [esi]"));
               break;
         }
         break;
      case lesser_equal:
      case greater_equal:
      case equal:
      case not_equal:
      case greater:
      case lesser:
         left_->generate(gen);
         right_->generate(gen);
         
         switch(get_type()->get_sym_type())
         {
            case sym_int:
               gen->push(Instruction(cmd_pop, op_register, "eax"));
               gen->push(Instruction(cmd_pop, op_register, "ecx"));
               gen->push(Instruction(cmd_cmp, op_register, "ecx", op_register, "eax"));
               gen->generate_setcc(token_.type());
               gen->push(Instruction(cmd_push, op_register, "eax"));
               break;
            case sym_double:
               gen->push(Instruction(cmd_fld, op_memory, "qword ptr [esp]"));
               gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "8"));
               gen->push(Instruction(cmd_fld, op_memory, "qword ptr [esp]"));
               gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "8"));
               gen->push(Instruction(cmd_fcompp));
               gen->push(Instruction(cmd_fstsw, op_register, "ax"));
               gen->push(Instruction(cmd_sahf));
               gen->generate_setcc(token_.type(), true);
               gen->push(Instruction(cmd_push, op_register, "eax"));
               break;
         }
         break;
      default:
      {
         int i = 0;
         bool use_bitwise_op = false;
         if (get_type()->get_sym_type() == sym_int && right_->is_const() && (token_.type() == mul_op || token_.type() == div_op))
         {
            i = boost::lexical_cast<int>(right_->get_string());
            use_bitwise_op = !(i & (i - 1));
            if (use_bitwise_op)
            {
               int j = 1, k = 0;
               while (j < i)
               {
                  j *= 2;
                  ++k;
               }
               i = k;
            }
            else
               right_->generate(gen);
         }
         else 
               right_->generate(gen);
         left_->generate(gen);
         switch(get_type()->get_sym_type())
         {
            case sym_int:
               gen->push(Instruction(cmd_pop, op_register, "eax"));               
               if (!use_bitwise_op)
                  gen->push(Instruction(cmd_pop, op_register, "ecx"));
               gen->push(Instruction(cmd_push, op_register, gen->generate_int_arithmetic(token_.type(), use_bitwise_op, i) ? "eax" : "edx"));
               break;
            case sym_double:
               gen->push(Instruction(cmd_fld, op_memory, "qword ptr [esp]"));
               gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, "8"));
               gen->push(Instruction(cmd_fld, op_memory, "qword ptr [esp]"));
               gen->generate_double_arithmetic(token_.type());
               gen->push(Instruction(cmd_fstp, op_memory, "qword ptr [esp]"));
               break;
         }
         break;
      }
   }
}

void UnaryOp::print(std::ofstream &output, int depth)
{
   for(int i = 0; i < depth; i++) 
      output << " ";
   output << sign_.get_string() << std::endl;
   expr_->print(output, depth + 5);
}

void UnaryOp::generate(const std::shared_ptr<Generator> &gen)
{
   expr_->generate(gen);
   switch(get_type()->get_sym_type())
   {
      case sym_int:
         gen->push(Instruction(cmd_pop, op_register, "eax"));
         switch(sign_.type())
         {
            case plus_op:
               break;
            case minus_op:
               gen->push(Instruction(cmd_neg, op_register, "eax"));
               gen->push(Instruction(cmd_push, op_register, "eax"));
               break;
            case not_op:
               gen->push(Instruction(cmd_test, op_register, "al", op_register, "al"));
               gen->push(Instruction(cmd_setz, op_register, "al"));
               gen->push(Instruction(cmd_push, op_register, "eax"));
               break;
         }
         break;
      case sym_double:
         gen->push(Instruction(cmd_fld, op_memory, "qword ptr [esp]"));
         switch(sign_.type())
         {
            case plus_op:
               gen->push(Instruction(cmd_fabs));
               break;
            case minus_op:
               gen->push(Instruction(cmd_fchs));
               break;
         }
         gen->push(Instruction(cmd_fstp, op_memory, "qword ptr [esp]"));
         break;
   }
}

void SynVar::generate(const std::shared_ptr<Generator> &gen)
{
   generate_base(gen);
   gen->push(Instruction(cmd_pop, op_register, "esi"));
   if(var_->get_type()->get_sym_type() == sym_array || var_->get_type()->get_sym_type() == sym_record)
   {      
      gen->push(Instruction(cmd_sub, op_register, "esp", op_immediate, var_->get_type()->get_size()));
      gen->push(Instruction(cmd_mov, op_register, "edi", op_register, "esp"));
      gen->push(Instruction(cmd_mov, op_register, "ecx", op_immediate, var_->get_number_of_elements()));
      gen->push_string("rep ");
      gen->push(Instruction(cmd_movsd));
   }
   else
   {
       if(var_->get_type()->get_sym_type() == sym_int)
         gen->push(Instruction(cmd_push, op_memory, "dword ptr [esi]"));
       else
         gen->push(Instruction(cmd_push, op_memory, "qword ptr [esi]"));
   }
}

void SynVar::generate_base(const std::shared_ptr<Generator> &gen, bool flag)
{
   if(var_->is_global())
      gen->push(Instruction(cmd_push, op_memory, "offset v_" + str_));
   else
   {
      std::string tmp = boost::lexical_cast<std::string>(var_->get_offset());
      if(var_->is_var_arg())
         gen->push(Instruction(cmd_mov, op_register, "esi", op_memory, "[" + tmp + " + ebp]"));
      else
         gen->push(Instruction(cmd_lea, op_register, "esi", op_memory, "[" + tmp + " + ebp]"));
      gen->push(Instruction(cmd_push, op_register, "esi"));
   }
}

void SynVar::pop_val(const std::shared_ptr<Generator> &gen)
{
   if(var_->get_type()->get_sym_type() == sym_int)
      gen->push(Instruction(cmd_pop, op_register, "eax"));
}

void SynVar::generate_arg_rec(const std::shared_ptr<Generator> &gen)
{
   if(var_->get_type()->get_sym_type() == sym_array || var_->get_type()->get_sym_type() == sym_record)
   {
      gen->push(Instruction(cmd_pop, op_register, "edi"));
      gen->push(Instruction(cmd_mov, op_register, "esi", op_register, "esp"));
      gen->push(Instruction(cmd_mov, op_register, "ecx", op_immediate, var_->get_number_of_elements()));
      gen->push_string("rep ");
      gen->push(Instruction(cmd_movsd));
      gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, var_->get_type()->get_size()));
   }
}
void SynVar::generate_lvalue(const std::shared_ptr<Generator> &gen)
{
   generate_base(gen);
}

void SynArray::print(std::ofstream &output, int depth)
{
   lp_->print(output, depth);
   for each(const auto& it in indexes_)
   {
      for(int i = 0; i < depth; ++i) 
         output << ' ';
      output << "[\n";
      it->print(output, depth + 5);
      for(int i = 0; i < depth; ++i) 
         output << ' ';
      output << "]\n";
   }
}

SynArray::SynArray(const std::string &nm, const std::shared_ptr<SynVar> &e, const std::list<std::shared_ptr<Expr>> &l,
   const std::shared_ptr<SymVar> &sv, const std::shared_ptr<SymType> &st): SynVar(nm, sv), lp_(e), el_type_(st)
{
   dim_ = l.size();
   auto it = --l.end();
   for(size_t k = dim_; k > 0; --k)
   {
      indexes_.push_back(*it);
      if(it != l.begin())
         --it;
   }
}

size_t SynArray::get_size_k(size_t k)
{
   return lp_->get_sym_var()->get_type()->get_element_size(k);
}

void SynArray::generate_base(const std::shared_ptr<Generator> &gen, bool flag)
{
   if(var_->is_global())
      gen->push(Instruction(cmd_push, op_memory, "offset v_" + str_));
   else
   {
      std::string tmp = boost::lexical_cast<std::string>(var_->get_offset());
      if(var_->is_var_arg())
         gen->push(Instruction(cmd_mov, op_register, "esi", op_memory, "[" + tmp + " + ebp]"));
      else
         gen->push(Instruction(cmd_lea, op_register, "esi", op_memory, "[" + tmp + " + ebp]"));
      gen->push(Instruction(cmd_push, op_register, "esi"));
   }
   generate_index(gen);
   gen->push(Instruction(cmd_push, op_register, "esi"));
}

void SynArray::generate_index(const std::shared_ptr<Generator> &gen)
{
   for each(const auto& it in indexes_)
      it->generate(gen);
   gen->push(Instruction(cmd_xor, op_register, "esi", op_register, "esi"));
   for (size_t i = 1; i <= dim_; ++i)
   {
      gen->push(Instruction(cmd_pop, op_register, "eax"));
      gen->push(Instruction(cmd_sub, op_register, "eax", op_immediate, "1"));
      gen->push(Instruction(cmd_imul, op_register, "eax", op_immediate, get_size_k(i)));
      gen->push(Instruction(cmd_add, op_register, "esi", op_register, "eax"));
      //gen->push(Instruction(cmd_lea, op_register, "esi", op_memory, "[(eax-1)*" + boost::lexical_cast<std::string>(get_size_k(i)) + "]"));
   }
   gen->push(Instruction(cmd_pop, op_register, "edi"));
   gen->push(Instruction(cmd_add, op_register, "esi", op_register, "edi"));
}

void SynArray::generate(const std::shared_ptr<Generator> &gen)
{
   generate_base(gen);
   gen->push(Instruction(cmd_pop, op_register, "esi"));
   switch(el_type_->get_sym_type())
   {
      case sym_double:
         gen->push(Instruction(cmd_push, op_memory, "qword ptr [esi]"));
         break;
      case sym_int:
         gen->push(Instruction(cmd_push, op_memory, "dword ptr [esi]"));
         break;
      case sym_array:
      case sym_record:
         gen->push(Instruction(cmd_sub, op_register, "esp", op_immediate, get_size_k(dim_)));
         gen->push(Instruction(cmd_mov, op_register, "edi", op_register, "esp"));
         gen->push(Instruction(cmd_mov, op_register, "ecx", op_immediate, var_->get_type()->get_number_element()));
         gen->push_string("rep ");
         gen->push(Instruction(cmd_movsd));
         break;
   }
}

void SynArray::pop_val(const std::shared_ptr<Generator> &gen)
{
   if (el_type_->get_sym_type() == sym_int)
      gen->push(Instruction(cmd_pop, op_register, "eax"));
}

void SynArray::generate_lvalue(const std::shared_ptr<Generator> &gen)
{
   generate_base(gen);
   if (el_type_->get_sym_type() == sym_array || el_type_->get_sym_type() == sym_record)
   {
      gen->push(Instruction(cmd_pop, op_register, "edi"));
      gen->push(Instruction(cmd_mov, op_register, "esi", op_register, "esp"));
      gen->push(Instruction(cmd_mov, op_register, "ecx", op_immediate, var_->get_type()->get_number_element()));
      gen->push_string("\trep ");
      gen->push(Instruction(cmd_movsd));
      gen->push(Instruction(cmd_add, op_register, "esp", op_immediate, get_size_k(dim_)));
   }
}

void SynConstStr::print(std::ofstream &output, int depth)
{
   for(int i = 0; i < depth; ++i)
      output << ' ';
   output << str_ << '\n';
}

void SynConstStr::generate(const std::shared_ptr<Generator> &gen)
{
   gen->push(Instruction(cmd_mov, op_register, "esi", op_memory, "offset s_" + boost::lexical_cast<std::string>(num_)));
   /*gen->push(Instruction(cmd_mov, op_register, "edi", op_immediate, boost::lexical_cast<std::string>(len_)));*/
}

SynConstStr::SynConstStr(const std::string &s, size_t n): Expr(std::make_shared<SymType>()), str_(s), num_(n)
{
   len_ = str_.length() - 2;
}

void SynRec::print(std::ofstream &output, int depth)
{
   recn_->print(output, depth + 5);
   for(int i = 0; i < depth; ++i) output << ' ';
   output << '.' << std::endl;
   field_->print(output, depth + 5);
}

void SynRec::generate_base(const std::shared_ptr<Generator> &gen, bool flag)
{
   if (recn_->get_syn_type() == syn_array)
   {
      recn_->generate_index(gen);
      gen->push(Instruction(cmd_push, op_register, "esi"));   
   }
   else if (!flag)
      recn_->generate_base(gen);

   gen->push(Instruction(cmd_pop, op_register, "esi"));
   gen->push(Instruction(cmd_add, op_register, "esi", op_immediate, field_->get_sym_var()->get_offset()));
   gen->push(Instruction(cmd_push, op_register, "esi"));

   if (field_->get_syn_type() == syn_array)
   {
      auto sar = std::static_pointer_cast<SynArray> (field_);
      sar->generate_index(gen);
      gen->push(Instruction(cmd_push, op_memory, "esi"));
   }
    if (field_->get_syn_type() == syn_rec)
      field_->generate_base(gen, true);
}

void SynRec::generate(const std::shared_ptr<Generator> &gen)
{
   generate_base(gen);
   gen->push(Instruction(cmd_pop, op_register, "esi"));
   switch (stype_->get_sym_type())
   {
      case sym_double:
         gen->push(Instruction(cmd_push, op_immediate, "qword ptr [esi]"));
         break;
      case sym_int:
         gen->push(Instruction(cmd_push, op_immediate, "dword ptr [esi]"));
         break;
      case sym_array: 
      case sym_record:
         gen->push(Instruction(cmd_push, op_register, "esi"));
         break;
   }
}

void SynRec::pop_val(const std::shared_ptr<Generator> &gen)
{
   if (stype_->get_sym_type() == sym_int)
      gen->push(Instruction(cmd_pop, op_register, "eax"));
}

void SynRec::generate_lvalue(const std::shared_ptr<Generator> &gen)
{
   generate_base(gen);
}
