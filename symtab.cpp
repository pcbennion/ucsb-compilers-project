#include <algorithm>
#include "symtab.hpp"
#include "stdio.h"
#include <assert.h>
#include <string>

/****** SymName Implemenation **************************************/

SymName::SymName(char* const x)
{
	m_spelling = x;
	m_parent_attribute = NULL;
}

SymName::SymName(const SymName & other)
{
	m_spelling = strdup(other.m_spelling);
	m_parent_attribute = other.m_parent_attribute;
}

SymName& SymName::operator=(const SymName & other)
{
	delete m_spelling;
	SymName tmp(other);
	swap(tmp);
	return *this;
}

void SymName::swap(SymName & other)
{
	std::swap(m_spelling, other.m_spelling);
}

SymName::~SymName()
{
	delete( m_spelling );
}

void SymName::accept(Visitor *v)
{ 
	v->visitSymName(this); 
}

SymName* SymName::clone() const
{
	return new SymName(*this);
}

const char* SymName::spelling()
{
	return m_spelling;
}

const Symbol* SymName::symbol()
{
	return m_symbol;
}

void SymName::set_symbol(Symbol* s)
{
	//you should not try to reset a symbol pointer
	//once it is set, that pointer is used all 
	//sorts of places - changes to it once it is
	//set can results in weird behavior
	assert( m_symbol == NULL );
	m_symbol = s;
}


/****** SymTab Implementation **************************************/

SymTab::SymTab()
{
	m_head = new SymScope;
	m_cur_scope = m_head;
}

SymTab::~SymTab()
{
	delete m_head;
}

bool SymTab::is_dup_string(const char* name)
{
	return m_head->is_dup_string(name);
}

void SymTab::open_scope()
{
    m_cur_scope = m_cur_scope->open_scope();
    assert( m_cur_scope != NULL );
}

void SymTab::open_scope(SymScope* parent)
{
    m_cur_scope = parent->open_scope(m_cur_scope);
    assert( m_cur_scope != NULL );
}

void SymTab::close_scope()
{
    //check to make sure we don't pop more than we push
    assert( m_cur_scope != m_head );
    assert( m_cur_scope != NULL );

    m_cur_scope = m_cur_scope->close_scope();
}

bool SymTab::exist(const char* name )
{
	assert( name != NULL );
	return m_cur_scope->exist( name );
}

bool SymTab::insert(const char* name, Symbol * s )
{
	assert( name != NULL );
	assert( s != NULL );
	// the assert below fails, it is because you tried to insert
	// a pointer to a string that is already in the SymTab.  You 
	// can have duplicate names, but each needs to reside it it's
	// own chunk of memory (see example)
	//assert( is_dup_string(name) ); 
	Symbol* r = m_cur_scope->insert( name, s );
	if ( r == NULL ) return true;
	else return false;
}

bool SymTab::insert_in_parent_scope(const char* name, Symbol * s )
{
	assert( name != NULL );
	assert( s != NULL );
	// the assert below fails, it is because you tried to insert
	// a pointer to a string that is already in the SymTab.  You 
	// can have duplicate names, but each needs to reside it it's
	// own chunk of memory (see example)
	//assert( is_dup_string(name) ); 
	// make sure there is an actual parent scope
	assert( m_cur_scope->m_parent != NULL );	
	Symbol* r = m_cur_scope->m_parent->insert( name, s );
	if ( r == NULL ) return true;
	else return false;
}

SymScope* SymTab::get_current_scope()
{
	return m_cur_scope;
}

Symbol* SymTab::lookup( const char * name )
{
	assert( name != NULL );
	return m_cur_scope->lookup( name );
}

Symbol* SymTab::lookup( SymName * name )
{
	assert( name != NULL );
	return m_cur_scope->lookup( name->spelling() );
}


void SymTab::dump( FILE* f )
{
	m_head->dump(f, 0);
}

/****** SymScope Implementation **************************************/

SymScope::SymScope()
{
    m_parent = NULL;
}

SymScope::SymScope(SymScope * last, SymScope * parent)
{
    m_last = last;
    m_parent = parent;
    if (parent!=NULL) {
        parent->add_child(this);
    }
}

SymScope::~SymScope()
{
	//delete the keys, but not the symbols (symbols are linked elsewhere)
	ScopeTableType::iterator si, this_si;

	si = m_scopetable.begin();
	while ( si!=m_scopetable.end() )
	{
		std::string oldkey = si->first;
		this_si = si;
		++si;

		m_scopetable.erase( this_si );
	}

	//now delete all the children
	list<SymScope*>::iterator li;
	for( li=m_child.begin(); li!=m_child.end(); ++li )
	{
		delete *li;
	}
}

void SymScope::dump(FILE* f, int nest_level)
{
	//recursively prints out the symbol table
	//from the head down through all the childrens

	ScopeTableType::iterator si;

	//indent appropriately
	for( int i=0; i<nest_level; i++ ) { fprintf(f,"\t"); }
	fprintf(f,"+-- Symbol Scope ---\n");

	for( si = m_scopetable.begin(); si != m_scopetable.end(); ++si )
	{
		//indent appropriately
		for( int i=0; i<nest_level; i++ ) { fprintf(f,"\t"); }
		fprintf( f, "| %s \n", si->first.c_str() );
	}
	for( int i=0; i<nest_level; i++ ) { fprintf(f,"\t"); }
	fprintf(f,"+-------------\n\n");

	//now print all the children
	list<SymScope*>::iterator li;
	for( li=m_child.begin(); li!=m_child.end(); ++li )
	{
		(*li)->dump(f, nest_level+1);
	}
}

bool SymScope::is_dup_string(const char* name)
{
	ScopeTableType::iterator si;

	si = m_scopetable.find( name );
	if ( si != m_scopetable.end() ) {
		//check if the pointers match
		if ( si->first == name ) return false;
	}

	list<SymScope*>::iterator li;
	for( li=m_child.begin(); li!=m_child.end(); ++li )
	{
		bool r = (*li)->is_dup_string(name);
		if ( r==false ) return false;
	}

	//if it gets this far, there is no duplicate
	return true;
}

void SymScope::add_child(SymScope* c) 
{
	m_child.push_back(c);
}


SymScope* SymScope::open_scope()
{
    return new SymScope(this, this);
}

SymScope* SymScope::open_scope(SymScope* last)
{
    return new SymScope(last, this);
}

SymScope* SymScope::close_scope()
{
    return m_last;
}

bool SymScope::exist( const char* name )
{
	Symbol* s;
	s = lookup(name);
	// return true if name exists
	if ( s!=NULL ) return true;
	else return false;
}

Symbol* SymScope::insert( const char* name, Symbol * s )
{
	pair<ScopeTableType::iterator,bool> iret;
	typedef pair<std::string,Symbol*> hpair;
	iret = m_scopetable.insert( hpair(std::string(name),s) );
	if( iret.second == true ) {
		//insert was successfull
		return NULL;
	} else {
		//cannot insert, there was a duplicate entry
		//return a pointer to the conflicting symbol
		return iret.first->second;
	}	
}
 
Symbol* SymScope::lookup( const char * name )
{
	//first check the current table;
	ScopeTableType::const_iterator i;
	i = m_scopetable.find( std::string(name) );
	if ( i != m_scopetable.end() ) {
		return i->second;
	}

	//failing that, check all the parents;
	if ( m_parent != NULL ) {
		return m_parent->lookup(name);
	} else {
		//if this has no parents, then it cannot be found
		return NULL;
	}
}
