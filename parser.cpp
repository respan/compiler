#include "parser.h"
#include <sstream>

void Parser::is_type_equals(const std::shared_ptr<Expr> &e1, const std::shared_ptr<Expr> &e2, size_t line, bool is_arithmetic)
{
   auto t1 = e1->get_type();
   auto t2 = e2->get_type();
   bool types_eq = false;
   if(is_arithmetic && (t1->get_sym_type() == sym_double || t2->get_sym_type() == sym_double) && (t1->get_sym_type() != sym_array && t2->get_sym_type() != sym_array))
      types_eq = true;
   if(t1->get_sym_type() == sym_double && t2->get_sym_type() == sym_int)
      types_eq = true;
   if (t1->get_sym_type() == t2->get_sym_type() && t1->get_sym_type() != sym_array)
      types_eq = true;
   if (t1->get_sym_type() == sym_array)
       if(t2->get_sym_type() == sym_array)
          types_eq = (t1.get() == t2.get());
   if (!types_eq)
      type_match_error(t1, t2, line);
   return;
}

std::shared_ptr<SynObj> Parser::parse()
{
   scan_.next();
   parse_declaration();
   auto obj = parse_stmt(table_);
   scan_.require_token(dot, ".");
   return obj;
}

std::shared_ptr<Expr> Parser::parse_expr(const std::shared_ptr<SymTable> &sym_table)
{
   auto left = parse_term(sym_table);
   while (scan_ == plus_op || scan_ == minus_op || scan_ == or_op || scan_ == xor_op)
   {
      Token op = scan_.get();
      scan_.next();
      auto right = parse_term(sym_table);
      make_node(left, right, op);
   }
   return left;
}

std::shared_ptr<Expr> Parser::parse_term(const std::shared_ptr<SymTable> &sym_table)
{
   auto left = parse_factor(sym_table);
   while (scan_ == mul_op || scan_ == div_op || scan_ == and_op || scan_ == mod_op)
   {
      Token op = scan_.get();
      scan_.next();
      auto right = parse_factor(sym_table);
      make_node(left, right, op);
   }
   return left;
}

std::shared_ptr<Expr> Parser::parse_unary(const std::shared_ptr<SymTable> &sym_table)
{
   Token sgn = scan_.get();
   scan_.next();
   auto expr = parse_factor(sym_table);
   if (sgn.type() == not_op && expr->get_type()->get_sym_type() != sym_int)
      logical_op_error(sgn);
   if (expr->is_const() && sgn.type() != plus_op)
      return constant_folding(expr, expr, sgn.type(), true);
   return std::make_shared<UnaryOp>(expr->get_type(), sgn, expr, expr->is_const());
}

std::shared_ptr<Expr> Parser::parse_factor(const std::shared_ptr<SymTable> &sym_table)
{
   switch(scan_.get().type())
   {
      case id:
      {
         if(scan_.get().is_keyword())
            break;
         Token ident = scan_.get();
         auto var = sym_table->p_find(ident);
         auto table = sym_table;

         if (!var)
         {
            var = table_->p_find(ident);
            if (!var)
               scan_.error("Undeclared identifier: ", ident);
         }

         scan_.next();

         if (scan_ == left_round_paren || var->is_proc())
            return parse_function_call(ident, sym_table);
         return parse_ident(var, sym_table, std::make_shared<SymTable>());
         break;
      }
      case inum:
      case dnum:
      {
         std::string str = scan_.get().get_string();
         LexemeType type = scan_.get().type();
         scan_.next();
         if (type == inum)
            return std::make_shared<SynConstInt>(str);
         ++double_count_;
         table_->insert("dc_" + boost::lexical_cast<std::string>(double_count_),
            std::make_shared<SymConst>(double_count_, str, std::static_pointer_cast<SymType>(table_->p_find("double"))));
         return std::make_shared<SynConstDouble>(str, double_count_);
         break;
      }
      case strng:
      {
         std::string str = scan_.get().get_string();
         scan_.next();
         ++string_count_;
         table_->insert("s_" + boost::lexical_cast<std::string>(string_count_), 
            std::make_shared<SymConst>(string_count_, str, std::static_pointer_cast<SymType>(table_->p_find("integer"))));
         return std::make_shared<SynConstStr>(str, string_count_);
         break;
      }
      case left_round_paren:
      {
         scan_.next();
         auto rel = parse_rel(sym_table);
         rel->set_higher_priority();
         scan_.require_token(right_round_paren, ")");
         return rel;
         break;
      }
      case minus_op:
      case plus_op:
      case not_op:
         return parse_unary(sym_table);
         break;
      default:
         return std::make_shared<EmptyExpr>();
         break;
   }
}

