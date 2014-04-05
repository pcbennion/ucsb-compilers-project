#ifndef CLASSHIERARCHY_HPP
#define CLASSHIERARCHY_HPP

#include "ast.hpp"
#include "attribute.hpp"
#include "symtab.hpp"
#include <unordered_map>
#include <cstddef>
#include <cstring>
#include <string>

class OffsetTable
{

private:

    int totalSize;
    int paramSize;
    std::unordered_map<std::string,int > m_offset;
    std::unordered_map<std::string,int > m_size;
    std::unordered_map<std::string,CompoundType> m_type;
	
public:

    OffsetTable();
	
    void insert(const char * symname, int offset, int size,CompoundType type);
    int get_offset(const char * symname);
    int get_size(const char * symname);
    CompoundType get_type(const char * symname);
    bool exist(const char * symname);
	
    int getTotalSize();
    void setTotalSize(int);
    int getParamSize();
    void setParamSize(int);
    void copyTo(OffsetTable*);
};


class ClassName {
    char* m_spelling; // "name" of the class
    
    public:
    
    ClassName(const ClassName &);
    ClassName &operator=(const ClassName &);
    ClassName(char* const x);
    ClassName(const char* const x);
    ~ClassName();
    virtual void accept(Visitor *v);
    virtual ClassName *clone() const;
    void swap(ClassName &);
    
    const char* spelling();
    
    Attribute* m_parent_attribute;
};

class ClassNode {
    public:    
    ClassName *name;
    ClassName *superClass;
    
    ClassImpl *p;
    SymScope* scope;    
    OffsetTable*offset;

    ClassNode(){offset=new OffsetTable();}
};

typedef std::unordered_map<const std::string, ClassNode*, std::hash<std::string> > ClassMap;

class ClassTable {
    ClassMap nameMap;
    ClassNode * topClass;
    
    public:
    ClassTable();
    ~ClassTable();

    bool exist( const char * name );
    ClassNode* insert( const char * name, ClassNode * node );
    ClassNode* insert( const char  * name, const char * superClass, ClassImpl * astNode, SymScope * classScope );
    ClassNode* lookup( const char * name );
    
    ClassNode* getParentOf( const char * name );
     
    bool exist( ClassName* name );
    ClassNode* insert( ClassName * name, ClassNode * node );
    ClassNode* insert( ClassName * name, ClassName * superClass, ClassImpl * astNode, SymScope * classScope );
    ClassNode* lookup( ClassName * name );
    
    ClassNode* getParentOf( ClassName * name );
};


#endif //CLASSHIERARCHY_HPP
