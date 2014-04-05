#include <vector>

#ifndef ATTRIBUTE_HPP
#define ATTRIBUTE_HPP
class SymScope;

enum Basetype
{
	bt_undef = 0,
	bt_integer = 1,
	bt_boolean = 2,
	bt_function = 4,
	bt_object = 8,
	bt_nothing = 16
};

struct CompoundType
{
	Basetype baseType;
	const char * classID;
};

struct MethodType
{
	CompoundType returnType;
	std::vector<CompoundType> argsType;
};

struct AllType
{
	Basetype baseType;
	CompoundType classType;
	MethodType methodType;
	
	int m_offset;
	int m_size;
};

class Attribute
{
  public:
  AllType m_type; //type of the subtree
  int lineno; //line number on which that ast node resides
  SymScope * m_scope;


  Attribute() { 
	m_type.baseType = bt_undef;
	lineno = 0;
  }
};

#endif //ATTRIBUTE_HPP