std::shared_ptr<Expr> Parser::parse_rel(const std::shared_ptr<SymTable> &sym_table)
{
   auto left = parse_expr(sym_table);
   while (scan_ == lesser_equal || scan_ == greater_equal || scan_ == equal ||
          scan_ == not_equal || scan_ == greater || scan_ == lesser)
   {
      Token op = scan_.get();
      scan_.next();
      auto right = parse_expr(sym_table);
      if (left->is_const() && right->is_const())
         left = constant_folding(left, right, op.type());
      else
      {
         is_type_equals(left, right, op.get_line(), true);
         left = std::make_shared<BinaryOp>(choose_expr_type(right, left), op, left, right);
      }
   }
   return left;
}

std::shared_ptr<Statement> Parser::parse_block(const std::shared_ptr<SymTable> &sym_table)
{
   scan_.require_token(begin_stmt, "begin");
   auto block = std::make_shared<Block>();
   parse_stmt_and_push_to_block(sym_table, block, end_stmt);
   scan_.require_token(end_stmt, "end");
   return block;
}

std::shared_ptr<Statement> Parser::parse_stmt(const std::shared_ptr<SymTable> &sym_table)
{
   switch(scan_.get().type())
   {
   case begin_stmt:
      return parse_block(sym_table);
      break;
   case while_stmt:
      return parse_while(sym_table);
      break;
   case repeat_stmt:
      return parse_repeat(sym_table);
      break;
   case if_stmt:
      return parse_if(sym_table);
      break;
   case for_stmt:
      return parse_for(sym_table);
      break;
   case break_stmt:
      scan_.next();
      return std::make_shared<BreakStmt>();
      break;
   case continue_stmt:
      scan_.next();
      return std::make_shared<ContinueStmt>();
      break;
   case inum:
   case dnum:
      return std::make_shared<ExprStmt>(parse_expr(sym_table));
      break;
   case id:
   {
      if (scan_.get().is_keyword())
         break;
      Token ident = scan_.get();
      scan_.next();
      if (scan_ == assignment || scan_ == left_square_paren || scan_ == dot)
         return std::make_shared<ExprStmt>(parse_assignment(ident, sym_table));
      return std::make_shared<ExprStmt>(parse_function_call(ident, sym_table));
      break;
   }
   case read_stmt:
   case write_stmt:
   case readln_stmt:
   case writeln_stmt:
   {
      Token ident = scan_.get();
      scan_.next();
      return parse_write_read(ident, sym_table);
      break;
   }
   default:
      scan_.error("Not expected token ", scan_.get());
      break;
   }
}

std::shared_ptr<Statement> Parser::parse_while(const std::shared_ptr<SymTable> &sym_table)
{
   scan_.require_token(while_stmt, "while");
   std::shared_ptr<Expr> expr;

   if (scan_ == left_round_paren)
   {
      scan_.next();
      expr = parse_rel(sym_table);
      scan_.require_token(right_round_paren, ")");
   }
   else 
      expr = parse_rel(sym_table);

   scan_.require_token(do_stmt, "do");

   if (expr->get_string() == "0")
   {
      parse_stmt(sym_table);
      return std::make_shared<EmptyStmt>();
   }

   return std::make_shared<WhileStmt>(expr, parse_stmt(sym_table));
}

std::shared_ptr<Statement> Parser::parse_repeat(const std::shared_ptr<SymTable> &sym_table)
{
   scan_.require_token(repeat_stmt, "repeat");
   auto block = std::make_shared<Block>();
   parse_stmt_and_push_to_block(sym_table, block, until_stmt);
   scan_.require_token(until_stmt, "until");
   auto expr = parse_rel(sym_table);
   return std::make_shared<RepeatStmt>(expr, block);
}

