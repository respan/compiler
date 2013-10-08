#include "scanner.h"
#include <algorithm>

const Token &Scanner::get() const
{
   return current_;
}

bool Scanner::operator ==(int t)
{
   return current_.type() == t;
}

bool Scanner::operator !=(int t)
{
   return current_.type() != t;
}

int Scanner::is_eof() const
{
   return feof(input_);
}

void Scanner::error(const std::string &mes, const Token &token1, const Token &token2, int code)
{
   switch (code)
   {
   case 0:
      output_ << "Error at line " << token1.get_line() << ", col " << token1.get_col();
      output_ << ": " << mes << "-- \"" << token1.get_string().c_str() << "\"" << std::endl;
      break;
   case 1:
      output_ << "Error at line " << token2.get_line() << ": Expected " << "\"";
      output_ << token1.get_string().c_str() << "\" but was \"" << token2.get_string().c_str() << "\"";
      break;
   }
   exit(0);
}

void Scanner::type_match_error(const std::string &t1, const std::string &t2, size_t line)
{
   output_ << "Error at line " << line << ": impossible type conversion from " << t2 << " to " << t1;
   exit(0);
}

void Scanner::require_token(LexemeType t, const std::string &s)
{
   if (*this != t)
      error("Expected ", Token(t, s), get(), 1);
   next();
}

LexemeType set_type_keyword(const std::string &s)
{
   if (s == "and")
      return and_op; 
   if (s == "or")
      return or_op; 
   if (s == "xor")
      return xor_op; 
   if (s == "div")
      return div_op; 
   if (s == "mod")
      return mod_op; 
   if (s == "not")
      return not_op; 
   if (s == "begin")
      return begin_stmt;
   if (s == "end")
      return end_stmt;
   if (s == "if")
      return if_stmt;
   if (s == "for")
      return for_stmt;
   if (s == "while")
      return while_stmt;
   if (s == "repeat")
      return repeat_stmt;
   if (s == "write")
      return write_stmt;
   if (s == "writeln")
      return writeln_stmt;
   if (s == "read")
      return read_stmt;
   if (s == "readln")
      return readln_stmt;
   if (s == "break")
      return break_stmt;
   if (s == "continue")
      return continue_stmt;
   if (s == "do")
      return do_stmt;
   if (s == "until")
      return until_stmt;
   if (s == "then")
      return then_stmt;
   if (s == "else")
      return else_stmt;
   if (s == "to")
      return to_stmt;
   if (s == "downto")
      return downto_stmt;
   if (s == "var")
      return var_decl;
   if (s == "type")
      return type_decl;
   if (s == "function")
      return function_decl;
   if (s == "procedure")
      return procedure_decl;
   if (s == "integer")
      return integer_decl;
   if (s == "double")
      return double_decl;
   if (s == "array")
      return array_decl;
   if (s == "of")
      return of_decl;
   if (s == "record")
      return rec_decl;
   return id;
}

const Token &Scanner::output(LexemeType type, std::string &chars, bool isread)
{
   std::transform(chars.begin(), chars.end(), chars.begin(), ::tolower);
   isread_ = isread;
   state_ = start;
   int t = (type == id  || type == inum || type == dnum || type == and_op || type == or_op || type == xor_op ||
           (type == div_op && chars != "/") || type == mod_op || type == not_op) ? 1 : 0;
   size_t column = t ? col_ - chars.size() - 1 : col_ - chars.size();
   if (type == id)
      type = set_type_keyword(chars);
   return current_ = Token(type, chars, line_, column);
}

