#pragma once
#ifndef COMPILER_SYMBOL_H_
#define COMPILER_SYMBOL_H_
#include <cmath>
#include "scanner.h"
#include "generator.h"

enum SymTypes 
{
   sym_int, sym_double, sym_array, sym_record, sym_none, sym_proc,
};

class SymTable;
class SymType;

class Symbol 
{
protected:
   std::string name_;
   bool used_;
public:
   Symbol(const std::string &n = "", bool u = false): name_(n), used_(u) {}
   virtual ~Symbol(){}
   virtual std::string get_name() const { return name_; }
   bool operator ==(Symbol s) { return (name_ == s.name_); }
   virtual bool is_proc() const { return false; }
   virtual void print_block(std::ofstream &output) { return; }
   virtual void print_local_table(std::ofstream &output) { return; }
   virtual SymTypes get_sym_type() const  { return sym_none; }
   virtual size_t get_size() { return 0; }
   virtual std::shared_ptr<SymType> get_type() const { return nullptr; }
   virtual std::shared_ptr<SymTable> get_sub_table() const { return nullptr; }
   virtual std::shared_ptr<SymType> get_element_type() const { return nullptr; }
   virtual std::shared_ptr<SymType> get_element_i_type(int i) const { return nullptr; }
   virtual void generate(const std::shared_ptr<Generator> &gen) {};
   void set_used() { used_ = true; }
   bool is_used() { return used_; }
};

class SymType: public Symbol 
{
public:
   SymType(const std::string &n = ""): Symbol(n) {}
   ~SymType(){}
    virtual size_t get_element_size(size_t k) const { return 0; }
    virtual std::shared_ptr<SymType> get_element_k_type(size_t k) const { return nullptr; }
    void set_name(const std::string &n) { name_ = n; }
    virtual int get_left() const { return 0; }
    virtual int get_right() const { return 0; }
    virtual size_t get_number() { return 0; }
    virtual size_t get_number_element() { return 0; }
};

class SymTable
{
protected:
   std::map<std::string, std::shared_ptr<Symbol>> table_; 
public:
   SymTable() {}
   ~SymTable() {}
   void insert(const std::shared_ptr<Symbol> &symb) { table_[symb->get_name()] = symb; }
   void insert(const std::string &key, const std::shared_ptr<Symbol> &symb) { table_[key] = symb; }
   void insert(const Token &key, const std::shared_ptr<Symbol> &symb) { table_[key.get_string()] = symb; }
   void erase(const std::string &key) { table_.erase(key); }
   size_t find(const Token &key) { return table_.count(key.get_string()); }
   size_t find(const std::string &key) { return table_.count(key); }
   std::map<std::string, std::shared_ptr<Symbol>> &get_table() { return table_; }
   std::shared_ptr<Symbol> p_find(const Token &key);
   std::shared_ptr<Symbol> p_find(const std::string &key);
   void print_var_table(std::ofstream &output, bool is_block = false);
   void generate(const std::shared_ptr<Generator> &gen);
};

class SymVar: public Symbol 
{
   std::shared_ptr<SymType> type_;
   int offset_;
   bool global_, var_arg_;
public:
   SymVar(const std::string &n, const std::shared_ptr<SymType> &t, int os = 0, bool gl = true, bool isv = false):
      Symbol(n), type_(t), offset_(os), global_(gl), var_arg_(isv) {}
   ~SymVar() {}
   std::shared_ptr<SymType> get_type() const { return type_; }
   std::shared_ptr<SymType> get_element_type() const { return type_->get_element_type(); }
   std::shared_ptr<SymType> get_element_k_type(size_t k) { return type_->get_element_k_type(k); }
   bool is_global() { return global_; }
   bool is_var_arg() { return var_arg_; }
   int get_offset() { return offset_; }
   void set_offset(int off) { offset_ = off; }
   size_t get_number_of_elements() { return type_->get_number(); }
   void generate(const std::shared_ptr<Generator> &gen);

};

class SymTypeScal: public SymType 
{
public:
   SymTypeScal(const std::string &n = ""): SymType(n) {}
};

class Int: public SymTypeScal
{
public:
   Int(const std::string &n = ""): SymTypeScal(n) {}
   SymTypes get_sym_type() const { return sym_int; }
   size_t get_size() { return sizeof(int); }
   void generate(const std::shared_ptr<Generator> &gen) { gen->push_string("\tdd ?\n"); }
};

class IntRange: public Int
{
   int lb, rb;
public:
   IntRange(const std::string &n = "", int r = 0, int l = 0): Int(n), lb(l), rb(r) {}
   int get_left() const { return lb; }
   int get_right() const { return rb; }
};

class Double: public SymTypeScal
{
public:
   Double(const std::string &n = ""): SymTypeScal(n) {}
   SymTypes get_sym_type() const { return sym_double; }
   size_t get_size() { return sizeof(double); }
   void generate(const std::shared_ptr<Generator> &gen) { gen->push_string("\tdq ?\n"); }
};

class SymArray: public SymType 
{
   size_t size_;
   std::shared_ptr<SymType> element_type_;
public:
   SymArray(const std::string &n, const std::shared_ptr<SymType> &s, size_t t = 0): SymType(n), element_type_(s), size_(t) {}
   ~SymArray() {}
   std::shared_ptr<SymType> get_element_type() const ;
   size_t get_element_size(size_t k) const;
   std::shared_ptr<SymType> get_element_k_type(size_t k) const;
   std::shared_ptr<SymType> get_type() const { return element_type_; }
   size_t get_number() { return size_ * (element_type_->get_size() / sizeof(size_t)); }
   size_t get_number_element();
   SymTypes get_sym_type() const  { return sym_array; }
   size_t get_size() { return size_ * element_type_->get_size(); }
   std::string get_name() const { return name_;   }
   void generate(const std::shared_ptr<Generator> &gen) { gen->push_string("\tdb " + boost::lexical_cast<std::string>(get_size()) + " dup(?)\n"); }
};

class SymStruct: public SymType 
{
   std::shared_ptr<SymTable> fields_;
public:
   SymStruct(const std::string &n, const std::shared_ptr<SymTable> &st): SymType(n), fields_(st) {}
   ~SymStruct() {}
   std::shared_ptr<SymTable> get_sub_table() const { return fields_; }
   SymTypes get_sym_type() const  { return sym_record; }
   size_t get_size();
   std::string get_name() const { return name_; }
   size_t get_number() { return get_size()/sizeof(size_t); }
   void generate(const std::shared_ptr<Generator> &gen) { gen->push_string("\tdb " + boost::lexical_cast<std::string>(get_size()) + " dup(?)\n"); }

};

class SymConst: public Symbol 
{
   std::string value_;
   int const_num_;
   std::shared_ptr<SymType> const_type_;
public:
   SymConst(int k, const std::string &s, const std::shared_ptr<SymType> &st): const_num_(k), value_(s), const_type_(st) {set_used();}
   ~SymConst() {}
   std::shared_ptr<SymType> get_type() const { return const_type_; }
   std::string get_name() const { return const_type_->get_name(); }
   void generate(const std::shared_ptr<Generator> &gen);
};

#endif