std::shared_ptr<Statement> Parser::parse_if(const std::shared_ptr<SymTable> &sym_table)
{
   scan_.require_token(if_stmt, "if");
   auto expr = parse_rel(sym_table);
   scan_.require_token(then_stmt, "then");
   auto if_block = parse_stmt(sym_table);

   if (scan_ != semicolon)
   {  
      scan_.require_token(else_stmt, "else");
      if (expr->get_string() == "0")
         return parse_stmt(sym_table);
      return std::make_shared<IfStmt>(expr, if_block, parse_stmt(sym_table));
   }

   if (expr->get_string() == "0")
      return std::make_shared<EmptyStmt>();

   return std::make_shared<IfStmt>(expr, if_block, std::make_shared<EmptyStmt>());   
}

std::shared_ptr<Statement> Parser::parse_for(const std::shared_ptr<SymTable> &sym_table)
{
   scan_.require_token(for_stmt, "for");
   Token loop_var = scan_.get();
   scan_.next();

   auto initial_value = parse_assignment(loop_var, sym_table);
   Token clue = scan_.get();

   if (clue.type() != to_stmt && clue.type() != downto_stmt)
      scan_.error("There is must be \"to\" or \"downto\" but was ", clue);

   scan_.next();
   auto final_value = parse_rel(sym_table);

   scan_.require_token(do_stmt, "do");

   auto block = parse_stmt(sym_table);

   if (initial_value->get_right_expr()->is_const() && final_value->is_const())
   {
      int iv = boost::lexical_cast<int>(initial_value->get_right_expr()->get_string()),
          fv = boost::lexical_cast<int>(final_value->get_string());
      if (clue.type() == to_stmt && iv > fv)
         return std::make_shared<EmptyStmt>();
      if (clue.type() == downto_stmt && iv < fv)
         return std::make_shared<EmptyStmt>();
   }

   return std::make_shared<ForStmt>(initial_value, final_value, clue, loop_var, block);
}

std::shared_ptr<Statement> Parser::parse_write_read(const Token &ident, const std::shared_ptr<SymTable> &sym_table)
{
   std::list<std::shared_ptr<Expr>> expr_list;
   if (scan_ == left_round_paren)
   {
      scan_.next();
      auto par = parse_rel(sym_table);
      int type = par->get_type()->get_sym_type();

      expr_list.push_back(par);

      if (type != sym_double && type != sym_int)
         type_match_error(std::make_shared<Double>(Double("double or integer")), par->get_type(), ident.get_line());

      while (scan_ == comma)
      {
         scan_.next();
         expr_list.push_back(parse_rel(sym_table));
      }
      scan_.require_token(right_round_paren, ")");
   }
   switch (ident.type())
   {
   case write_stmt:
      return std::make_shared<WriteCall>(expr_list, false);
      break;
   case writeln_stmt:
      return std::make_shared<WriteCall>(expr_list, true);
      break;
   case read_stmt:
      return std::make_shared<ReadCall>(expr_list, false);
      break;
   case readln_stmt:
      return std::make_shared<ReadCall>(expr_list, true);
      break;
   }      
}

std::shared_ptr<Expr> Parser::parse_assignment(const Token &ident, const std::shared_ptr<SymTable> &sym_table)
{
   auto var = sym_table->p_find(ident);
   auto table = sym_table;
   if (!var)
   {
      var = table_->p_find(ident);
      if (!var)
         scan_.error("Undeclared identifier: ", ident);
      table = table_;
   }
   auto left = parse_ident(var, sym_table, std::make_shared<SymTable>()); 
   scan_.require_token(assignment, ":=");
   auto right = parse_expr(sym_table);

   is_type_equals(left, right, ident.get_line());
   return std::make_shared<BinaryOp>(choose_expr_type(right, left), Token(assignment, ":="), left, right);
}

