#include "symbol.h"
#include <boost/math/special_functions/modf.hpp>

std::shared_ptr<Symbol> SymTable::p_find(const Token &key)
{
   return p_find(key.get_string());
}

std::shared_ptr<Symbol> SymTable::p_find(const std::string &key)
{
   auto it = table_.find(key);
   if (it != table_.end())
      return (*it).second;
   return nullptr;
}

void SymTable::print_var_table(std::ofstream &output, bool is_block)
{
   output << "\n";
   for each(const auto& it in table_)
   {
      if (is_block)
         output << "     ";
      if (it.second->is_used())
      {
         if (it.second->get_sym_type() == sym_none)
         {
            if (it.second->is_proc())
            {
               output << it.first << "\t" << ((it.second->get_name() == "") ? "void" : it.second->get_type()->get_name()) << "\n";
               it.second->print_local_table(output);
               it.second->print_block(output);
            }
            else if (it.second->get_type()->get_name() == "")
               output << it.first << "\tvoid\n";
            else if (it.second->get_type()->get_name() == "array")
            {
               std::string mes = it.first;
               auto t = it.second->get_type();
               while (t->get_sym_type() == sym_array)
               {
                  mes += " \tarray[";
                  mes += boost::lexical_cast<std::string>(t->get_size());
                  mes += "] of ";
                  t = t->get_type();
               }
               mes += t->get_name();
               output << mes << "\n";
            }
            else if (it.second->get_type()->get_name() == "range")
               output << it.first << "\t" << it.second->get_type()->get_name() << "from " << it.second->get_type()->get_left() << "to " << it.second->get_type()->get_right() << "\n";
            else
               output << it.first << "\t" << it.second->get_type()->get_name() << "\n";
         }
         else
         {
            if(it.second->get_sym_type() == sym_record)
            {
               output << it.first << "\t" << "record\n";
               it.second->get_sub_table()->print_var_table(output, true);
               output << "end\n";
            }
            else if (it.second->get_sym_type() == sym_array)
               output << it.first << "\tarray of " << it.second->get_element_type()->get_name() << "[" << it.second->get_size() << "]\n";
            else if (it.first == "integer")
               output << "integer\tinteger\n";
            else if (it.first == "double")
               output << "double\tdouble\n";
            else if (it.second->get_sym_type() == sym_int)
               output << it.second->get_name() << "\tinteger" << "\n";
            else if (it.second->get_sym_type() == sym_double)
               output << it.second->get_name() << "\tdouble" << "\n";
            else if (it.second->is_proc())
            {
               output << it.first << "\t" << ((it.second->get_name() == "") ? "void" : it.second->get_type()->get_name());
               it.second->print_local_table(output);
               it.second->print_block(output);
            }
            else
               output << it.second->get_name() << "\n";
         }
      }
   }
}

void SymVar::generate(const std::shared_ptr<Generator> &gen)
{
   std::string str = "\tv_" + name_ + " ";
   gen->push_string(str);
   type_->generate(gen);
}

void SymConst::generate(const std::shared_ptr<Generator> &gen)
{
   std::string tmp = boost::lexical_cast<std::string>(const_num_);
   if (const_type_->get_sym_type() == sym_int)
      gen->push_const_decl("s_" + tmp,  " db " + value_ + ", 0\n");
   else 
      gen->push_const_decl("dc_" + tmp, " dq " + value_ + "\n");
}

size_t SymArray::get_number_element()
{
   if(element_type_->get_sym_type() == sym_double)
      return size_ * 2;
   if (element_type_->get_sym_type() == sym_int)
      return size_;
   return element_type_->get_number();
}

std::shared_ptr<SymType> SymArray::get_element_k_type(size_t k) const 
{
   if (k == 1)
      return element_type_;
   auto st = element_type_;
   for (size_t i = 1; i < k; ++i)
      st = st->get_type();
   return st;
}

size_t SymArray::get_element_size(size_t k) const
{
   if (k == 1)
      return element_type_->get_size();
   auto st = element_type_;
   for (size_t i = 1; i < k; ++i)
      st = st->get_type();
   return st->get_size();
}

std::shared_ptr<SymType> SymArray::get_element_type() const
{
   if (element_type_->get_sym_type() == sym_array)
      return element_type_->get_element_type();
   return element_type_;
}

size_t SymStruct::get_size()
{
   size_t size = 0;
   for each(const auto& it in fields_->get_table())
      size += it.second->get_type()->get_size();
   return size;
}

void SymTable::generate(const std::shared_ptr<Generator> &gen)
{
   for each(const auto& it in table_)
   {
      if(it.second->is_used())
         it.second->generate(gen);
   }
}
