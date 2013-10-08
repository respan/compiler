#pragma once
#ifndef COMPILER_TOKEN_H_
#define COMPILER_TOKEN_H_
#include <string>
#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>

enum LexemeType   
{
   id, inum, dnum, strng, right_square_paren, left_square_paren, right_round_paren, left_round_paren,
   plus_op, minus_op, mul_op, div_op, assignment, colon, semicolon, dot, comma, lesser_equal, greater_equal, equal, not_equal,
   greater, lesser, or_op, xor_op, and_op, mod_op, not_op, begin_stmt, end_stmt, if_stmt, for_stmt, while_stmt, repeat_stmt,
   read_stmt, readln_stmt, write_stmt, writeln_stmt, break_stmt, continue_stmt, do_stmt, until_stmt, then_stmt, else_stmt, to_stmt,
   downto_stmt, type_decl, var_decl, procedure_decl, function_decl, integer_decl, double_decl, array_decl, rec_decl, of_decl, error_lex = -1, 
};

static std::string keywords[34] = 
{  
   "and", "array", "begin", "break", "const", "continue",
   "div", "do", "downto", "else", "end", "file", "for", 
   "function", "if", "in", "mod", "nil", "not", "of",
   "or", "procedure", "program", "record", "repeat",
   "set", "then", "to", "until", "uses", "var", "while",
   "with", "xor",
};

class Token
{
   LexemeType type_;
   size_t line_, col_;
   std::string str_;
public:
   Token() {};
   Token(LexemeType t, std::string s = std::string(), size_t l = 1, size_t c = 0): type_(t), str_(s), line_(l), col_(c) {}
   Token(LexemeType t, const char *s = "", size_t l = 1, size_t c = 0): type_(t), str_(s), line_(l), col_(c) {}
   Token(const Token &t);
   void print(std::ofstream &output);
   LexemeType type() const;
   bool is_keyword() const;
   const std::string& get_string() const;
   int get_int_value() const;
   double get_double_value() const;
   size_t get_line() const;
   size_t get_col() const;
   ~Token() {}
};

#endif