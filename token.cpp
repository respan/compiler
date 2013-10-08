#include "token.h"

Token::Token(const Token &t)
{
   str_ = t.str_;
   type_ = t.type_;
   line_ = t.line_;
   col_ = t.col_;
}

size_t Token::get_line() const
{
   return line_;
}

size_t Token::get_col() const
{
   return col_;
}

int Token::get_int_value() const
{
   return boost::lexical_cast<int>(str_.c_str());
}

double Token::get_double_value() const 
{
   return boost::lexical_cast<double>(str_.c_str());
}

const std::string& Token::get_string() const 
{
   return str_;
}

LexemeType Token::type() const
{
   return type_;
}

bool Token::is_keyword() const
{
   for(int i = 0; i < 33; ++i)
   {
      if(str_ == keywords[i])
         return true;
   }
   return false;
}

void Token::print(std::ofstream &output)
{
   if (line_ && col_)
      output << std::endl << line_ << " " << col_ << " ";
   switch (type_)
   {
      case id:
      case begin_stmt:
      case end_stmt:
      case if_stmt:
      case for_stmt:
      case while_stmt:
      case repeat_stmt:
      case read_stmt:
      case readln_stmt:
      case write_stmt:
      case writeln_stmt:
      case break_stmt:
      case continue_stmt:
      case do_stmt:
      case until_stmt:
      case then_stmt:
      case else_stmt:
         if (is_keyword())
            output << "keyword ";
         else
            output << "identifier ";
         output << str_ << " " << str_ << std::endl;
         break;
      case inum:
         output << "int " << str_ << " ";
         if(str_.size() > 10)
            output << "OUT OF RANGE";
         else
            output << get_int_value();
         break;
      case dnum:
         output << "double " << str_ << " " << get_double_value();
         break;
      case right_square_paren:
      case left_square_paren:
      case right_round_paren:
      case left_round_paren:
      case colon:
      case semicolon:
      case dot:
      case comma:
         output << "divider " << str_ << " " << str_;
         break;
      case plus_op:
      case minus_op:
      case mul_op:
      case div_op:
      case lesser_equal:
      case greater_equal:
      case equal:
      case not_equal:
      case greater:
      case lesser:
      case assignment:
      case or_op:
      case xor_op:
      case and_op:
      case mod_op:
      case not_op:
         output << "op " << str_ << " " << str_;
         break;
      case strng:
         output << "string " << str_ << " " << str_;
         break;         
   }
}

