#ifndef SYMTAB_HPP
#define SYMTAB_HPP

#include "ast.hpp"
#include "attribute.hpp"
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_map>

using namespace std;

//all types
typedef AllType Symbol;

class SymName 
{
  char* m_spelling; // "name" of the symbol
  Symbol* m_symbol; // pointer to the symbol for this name

  public:

  SymName(const SymName &);
  SymName &operator=(const SymName &);
  SymName(char* const x);
  ~SymName();
  virtual void accept(Visitor *v);
  virtual SymName *clone() const;
  void swap(SymName &);

  const char* spelling();
  const Symbol* symbol();
  void set_symbol( Symbol* symbol );

  Attribute* m_parent_attribute;
};

// this is one-level of scope for the SymTab
// it is not defined in the hpp file because it 
// is only used in the implementation of SymTab

//comparison structure for strings
struct eqstr {
        bool operator()(const char* s1, const char* s2) const
                { return strcmp(s1, s2) == 0; }
                };

class SymScope
{

    SymScope* m_parent;
    SymScope* m_last;
    list<SymScope*> m_child;
    typedef std::unordered_map<string, Symbol*, std::hash<std::string> > ScopeTableType;
    ScopeTableType m_scopetable;

    public:
    SymScope* parent();
    void add_child(SymScope* c);
    SymScope(SymScope* last, SymScope * parent);
    bool is_dup_string(const char* name); //used for error check

    void dump( FILE* f, int nest_level );
    SymScope* open_scope();
    SymScope* open_scope(SymScope* last);
    SymScope* close_scope();
    bool exist( const char* name );
    Symbol* insert( const char* name, Symbol * s );
    Symbol* lookup( const char * name );

    SymScope();
    ~SymScope();

    friend class SymTab; //symtab is a wrapper class
};

// This is the symbol table header which is similar
// to the interface described in class.  There is a
// open and close scope to grow a symbol table tree.
// lookup and exist recurisively search all of the
// parent scopes, while insert considers only the
// current scope.  An example chunk of code is below
class SymTab
{
    private:
    SymScope* m_head;
    SymScope* m_cur_scope;
    bool is_dup_string(const char*);

    public:

    SymTab();
    ~SymTab();

    void open_scope();
    void open_scope(SymScope* parent);
    void close_scope();

    //returns true if name is found in the current SymTab
    //or any of the parents
    bool exist(const char* name );

    //tries to insert a pointer to s into the symbol table and
    //returns true if successful.
    //name should be a "fresh" copy, meaning that the SymTab
    //will keep track of the memory pointed to by name and free
    //it in the destructor.  name should not point to some memory
    //that has already been inserted into the symtab (see example)
    bool insert(const char* name, Symbol * s );

    //does an insert into the parent scope of the working scope
    //(it will have an assert failure if there is no parent scope)
    bool insert_in_parent_scope(const char* name, Symbol * s );

    //tries to locate name in the current SymTab and all
    //of the parent SymTabs
    Symbol* lookup( const char * name );
    Symbol* lookup( SymName * name );

    //get current scope
    SymScope* get_current_scope();
    SymScope* get_scope();

    //dump the contents of the symbol table to the file
    //descriptor provided.  very useful for debugging
    void dump( FILE* f );
};

#endif //SYMTAB_HPP
