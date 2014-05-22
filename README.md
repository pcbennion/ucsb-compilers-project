ucsb-compilers-project
======================

A fully functional x86 compiler for a simple object-oriented language. Several of these files were provided by the instructor as a framework for automated building and testing. Files I wrote/edited consist of:
- lexer.l
- parser.ypp
- codegen.cpp
- typecheck.cpp
- small edits to symtab.cpp

The program takes input files from the following language and translates them into equivalent x86 assembly code. Parsing uses the Yacc framework to easily translate tokens into C code. Once all of the tokens are established as object, the entire parse tree is iterated through for typechecking. Each object in the tree is then visited one last time for assembly generation. 

In addition, ast2dot.cpp can be used to generate a graphical representation of the parse tree. 

The language is defined as follows:

Objects:
Name (OR 'Name from ParentName' for inheritance) {
  Variables
  Functions
};

Variables:
VarName : Type;
OR
VarName , VarName , ... , VarName : Type;

Functions:
MethodName(Param : Type, Param : Type , etc) : ReturnType {
  Local Variables
  Statements
  Return statement
};

Statements:
ID = Expression;
print Expression;
if Expression then Statement;

Expressions:
Similar to C expressions - arithmetic operators are '+, -, *, /', boolean operators are 'and, or, not', and valid comparisons are '<, <='. Function calls are Var.FuncName(Params) for other objects, FuncName(Param) for calls to the 'this' object.

Types:
Valid primitive types are Int and Bool. Local variables are allowed to have a class as their type. Additionally, function return types can be "Nothing" for returning void.

Additional Notes:
The final class must be named "Program", and must have a method named "Start()". The Start function equivalent to int main() in C.
