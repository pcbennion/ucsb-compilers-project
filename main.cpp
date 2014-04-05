/*
	This file is provided for you to run your parser.  You should not have
	to edit anything if you did the yacc and lex files correctly.  All this
	file does is calls yyparse() and uses the visitor class ast2dot to print
	the graph
*/
#include "ast.hpp"
#include "parser.hpp"
#include "typecheck.cpp" 
#include "codegen.cpp"
#include <assert.h>

extern int yydebug; // set this to 1 if you want yyparse to dump a trace
extern int yyparse(); // this actually the parser which then calls the scanner

Program_ptr ast; // make sure to set to the final syntax tree in parser.ypp
void dopass_ast2dot(Program_ptr ast); // this is defined in ast2dot.cpp

void dopass_typecheck(Program_ptr ast, SymTab* st, ClassTable* ct) {
        Typecheck* typecheck = new Typecheck(stderr, st, ct); //create the visitor
        ast->accept(typecheck); //walk the tree with the visitor above
}

void dopass_codegen(Program_ptr ast, SymTab* st, ClassTable* ct) {
        Codegen* codegen = new Codegen(stderr, st, ct); //create the visitor
        ast->accept(codegen); //walk the tree with the visitor above
	delete codegen;
}

int main(void) {
    SymTab st; //symbol table 
    ClassTable ct;
    // set this to 1 if you would like to print a trace 
    // of the entire parsing process (it prints to stdout)
    yydebug = 0; 
    
    // after parsing, the global "ast" should be set to the
    // syntax tree that we have built up during the parse
    yyparse();  
    
    // walk over the ast and print it out as a dot file
    dopass_ast2dot( ast );
    dopass_typecheck(ast, &st, &ct); 
    dopass_codegen(ast, &st, &ct); 
    return 0;
}