std::shared_ptr<Expr> Parser::parse_ident(std::shared_ptr<Symbol> ident, const std::shared_ptr<SymTable> &sym_table, const std::shared_ptr<SymTable> &param_table, bool is_par)
{
   ident->set_used();
   auto type = ident->get_type();
   auto var = std::static_pointer_cast<SymVar>(ident);
   auto expr = std::make_shared<SynVar>(ident->get_name(), var);

   if (scan_ == left_square_paren)
   {
      if (type->get_sym_type() != sym_array)
         type_match_error(type->get_type(), std::shared_ptr<SymArray>(), scan_.get().get_line());
      std::list<std::shared_ptr<Expr>> indexes;
      size_t count = 0;
      while (scan_ == left_square_paren)
      {
         scan_.next();
         std::shared_ptr<Expr> index_expr;

         if (is_par)
            index_expr = parse_expr(param_table);
         else 
            index_expr = parse_expr(sym_table);

         scan_.require_token(right_square_paren, "]");

         if (index_expr->get_type()->get_sym_type() != sym_int)
            type_match_error(std::make_shared<Int>(Int("integer")), index_expr->get_type(), scan_.get().get_line());

         indexes.push_back(index_expr);
         ++count;
      }
      type = var->get_element_k_type(count);
      expr = std::make_shared<SynArray>(ident->get_name(), expr, indexes, var, type);
   }
   while (scan_ == dot)
   {
      Token tok = scan_.next();
      scan_.next();

      if (expr->get_syn_type() == syn_array)
      {
         auto table = type->get_sub_table();
         if (table == nullptr)
            scan_.error("From the left should be record to access a field", tok);
         ident = table->p_find(tok);
      }
      else
         ident = type->get_sub_table()->p_find(tok);

      std::shared_ptr<SynVar> field = std::static_pointer_cast<SynVar>(parse_ident(ident, type->get_sub_table(), sym_table, true));
      expr = std::make_shared<SynRec>(tok.get_string(), var, expr, field, field->get_type());
   }
   return expr;
}

std::shared_ptr<Expr> Parser::parse_function_call(const Token &ident, const std::shared_ptr<SymTable> &sym_table)
{
   std::list<std::shared_ptr<Expr>> args;
   size_t count = 0;
   std::shared_ptr<SymProc> st = std::static_pointer_cast<SymProc>(sym_table->p_find(ident));
   if (!st)
   {
      st = std::static_pointer_cast<SymProc>(table_->p_find(ident));
      if (!st)
         scan_.error("Undeclared identifier: ", ident);
   }
   st->set_used();
   if (scan_ == left_round_paren)
   {
      scan_.next();
      auto par = parse_rel(sym_table);

      if (par->get_type()->get_sym_type() == sym_none && st->get_arg_list().size())
         scan_.error("Missed arguments", ident);
      else if (par->get_type()->get_sym_type() != sym_none && !st->get_arg_list().size())
         scan_.error("Too much arguments", ident);
      else if (par->get_type()->get_sym_type() != sym_none && st->get_arg_list().size())
      {
         is_type_equals(st->get_arg(count), par, ident.get_line());
         args.push_back(par);
         ++count;

         while (scan_ == comma)
         {
            scan_.next();
            par = parse_rel(sym_table);
            if (count >= st->get_arg_list().size())
               scan_.error("Too much arguments", ident);
            is_type_equals(st->get_arg(count), par, ident.get_line());
            args.push_back(par);
            ++count;
         }
      }

      scan_.require_token(right_round_paren, ")");
   }
   else if (st->get_arg_list().size() > count)
      scan_.error("Missed arguments", ident);
   return std::make_shared<FunCall>(args, ident, st);
}

void Parser::parse_declaration()
{
   LexemeType t;
   while (scan_ != begin_stmt)
   {
      t = scan_.get().type();
      switch(t)
      {
         case type_decl: 
         {
            scan_.next();
            parse_type_declaration();
            break;
         }
         case var_decl:
         {
            scan_.next();
            parse_var_decalration(table_, false);
            break;
         }
         default:
         {
            scan_.next();
            std::string name = scan_.get().get_string();

            if (table_->find(name))
               scan_.error("Duplicate identifier", Token(id, name));

            scan_.next();

            std::list<std::shared_ptr<SynVar>> params_list;
            auto local_table = std::make_shared<SymTable>();
            int offset = 0;

            if (scan_ == left_round_paren)
            {
               scan_.next();
               params_list = parse_params(local_table, offset);
               scan_.require_token(right_round_paren, ")");
            }

            std::shared_ptr<SymType> func_type;

            if (t == function_decl)
            {
               scan_.require_token(colon, ":");
               func_type = parse_type();

               if (func_type->get_name() == "array")
                  scan_.error("Wrong declaration function type", Token(id, name));

               if (offset == 0)
                  offset = 8;

               local_table->insert(std::make_shared<SymVar>("result", func_type, offset, false, false));
               local_table->insert(std::make_shared<SymVar>(name, func_type, offset, false, false));

               scan_.next();
            }

            scan_.require_token(semicolon, ";");
            int size = 0;

            if (scan_ == var_decl)
            {
               scan_.next();
               size = parse_var_decalration(local_table, true);
            }

            std::shared_ptr<SymProc> procedure;
            if (t == function_decl)
               procedure = std::make_shared<SymFunc>(name, local_table, params_list, func_type, size);
            else
               procedure = std::make_shared<SymProc>(name, local_table, params_list, size);

            table_->insert(procedure);

            auto body = parse_block(local_table);
            scan_.require_token(semicolon, ";");
            procedure->set_block(body);
            break;
         }
      }
   }
   return;
}

