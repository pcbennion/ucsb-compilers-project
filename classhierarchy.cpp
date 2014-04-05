#include "classhierarchy.hpp"

/****** ClassName Implemenation **************************************/

ClassName::ClassName(char* const x)
{
    m_spelling = x;
    m_parent_attribute = NULL;
}
ClassName::ClassName(const char* const x)
{
    m_spelling = strdup(x);
    m_parent_attribute = NULL;
}

ClassName::ClassName(const ClassName & other)
{
    m_spelling = strdup(other.m_spelling);
    m_parent_attribute = other.m_parent_attribute;
}

ClassName& ClassName::operator=(const ClassName & other)
{
    delete m_spelling;
    ClassName tmp(other);
    swap(tmp);
    return *this;
}

void ClassName::swap(ClassName & other)
{
    std::swap(m_spelling, other.m_spelling);
}

ClassName::~ClassName()
{
    delete( m_spelling );
}

void ClassName::accept(Visitor *v)
{ 
    v->visitClassName(this); 
}

ClassName* ClassName::clone() const
{
    return new ClassName(*this);
}

const char* ClassName::spelling()
{
    return m_spelling;
}

/****** ClassTable Implemenation **************************************/

ClassTable::ClassTable() {
    topClass = new ClassNode();
    topClass->name = new ClassName((char * const) "TopClass");
    topClass->superClass = NULL;
    topClass->p = NULL;
    topClass->scope = NULL;
}

ClassTable::~ClassTable() {
    delete topClass;
}

bool ClassTable::exist( ClassName* name ) {
    if(name)
        return (this->lookup(name) != NULL);
    return false;
}

ClassNode* ClassTable::insert( ClassName* name, ClassNode * node ) {
    nameMap[std::string(name->spelling())] = node;
    return node;
}

ClassNode* ClassTable::insert( ClassName * name, ClassName * superClass, ClassImpl * astNode, SymScope * classScope ) {
    ClassNode* newNode = new ClassNode();
    newNode->name = name;
    newNode->superClass = superClass;
    newNode->p = astNode;
    newNode->scope = classScope;
    nameMap[std::string(name->spelling())] = newNode;
    return newNode;
}

ClassNode* ClassTable::lookup( ClassName * name ) {
    if(name && nameMap.find(std::string(name->spelling()))!=nameMap.end())
        return nameMap[std::string(name->spelling())];
    else
        return NULL;
}

ClassNode* ClassTable::getParentOf( ClassName * name ) {
    ClassNode *node = this->lookup(name);
    if(node)
        if (node->superClass)
            return lookup(node->superClass);
        else
            return topClass;
    else
        return NULL;
}
    bool  ClassTable::exist( const char * name ){
	return this->exist(new ClassName(name));	
}
    ClassNode*  ClassTable::insert( const char * name, ClassNode * node ){
	return this->insert(new ClassName(name),node);
}

    ClassNode*  ClassTable::insert( const char  * name, const char * superClass, ClassImpl * astNode, SymScope * classScope ){
	return this->insert(new ClassName(name),new ClassName(superClass),astNode,classScope);
}
    ClassNode*  ClassTable::lookup( const char * name ){
	return this->lookup(new ClassName(name));
}
   
    ClassNode*  ClassTable::getParentOf( const char * name ){
	return this->getParentOf(new ClassName(name));
}
/****** OffsetTable Implemenation **************************************/
void OffsetTable::insert(const char * symname, int offset, int size, CompoundType type)
{
	m_offset[std::string(symname)]=offset;
	m_size[std::string(symname)]=size;
	m_type[std::string(symname)]=type;
}
int OffsetTable::get_offset(const char * symname)
{
	return m_offset[std::string(symname)];
}
int OffsetTable::get_size(const char * symname) 
{
	return m_size[std::string(symname)];
}
void OffsetTable::setTotalSize(int size)
{
	totalSize=size;
}
int OffsetTable::getTotalSize()
{
	return totalSize;
}
int OffsetTable::getParamSize()
{
	return paramSize;
}
void OffsetTable::setParamSize(int size)
{
	paramSize=size;
}
void OffsetTable::copyTo(OffsetTable*dest)
{
	if(dest)
	{
		dest->m_offset=m_offset;
		dest->m_size=m_size;
		dest->m_type=m_type;
		dest->totalSize=totalSize;
		dest->paramSize=paramSize;
	}
}

CompoundType OffsetTable::get_type(const char * symname)
{
	return m_type[std::string(symname)];
}
OffsetTable::OffsetTable()
{
	totalSize=0;
	paramSize=0;
}
bool OffsetTable::exist(const char * symname)
{
	return (m_offset.find(std::string(symname)))!=m_offset.end();
}

