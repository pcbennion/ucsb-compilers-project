#include "ast.hpp"
#include "symtab.hpp"
#include "primitive.hpp"
#include "classhierarchy.hpp"
#include "stdio.h"

#include <stack>

class Ast2dot : public Visitor {
 private:
 FILE *m_out; //file for writting output
 int count; //used to give each node a uniq id
 stack<int> s; //stack for tracking parent/child pairs

 public:

 Ast2dot( FILE* out ) { 
	count = 0;
	s.push(0);
	m_out = out;
	fprintf( m_out, "digraph G { page=\"8.5,11\"; size=\"7.5, 10\"; \n" );
 }

 void finish() { 
	fprintf( m_out, "}; \n" );	
 }

 void add_edge( int p, int c )
 {
	fprintf( m_out, "\"%d\" -> \"%d\"\n", p, c );
 }

 void add_node( int c, const char* n )
 {
	fprintf( m_out, "\"%d\" [label=\"%s\"]\n" , c, n );
 }

 void add_null( int c )
 {
	fprintf( m_out, "\"%d\" [label=\"NULL\",fillcolor=red]\n" , c );
 }

 void draw( const char* n, Visitable* p) {
	count++; 			// each node gets a unique number
	add_edge( s.top(), count ); 	// from parent to this 
	add_node( count, n );		// name the this node
	s.push(count);			// now this node is the parent
	p->visit_children(this);	
	s.pop();			// now restore old parent
 }

 void draw_symname( const char* n, SymName* p) {
	count++; 			// each node gets a unique number
	add_edge( s.top(), count ); 	// from parent to this 
	// print symname strings
	fprintf( m_out, "\"%d\" [label=\"%s\\n\\\"%s\\\"\"]\n" , count, n, p->spelling() );
 }

 void draw_primitive( const char* n, Primitive* p) {
	count++; 			// each node gets a unique number
	add_edge( s.top(), count ); 	// from parent to this 
	fprintf( m_out, "\"%d\" [label=\"%s\\n%d\"]\n" , count, n, p->m_data );
 }
 
 void draw_classname( const char* n, ClassName* p) {
   count++; 			// each node gets a unique number
   add_edge( s.top(), count ); 	// from parent to this 
   // print symname strings
   fprintf( m_out, "\"%d\" [label=\"%s\\n\\\"%s\\\"\"]\n" , count, n, p->spelling() );
  }
 
 void visitProgramImpl(ProgramImpl *p) { draw("ProgramImpl", p); }
 void visitClassImpl(ClassImpl *p) { draw("ClassImpl", p); }
 void visitDeclarationImpl(DeclarationImpl *p) { draw("DeclarationImpl", p); }
 void visitMethodImpl(MethodImpl *p) { draw("MethodImpl", p); }
 void visitMethodBodyImpl(MethodBodyImpl *p) { draw("MethodBodyImpl", p); }
 void visitParameterImpl(ParameterImpl *p) { draw("ParameterImpl", p); }
 void visitAssignment(Assignment *p) { draw("Assignment", p); }
 void visitIf(If *p) { draw("If", p); }
 void visitPrint(Print *p) { draw("Print", p); }
 void visitReturnImpl(ReturnImpl *p) { draw("ReturnImpl", p); }
 void visitTInteger(TInteger *p) { draw("TInteger", p); }
 void visitTBoolean(TBoolean *p) { draw("TBoolean", p); }
 void visitTNothing(TNothing *p) { draw("TNothing", p); }
 void visitTObject(TObject *p) { draw("TObject", p); }
 void visitClassIDImpl(ClassIDImpl *p) { draw("ClassIDImpl", p); }
 void visitVariableIDImpl(VariableIDImpl *p) { draw("VariableIDImpl", p); }
 void visitMethodIDImpl(MethodIDImpl *p) { draw("MethodIDImpl", p); }
 void visitPlus(Plus *p) { draw("Plus", p); }
 void visitMinus(Minus *p) { draw("Minus", p); }
 void visitTimes(Times *p) { draw("Times", p); }
 void visitDivide(Divide *p) { draw("Divide", p); }
 void visitAnd(And *p) { draw("And", p); }
 void visitLessThan(LessThan *p) { draw("LessThan", p); }
 void visitLessThanEqualTo(LessThanEqualTo *p) { draw("LessThanEqualTo", p); }
 void visitNot(Not *p) { draw("Not", p); }
 void visitUnaryMinus(UnaryMinus *p) { draw("UnaryMinus", p); }
 void visitMethodCall(MethodCall *p) { draw("MethodCall", p); }
 void visitSelfCall(SelfCall *p) { draw("SelfCall", p); }
 void visitVariable(Variable *p) { draw("Variable", p); }
 void visitIntegerLiteral(IntegerLiteral *p) { draw("IntegerLiteral", p); }
 void visitBooleanLiteral(BooleanLiteral *p) { draw("BooleanLiteral", p); }
 void visitNothing(Nothing *p) {draw("Nothing", p); }

 //special cases
 void visitSymName(SymName *p) { draw_symname("SymName",p); }
 void visitPrimitive(Primitive *p) { draw_primitive("Primitive",p); }
 void visitClassName(ClassName *p) { draw_classname("ClassName",p); }

 //very special case
 void visitNullPointer() { count++; add_edge(s.top(), count); add_null(count); }
};

void dopass_ast2dot(Program_ptr ast) {
        Ast2dot* ast2dot = new Ast2dot(stdout); //create the visitor
        ast->accept(ast2dot); //walk the tree with the visitor above
	ast2dot->finish(); // finalize the printout
	delete ast2dot;
}