void Parser::parse_type_declaration()
{
   while (scan_ == id && !scan_.get().is_keyword())
   {
      Token ident = scan_.get();

      if (table_->find(ident))
         scan_.error("Duplicate identifier: ", ident);

      scan_.next();
      scan_.require_token(equal, "=");
      auto type = parse_type();
      scan_.next();
      table_->insert(ident, type);
      scan_.require_token(semicolon, ";");
   }   
}

int Parser::parse_var_decalration(const std::shared_ptr<SymTable> &sym_table, bool is_proc)
{
   std::list<std::string> lst;
   int offset = sizeof(int);
   size_t size = 0;
   offset *= -1;
   while (scan_ == id && !scan_.get().is_keyword())
   {
      lst.push_back(scan_.get().get_string());
      scan_.next();
      if (scan_ == colon)
      {
         scan_.next();
         if (scan_ == array_decl)
         {
            auto type = parse_type();
            for each(const auto& k in lst)
            {
               if (is_proc)
               {
                  size += type->get_size();
                  offset = size;
                  offset *= -1;
               }
               auto var = std::make_shared<SymVar>(k, type, offset, !is_proc);
               if (!sym_table->find(var->get_name()))
                  sym_table->insert(var);
               else 
                  scan_.error("Duplicate identifier: ", Token(id, k));
            }
         }
         else
         {
            auto type = parse_type();
            
            for each(const auto& k in lst) 
            {
               if (is_proc)
               { 
                  size += type->get_size();
                  offset = size;
                  offset *= -1;
               }
               std::shared_ptr<SymVar> var = std::make_shared<SymVar>(k, type, offset, !is_proc);
               if (!sym_table->find(var->get_name()))
                  sym_table->insert(var);
               else 
                  scan_.error("Duplicate identifier: ", Token(id, k));
            }
         }
         lst.clear();
         scan_.next();
         scan_.require_token(semicolon, ";");
      }
      else 
         scan_.require_token(comma, ",");
   }
   return size;
}

std::shared_ptr<SymType> Parser::parse_type()
{
   switch (scan_.get().type())
   {
      case integer_decl:
      {
         return std::static_pointer_cast<SymType>(table_->p_find("integer"));
         break;
      }
      case double_decl:
      {
         return std::static_pointer_cast<SymType>(table_->p_find("double"));
         break;
      }
      case inum:
      {
         long int lb = scan_.get().get_int_value(), rb;
         scan_.next(); 
         if (scan_ == dot)
         {
            scan_.require_token(dot, ".");
            rb = scan_.get().get_int_value();
         }
         return std::make_shared<IntRange>("range", lb, rb);
         break;
      }
      case array_decl:
      {
         scan_.next();
         scan_.require_token(left_square_paren, "[");
         int size = scan_.get().get_int_value();
         scan_.next();

         if (scan_ == dot)
         {
            scan_.next();
            scan_.require_token(dot, ".");
            size = scan_.get().get_int_value() - size + 1;
            scan_.next();
         }

         scan_.require_token(right_square_paren, "]");
         scan_.require_token(of_decl, "of");
         return std::make_shared<SymArray>("array", parse_type(), size);
         break;
      }
      case rec_decl:
      {
         auto fields = std::make_shared<SymTable>();
         scan_.next();
         std::list<std::string> lvar;
         int offset = 0;
         while (scan_ == id && scan_ != end_stmt)
         {
            lvar.push_back(scan_.get().get_string());
            scan_.next();
            if (scan_ == colon)
            {
               scan_.next();
               auto type = parse_type();
               for each(const auto& k in lvar)
               {
                  fields->insert(std::make_shared<SymVar>(k, type, offset));
                  offset += type->get_size();
               }
               lvar.clear();
               scan_.next();
            }
            scan_.next();
         }
         return std::make_shared<SymStruct>("record", fields);
         break;
      }
      case id:
      {
         if (!table_->find(scan_.get()))
            scan_.error("Undefined type: ", scan_.get());

         std::shared_ptr<SymType> type = std::static_pointer_cast<SymType>(table_->p_find(scan_.get()));
         type->set_name(scan_.get().get_string());
         return type;
         break;
      }
      default:
      {
         scan_.error("Undefined type: ", scan_.get());
         break;
      }
   }
}

