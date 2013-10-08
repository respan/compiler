#include "generator.h"

std::string commands[] = 
{
   "push", "pop", "add", "sub", "mul", "div", "mov", "idiv",
   "ret", "faddp", "fsubp", "fdivp", "fmulp", "fxch", "fabs", "fchs",
   "fstp", "fld", "jz", "jnz", "jne", "jg", "jge", "jl",
   "jle", "je", "test", "fstsw", "lea", "call", "jmp", "cmp",
   "invoke", "movsd", "or", "xor", "and", "imul", "neg", "inc",
   "dec", "fcompp", "sahf", "setg", "setl", "sete", "setne", "setle",
   "setge", "cdq", "fild", "sal", "sar", "seta", "setb", "setae",
   "setbe", "setz",
};

void Instruction::write_command(std::ofstream &output) const
{
   std::string str_cmd;
   switch(cmd_)
   {
      case cmd_wrlab:
         str_cmd = first_op_->get_name();
         if (first_op_->get_operand_type() == op_label)
            str_cmd += ":\n";
         break;
      case cmd_const_decl:
         str_cmd = "\t" + first_op_->get_name() + second_op_->get_name();
         break;
      default:
         output << "\t";
         if (second_op_->get_operand_type() == op_null)
            str_cmd = commands[cmd_] + "\t" + first_op_->get_name() + "\n";
         else
            str_cmd = commands[cmd_] + "\t" + first_op_->get_name() + ", " + second_op_->get_name() + "\n";
         break;
   }
   output << str_cmd;
}

Instruction::Instruction(AsmCommands c, AsmOperands t1, const std::string &o1, AsmOperands t2, const std::string &o2): cmd_(c)
{
   first_op_ = std::make_shared<Operand>(t1, o1); 
   second_op_ = std::make_shared<Operand>(t2, o2);
}

Instruction::Instruction(AsmCommands c, std::shared_ptr<Operand> op1, std::shared_ptr<Operand> op2): cmd_(c)
{
   first_op_ = op1;
   second_op_ = op2 ? op2 : std::make_shared<Operand>(op_null, "");
}

Instruction::Instruction(AsmCommands c, AsmOperands t1, const std::string &o1, AsmOperands t2, size_t o2): cmd_(c)
{
   first_op_ = std::make_shared<Operand>(t1, o1);
   second_op_ = std::make_shared<Operand>(t2, boost::lexical_cast<std::string>(o2));
}

void Generator::write_to_file(std::ofstream &output, bool opt)
{
   if (opt)
      optimize();
   for each(const auto& it in commands_)
      it.write_command(output);
}