const Token &Scanner::next()
{
   std::string chars;
   char local_symbol;
   
   LexemeType t = id;
   while (!feof(input_))
   {
      if (!isread_)
      {
         ++col_;
         symbol_ = fgetc(input_);
      }
      isread_ = false;
      switch (state_)
      {
         case start:
         {
            if (symbol_ == ' ' || symbol_ == '\n' || symbol_ == '\t')
            {
               if (symbol_ == '\n')
               {
                  ++line_;
                  col_ = 1;
               }
               state_ = start; 
            }
            else if (!feof(input_))
            {
               isread_ = true;
               state_ = id_1;
            }
            break;
         }
         case id_1: 
         {
            if (isalpha(symbol_))
               state_ = id_2;
            else
            {
               isread_ = true;
               state_ = numDigit_1;
            }
            break;
         }
         case id_2:
         {
            if (isdigit(symbol_) || isalpha(symbol_) || symbol_ == '_')
               state_ = id_2;
            else
            {
               isread_ = true;
               return output(id, chars, true);
            }
            break;
         }
         case numDigit_1:
         {
            if (isdigit(symbol_))
               state_ = numDigit_2;
            else
            {
               isread_ = true;
               state_ = equal_st;
            }
            break;
         }
         case numDigit_2:
         {
            if (isdigit(symbol_))
               state_ = numDigit_2;
            else if (symbol_ == '.')
               state_ = numDot;
            else
               return output(inum, chars, true);
            break;
         }
         case numDot:
         {
            if (isdigit(symbol_))
               state_ = numDigitAfterDot;
            else if(symbol_ == '.')
            {
               symbol_ = chars[chars.length() - 1];
               chars[chars.length() - 1] = '\0';
               _fseeki64(input_, -1, SEEK_CUR);
               return output(inum, chars, true);
            }
            else
               error("Dot after int number", Token(error_lex, ""));
            break;
         }
         case numDigitAfterDot:
         {
            if (isdigit(symbol_))
               state_ = numDigitAfterDot;
            else if (symbol_ == 'e' || symbol_ == 'E')
               state_ = numExponent;
            else
               return output(dnum, chars, true);
            break;
         }
         case numExponent:
         {
            if(symbol_ == '+' || symbol_ == '-')
               state_ = numSign;
            else if (isdigit(symbol_))
               state_ = numDigitAfterExp;
            else
               return output(dnum, chars, true);
            break;
         }
         case numSign:
         {
            if (isdigit(symbol_))
               state_ = numDigitAfterExp;
            else
               return output(dnum, chars, true);
            break;
         }
         case numDigitAfterExp:
         {
            if (isdigit(symbol_))
               state_ = numDigitAfterExp;
            else
               return output(dnum, chars, true);
            break;
         }
         case equal_st:
         {
            if (symbol_ == '=')
            {
               chars += '=';
               return output(equal, chars, false);
            }
            isread_ = true;
            state_ = greaterOperator_1;
            break;
         }
         case greaterOperator_1:
         {
            if (symbol_ == '>')
               state_ = greaterOperator_2;
            else
            {
               isread_ = true;
               state_ = smallerOperator_1;
            }
            break;
         }
         case greaterOperator_2:
         {
            if (symbol_ == '=')
               chars += '=';
            else
               isread_ = true;
            return output(chars.size() == 2 ? greater_equal : greater, chars, isread_);
            break;
         }
         case smallerOperator_1:
         {
            if (symbol_ == '<')
               state_ = smallerOperator_2;
            else
            {
               isread_ = true;
               state_ = comment;
            }
            break;
         }
         case smallerOperator_2:
         {
            t = id;
            if (symbol_ == '=')
            {
               chars += '=';
               t = lesser_equal;
            }
            else if  (symbol_ == '>')
            {
               chars += '>';
               t = not_equal;
            }
            else
               isread_ = true;
            return output(t == id ? lesser : t, chars, isread_);
            break;
         }
         case assignmentOperator:
         {
            if (symbol_ == '=')
            {
               chars += '=';
               return output(assignment, chars, false);
            }
            --col_;
            isread_ = true;
            _fseeki64(input_, -1, SEEK_CUR);
            symbol_ = local_symbol;
            state_ = divider;
            break;
         }
         case comment:
         {
            if (symbol_ == '{')
               state_ = commentCurlyBrackets;
            else if (symbol_ == '(')
               state_ = commentRoundBracketBegin;
            else if (symbol_ == '/')
               state_ = slashComment;
            else
            {
               isread_ = true;
               state_ = arithmeticOperator;
            }
            break;
         }
         case commentCurlyBrackets:
         {
            if(symbol_ != '}')
            {
               size_t l = line_, c = col_ - 2;
               local_symbol = fgetc(input_);
               while (local_symbol != '}')
               {
                  if (feof(input_))
                     error("found unclosed comment ", Token(error_lex, "{", l, c));
                  local_symbol = fgetc(input_);
                  ++col_;
               }
            }
            symbol_ = ' ';
            isread_ = true;
            state_ = start;
            break;
         }
         case commentRoundBracketBegin:
         {
            if (symbol_ == '*')
               state_ = commentRoundBracketEnd;
            else
            {
               --col_;
               _fseeki64(input_, -1, SEEK_CUR);
               symbol_ = chars[chars.length() - 1];
               isread_ = true;
               state_ = divider;
            }
            break;
         }
         case commentRoundBracketEnd:
         {
            char local_chars[2];
            size_t l = line_, c = col_ - 3;
            local_chars[0] = symbol_;
            local_chars[1] = fgetc(input_);
            ++col_;
            if (local_chars[0] == '\n')
               ++line_;
            if (local_chars[1] == '\n')
               ++line_;
            while (local_chars[0] != '*' || local_chars[1] != ')')
            {
               if(feof(input_))
                  error("found unclosed comment", Token(error_lex, "(*", l, c));
               local_chars[0] = local_chars[1];
               local_chars[1] = fgetc(input_);
               ++col_;
               if (local_chars[1] == '\n')
                  ++line_;
            }
            state_ = start;
            break;
         }
         case slashComment:
         {
            if (symbol_ == '/')
            {
               local_symbol = fgetc(input_);

               while(local_symbol != '\n' && !feof(input_))
               {
                  local_symbol = fgetc(input_);
                  ++col_;
               }
               ++line_;
               state_ = start;
            }
            else
            {
               --col_;
               _fseeki64(input_, -1, SEEK_CUR);
               symbol_ = chars[chars.length() - 1];
               isread_ = true;
               state_ = arithmeticOperator;
            }
            break;
         }
         case arithmeticOperator:
         {
            if (symbol_ == '+' || symbol_ == '-' || symbol_ == '*' || symbol_ == '/')
            {
               chars += symbol_;
               t = id;
               switch (symbol_)
               {
                  case '+': 
                  {
                     t = plus_op;
                     break;
                  }
                  case '-': 
                  {
                     t = minus_op;
                     break;
                  }
                  case '*': 
                  {
                     t = mul_op;
                     break;
                  }
                  case '/': 
                  {
                     t = div_op;
                     break;
                  }
               }
               return output(t, chars, false);
            }
            if (symbol_ == ':')
            {
               local_symbol = symbol_;
               state_ = assignmentOperator;
            }
            else
            {
               isread_ = true;
               state_ = divider;
            }
            break;
         }
         case divider:
         {
            if (symbol_ == '[' || symbol_ == ']' || symbol_ == '(' || symbol_ == ')' ||
               symbol_ == ';' || symbol_ == '.' || symbol_ == ',' || symbol_ == ':')
            {
               chars += symbol_;
               t = id;
               switch(symbol_)
               {
                  case '[': 
                  {
                     t = left_square_paren;
                     break;
                  }
                  case ']':
                  {
                     t = right_square_paren;
                     break;
                  }
                  case '(': 
                  {
                     t = left_round_paren;
                     break;
                  }
                  case ')':
                  {
                     t = right_round_paren;
                     break;
                  }
                  case ';':
                  {
                     t = semicolon;
                     break;
                  }
                  case '.':
                  {
                     t = dot;
                     break;
                  }
                  case ',':
                  {
                     t = comma;
                     break;
                  }
                  case ':': 
                  {
                     t = colon;
                     break;
                  }
               }
               return output(t, chars, false);
            }
            isread_ = true;
            state_ = quotation;
            break;
         }
         case quotation:
         {
            if (symbol_ == '\"' || symbol_ == '\'')
            {
               size_t l = line_, c = col_ - 2;
               local_symbol = fgetc(input_);
               ++col_;
               chars += symbol_;
               while (local_symbol != '\"' && local_symbol != '\'')
               {
                  if (feof(input_))
                     error("Found unclosed quotation ", Token(strng, &symbol_, l, c));
                  chars += local_symbol;
                  local_symbol = fgetc(input_);
                  ++col_;
               }
               chars += symbol_;
               return output(strng, chars, false);
            }
            state_ = errorSymbol;
            break;
         }
         case errorSymbol: 
         {
            error("Undefined symbol ", Token(error_lex, &symbol_));
            break;
         }
      }
      if (isread_) 
         chars = "";
      else
         chars += symbol_;
   }
   return current_ = Token(error_lex, "", 0);
}