std::list<std::shared_ptr<SynVar>> Parser::parse_params(const std::shared_ptr<SymTable> &sym_table, int &os)
{
   std::list<std::shared_ptr<SynVar>> var_list;
   auto params_table = std::make_shared<SymTable>();
   int offset = 8, num = 0;
   while (scan_ == id || scan_ == var_decl) 
   {
      bool is_var = false;
      if (scan_ == var_decl)
      {
         is_var = true;
         scan_.next();
      }
      bool cond = true;
      std::list<std::string> ls;
      while (cond) 
      {
         ls.push_back(scan_.get().get_string());
         ++num;
         scan_.next();
         if (scan_ == colon)
         {
            cond = false;
            scan_.next();
            auto type = parse_type();
            scan_.next();
            if (scan_ != right_round_paren)
               scan_.require_token(semicolon, ";");
            for each(const auto& k in ls) 
            {
               auto svar = std::make_shared<SymVar>(k, type, offset, false, is_var);
               auto varb = std::make_shared<SynVar>(k, svar);
               if (!params_table->find(svar->get_name()) && !sym_table->find(svar->get_name()))
               {
                  params_table->insert(svar);
                  var_list.push_back(varb);            
               }
               else 
                  scan_.error("Duplicate identifier: ", Token(id, k));
            }
         }
         else 
            scan_.require_token(comma, ",");
      }
   }
   if (!params_table->get_table().empty())
   {
      auto it = (*params_table).get_table().end();
      --it;
      for(int i = 0; i < num; ++i)
      {
         auto svar = std::static_pointer_cast<SymVar>((*it).second);
         svar->set_offset(offset);
         offset += svar->is_var_arg() ? 4 : svar->get_type()->get_size();
         sym_table->insert(svar);
         --it;
      }
   }      
   os = offset;
   return var_list;
}

std::string Parser::get_message(const std::shared_ptr<Symbol> &t)
{
   std::string mes = "";
   if(t->get_name() == "array")
   {
      mes += "array of ";
      mes += get_message(t->get_element_type());
   }
   else
      return t->get_name();
   return mes;
}

void Parser::print_table()
{
   table_->print_var_table(output_);
}

void Parser::type_match_error(const std::shared_ptr<SymType> &t1, const std::shared_ptr<SymType> &t2, size_t line)
{
   scan_.type_match_error(get_message(t1), get_message(t2), line);
}

void Parser::generate(const std::shared_ptr<Generator> &gen)
{
   auto st = std::static_pointer_cast<Statement>(parse());
   gen->push_string("include source\\start.inc\n");
   st->generate(gen);
   gen->push_string("\ninclude source\\end.inc\n");
   gen->push_string("\tint_frmt db '%d', 0\n\tdouble_frmt db '%f', 0\n\tnew_line db '', 0Dh, 0Ah, 0\n\tdouble_buff dq 0.0\n");
   table_->generate(gen);   
   gen->push_string("end start");
}

Parser::Parser(Scanner &s, std::ofstream &o) : scan_(s), output_(o), table_(std::make_shared<SymTable>()), double_count_(0), string_count_(0)
{
   table_->insert(std::make_shared<Int>("integer"));
   table_->insert(std::make_shared<Double>("double"));
}

