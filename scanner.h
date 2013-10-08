#pragma once
#ifndef COMPILER_SCANNER_H_
#define COMPILER_SCANNER_H_
#include "token.h"

enum States 
{
   start, greaterOperator_1, smallerOperator_1, greaterOperator_2, smallerOperator_2,
   divider, id_1, id_2, numDigit_1, numDigit_2, numDot, numDigitAfterDot,
   numExponent, numSign, numDigitAfterExp, equal_st, slashComment,
   arithmeticOperator, assignmentOperator, comment, commentCurlyBrackets,
   commentRoundBracketBegin, commentRoundBracketEnd, errorSymbol, quotation,
};

class Scanner
{
   FILE *input_;
   std::ofstream &output_;
   char symbol_;
   int state_;
   size_t line_, col_;
   Token current_;
   bool isread_;
   const Token &output(LexemeType type, std::string &chars, bool isread);
public:
   Scanner(FILE *inp, std::ofstream &out): input_(inp), output_(out), state_(0), line_(1), col_(1), isread_(false) {}
   const Token &get() const;
   const Token &next();
   int is_eof() const;
   bool operator ==(int t);
   bool operator !=(int t);
   void error(const std::string &mes, const Token &token1, const Token &token2 = Token(), int code = 0);
   void type_match_error(const std::string &t1, const std::string &t2, size_t line);
   void require_token(LexemeType t, const std::string &s);
   ~Scanner() {}
};
#endif