void Generator::optimize()
{
   bool flag = true;
   std::list<std::pair<std::string, std::string>> label_list;
   std::list<std::pair<std::string, std::string>> var_list;
   do
   {
      auto first_instr = commands_.begin();
      auto second_instr = ++commands_.begin();
      auto decl_it = first_instr;
      flag = false;
      do
      {
         if (*first_instr == cmd_wrlab && first_instr->get_first() == op_null && first_instr->get_first()->get_name() == "\ninclude source\\end.inc\n")
         {
            decl_it = first_instr;
            ++first_instr;
            ++second_instr;
         }

//       push a
//       pop b
         if(*first_instr == cmd_push && *second_instr == cmd_pop)
         {
            // a != b -> mov b, a
            if (first_instr->get_first() != second_instr->get_first() && second_instr->get_first() == op_register)
            {
               commands_.insert(first_instr, Instruction(cmd_mov, second_instr->get_first(), first_instr->get_first()));
               delete_instr(first_instr, second_instr);
               flag = true;
            }
            if(first_instr->get_first() == second_instr->get_second())
            {
               delete_instr(first_instr, second_instr);
               flag = true;
            }
         }

//       pop r
//       push r
//       -> delete pop && push
         if(*first_instr == cmd_pop && *second_instr == cmd_push && first_instr->get_first() == second_instr->get_first())
         {
            delete_instr(first_instr, second_instr);
            flag = true;
         }

//                ...
//       label1:
//       label2:
//                ...
//       -> delete label1
         if (*first_instr == cmd_wrlab && *second_instr == cmd_wrlab && first_instr->get_first() == op_label && second_instr->get_first() == op_label)
         {
            label_list.push_back(std::make_pair(first_instr->get_first()->get_name(), second_instr->get_first()->get_name()));
            delete_instr(first_instr);
            flag = true;
         }

         //change deleted labels in jumps
         if (is_jump(first_instr->get_cmd()))
         {
            for each(auto it in label_list)
               if (it.first == first_instr->get_first()->get_name())
               {
                  first_instr->get_first()->set_name(it.second);
                  flag = true;
                  break;
               }
         }

//          jmp label
//       label: 
//          -> delete jmp
         if (*first_instr == cmd_jmp && *second_instr == cmd_wrlab && first_instr->get_first()->get_name() == second_instr->get_first()->get_name())
         {
            delete_instr(first_instr);
            flag = true;
         }

//       jmp l1
//       jmp l2
//       -> delete l2

         if (*first_instr == cmd_jmp && *second_instr == cmd_jmp)
         {
            second_instr = commands_.erase(second_instr);
            flag = true;
         }

         //mov r, 0 -> xor r, r
         if (*first_instr == cmd_mov && first_instr->get_first() == op_register && first_instr->get_second()->get_name() == "0")
         {
            commands_.insert(first_instr, Instruction(cmd_xor, first_instr->get_first(), first_instr->get_first()));            
            delete_instr(first_instr);
            flag = true;
         }

//       mov r, r
//       -> delete
         if (*first_instr == cmd_mov && first_instr->get_first() == first_instr->get_second())
         {
            delete_instr(first_instr);
            flag = true;
         }

//       add r, 0 || sub r, 0
//       -> delete
         if ((*first_instr == cmd_add || *first_instr == cmd_sub) && first_instr->get_second() == "0")
         {
            delete_instr(first_instr);
            flag = true;
         }

//       add r, 1
//       -> inc r
         if (*first_instr == cmd_add && first_instr->get_second()->get_name() == "1")
         {
            commands_.insert(first_instr, Instruction(cmd_inc, first_instr->get_first()));
            delete_instr(first_instr);
            flag = true;
         }

//       sub r, 1
//       -> dec r
         if (*first_instr == cmd_sub && first_instr->get_second()->get_name() == "1")
         {
            commands_.insert(first_instr, Instruction(cmd_dec, first_instr->get_first()));
            delete_instr(first_instr);
            flag = true;
         }

//       mov r, 1
//       dec r
//       -> xor r, r
         if (*first_instr == cmd_mov && *second_instr == cmd_dec && first_instr->get_first() == second_instr->get_first() && first_instr->get_second()->get_name() == "1")
         {
            commands_.insert(first_instr, Instruction(cmd_xor, first_instr->get_first(), first_instr->get_first()));
            delete_instr(first_instr, second_instr);
            flag = true;
         }

//       xor r, r
//       imul r, i || add a, r || sub a, r || div r, i
//       -> delete imul/add/sub 
         if (*first_instr == cmd_xor && (((*second_instr == cmd_imul || *second_instr == cmd_idiv) && first_instr->get_first() == second_instr->get_first()) ||
            ((*second_instr == cmd_add || *second_instr == cmd_sub) && first_instr->get_first() == second_instr->get_second())))
         {
            second_instr = commands_.erase(second_instr);
            flag = true;
         }


//       val1 dq 1.2
//       val2 dq 1.2
//       -> delete val2, and use val1 instead val2
         if (*first_instr == cmd_const_decl)
         {
            for (auto it = decl_it; it != commands_.end(); ++it)
               if (*it == cmd_const_decl && first_instr->get_first()->get_name() != (*it).get_first()->get_name() &&
                  first_instr->get_second()->get_name() == (*it).get_second()->get_name())
               {
                  var_list.push_back(std::make_pair(it->get_first()->get_name(), first_instr->get_first()->get_name()));
                  if (second_instr == it)
                     ++second_instr;
                  it = commands_.erase(it);
                  flag = true;
               }
         }

         if ((*first_instr == cmd_push && first_instr->get_first() == op_memory) || 
            (*first_instr == cmd_mov && first_instr->get_first() == op_register && first_instr->get_second() == op_memory))
            for each(auto it in var_list)
            {
               if ("qword ptr " + it.first == first_instr->get_first()->get_name())
               {
                  first_instr->get_first()->set_name("qword ptr " + it.second);
                  flag = true;
                  break;
               }
               else if ("offset " + it.first == first_instr->get_second()->get_name())
               {
                  first_instr->get_second()->set_name("offset " + it.second);
                  flag = true;
                  break;
               }
            }

         ++first_instr;
         ++second_instr;

      } while (second_instr != commands_.end());
   } while(flag);
}