std::shared_ptr<Expr> Parser::constant_folding(const std::shared_ptr<Expr> &e1, const std::shared_ptr<Expr> &e2, int sign, bool is_unary)
{
   if (e1->get_type()->get_sym_type() == sym_double || e2->get_type()->get_sym_type() == sym_double)
   {
      double d1, d2;
      int i = 0;
      d1 = boost::lexical_cast<double>(e1->get_string());
      if (!is_unary)
         d2 = boost::lexical_cast<double>(e2->get_string());
      switch(sign)
      {
      case plus_op:
         if (!is_unary)
            d1 += d2;
         break;
      case minus_op:
         if (is_unary)
            d1 = -d1;
         else
            d1 -= d2;
         break;
      case mul_op:
         d1 *= d2;
         break;
      case div_op:
         d1 /= d2;
         break;
      case mod_op:
         d1 = std::fmod(d1, d2);
         break;
      case lesser_equal:
         i = d1 <= d2;
         break;
      case greater_equal:
         i = d1 >= d2;
         break;
      case equal:
         i = d1 == d2;
         break;
      case not_equal: 
         i = d1 != d2;
         break;
      case greater:
         i = d1 > d2;
         break;
      case lesser:
         i = d1 < d2;
         break;
      }
      if (e1->get_type()->get_sym_type() == sym_double && e2->get_type()->get_sym_type() == sym_double && !is_unary)
      {
         table_->erase("dc_" + boost::lexical_cast<std::string>(double_count_));
         --double_count_;
      }
      if(i)
      {
         table_->erase("dc_" + boost::lexical_cast<std::string>(double_count_));
         --double_count_;
         return std::make_shared<SynConstInt>(boost::lexical_cast<std::string>(i));;
      }
      std::string val = boost::lexical_cast<std::string>(d1);
      double e;
      std::string s = std::abs(std::modf(d1, &e)) < 0.00001 ? ".0" : "";
      table_->insert("dc_" + boost::lexical_cast<std::string>(double_count_),
         std::make_shared<SymConst>(double_count_, val + s, std::static_pointer_cast<SymType>(table_->p_find("double"))));
      return std::make_shared<SynConstDouble>(val + s, double_count_);
   }
   else
   {
      int i1, i2;
      i1 = boost::lexical_cast<int>(e1->get_string());
      i2 = boost::lexical_cast<int>(e2->get_string());
      switch(sign)
      {
      case plus_op:
         if (!is_unary)
            i1 += i2;
         break;
      case minus_op:
         if (is_unary)
            i1 = -i1;
         else
            i1 -= i2;
         break;
      case mul_op:
         i1 *= i2;
         break;
      case div_op:
         i1 /= i2;
         break;
      case mod_op:
         i1 %= i2;
         break;
      case lesser_equal:
         i1 = i1 <= i2;
         break;
      case greater_equal:
         i1 = i1 >= i2;
         break;
      case equal:
         i1 = i1 == i2;
         break;
      case not_equal: 
         i1 = i1 != i2;
         break;
      case greater:
         i1 = i1 > i2;
         break;
      case lesser:
         i1 = i1 < i2;
         break;
      case and_op:
         i1 = i1 && i2;
         break;
      case xor_op:
         i1 = !i1 ^ !i2;
         break;
      case or_op:
         i1 = i1 || i2;
         break;
      case not_op:
         i1 = !i1;
         break;
      }
      return std::make_shared<SynConstInt>(boost::lexical_cast<std::string>(i1));
   }
}

void Parser::logical_op_error(const Token &op)
{
   output_ << "Error at line " << op.get_line() << ": " << op.get_string() + " operation can be used with int type only";
   exit(0);
}

void Parser::make_node(std::shared_ptr<Expr> &left, const std::shared_ptr<Expr> &right, const Token &op)
{
   auto type = op.type();
   auto expr_type = choose_expr_type(right, left);
   if ((type == and_op || type == xor_op || type == or_op) && expr_type->get_sym_type() != sym_int)
      logical_op_error(op);
   bool hp = left->is_higher_priority() || right->is_higher_priority();
   if (!hp && left->is_const() && right->is_const())
      left = constant_folding(left, right, op.type());
   else if (!hp && left->get_right_expr() && left->get_right_expr()->is_const() && right->is_const())
      left->change_right_expr(constant_folding(left->get_right_expr(), right, op.type()));
   else
   {
      is_type_equals(left, right, op.get_line(), true);
      left = std::make_shared<BinaryOp>(choose_expr_type(right, left), op, left, right);
   }
}

void Parser::parse_stmt_and_push_to_block(const std::shared_ptr<SymTable> &sym_table, const std::shared_ptr<Block> &block, LexemeType type)
{
   bool br = false;
   while (scan_ != type)
   {
      auto stmt = parse_stmt(sym_table);
      if (!br)
         block->push_stmt(stmt);
      scan_.require_token(semicolon, ";");
      if (stmt->is_break_or_continue())
         br = true;
   }
}