void Generator::delete_instr(std::list<Instruction>::iterator &it1, std::list<Instruction>::iterator &it2)
{
   it2 = commands_.erase(it2);
   delete_instr(it1);
}

void Generator::delete_instr(std::list<Instruction>::iterator &it1)
{
   it1 = commands_.erase(it1);
   --it1;
}

bool Generator::is_jump(AsmCommands c)
{
   switch(c)
   {
   case cmd_jz:
   case cmd_jnz:
   case cmd_jne: 
   case cmd_jg: 
   case cmd_jmp:
   case cmd_jge:
   case cmd_jl:
   case cmd_jle:
   case cmd_je:
      return true;
      break;
   default:
      return false;
      break;
   }
}

int Generator::generate_int_arithmetic(LexemeType t, bool use_bitwise_op, int i)
{
   switch(t)
   {
   case plus_op:
      push(Instruction(cmd_add, op_register, "eax", op_register, "ecx"));
      break;
   case minus_op:
      push(Instruction(cmd_sub, op_register, "eax", op_register, "ecx"));
      break;
   case mul_op:
      if (use_bitwise_op)
         push(Instruction(cmd_sal, op_register, "eax", op_immediate, i));
      else
         push(Instruction(cmd_mul, op_register, "ecx"));
      break;
   case or_op:
      push(Instruction(cmd_or, op_register, "eax", op_register, "ecx"));
      break;
   case xor_op:
      push(Instruction(cmd_xor, op_register, "eax", op_register, "ecx"));
      break;
   case and_op:
      push(Instruction(cmd_and, op_register, "eax", op_register, "ecx"));
      break;
   case mod_op:
      push(Instruction(cmd_cdq));
      push(Instruction(cmd_idiv, op_register, "ecx"));
      return 0;
      break;
   case div_op:
      if (use_bitwise_op)
         push(Instruction(cmd_sar, op_register, "eax", op_immediate, i));
      else
      {
         push(Instruction(cmd_cdq));
         push(Instruction(cmd_idiv, op_register, "ecx"));
      }
      break;
   }
   return 1;
}

void Generator::generate_double_arithmetic(LexemeType t)
{
   switch (t)
   {
   case plus_op:
      push(Instruction(cmd_faddp));
      break;
   case minus_op:
      push(Instruction(cmd_fsubp));
      break;
   case mul_op:
      push(Instruction(cmd_fmulp));
      break;
   case div_op:
      push(Instruction(cmd_fdivp));
      break;
   }
}

void Generator::generate_setcc(LexemeType t, bool is_unsigned_cmp)
{
   switch(t)
   {
   case lesser_equal:
      if (is_unsigned_cmp)
         push(Instruction(cmd_setbe, op_register, "al"));
      else
         push(Instruction(cmd_setle, op_register, "al"));
      break;
   case greater_equal:
      if (is_unsigned_cmp)
         push(Instruction(cmd_setae, op_register, "al"));
      else
         push(Instruction(cmd_setge, op_register, "al"));
      break;
   case equal:
      push(Instruction(cmd_sete, op_register, "al"));
      break;
   case not_equal:
      push(Instruction(cmd_setne, op_register, "al"));
      break;
   case greater:
      if (is_unsigned_cmp)
         push(Instruction(cmd_seta, op_register, "al"));
      else
         push(Instruction(cmd_setg, op_register, "al"));
      break;
   case lesser:
      if (is_unsigned_cmp)
         push(Instruction(cmd_setb, op_register, "al"));
      else
         push(Instruction(cmd_setl, op_register, "al"));
      break;
   }
}

void Generator::generate_pop_test()
{
   push(Instruction(cmd_pop, op_register, "eax"));
   push(Instruction(cmd_test, op_register, "al", op_register, "al"));
}

