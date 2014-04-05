#include "ast.hpp"
#include "symtab.hpp"
#include "classhierarchy.hpp"
#include "primitive.hpp"
#include "assert.h"
#include <typeinfo>
#include <stdio.h>

class Codegen : public Visitor
{
  private:
  
  FILE * m_outputfile;
  SymTab *m_symboltable;
  ClassTable *m_classtable;
  
  const char * heapStart="_heap_start";
  const char * heapTop="_heap_top";
  const char * printFormat=".LC0";
  const char * printFun="Print";
  const char * currClassName;
  
  OffsetTable*currClassOffset;
  OffsetTable*currMethodOffset;

  bool inMethod;
  
  // basic size of a word (integers and booleans) in bytes
  static const int wordsize = 4;
  
  int label_count; //access with new_label
  
  // ********** Helper functions ********************************
  
  // this is used to get new unique labels (cleverly named label1, label2, ...)
  int new_label() { return label_count++; }

  // PART 1:
  // 1) get arithmetic expressions on integers working:
  //	  you wont really be able to run your code,
  //	  but you can visually inspect it to see that the correct
  //    chains of opcodes are being generated.
  // 2) get function calls working:
  //    if you want to see at least a very simple program compile
  //    and link successfully against gcc-produced code, you
  //    need to get at least this far
  // 3) get boolean operation working
  //    before we can implement any of the conditional control flow 
  //    stuff, we need to have booleans worked out.  
  // 4) control flow:
  //    we need a way to have if-elses and for loops in our language. 
  //
  // Hint: Symbols have an associated member variable called m_offset
  //    That offset can be used to figure out where in the activation 
  //    record you should look for a particuar variable
  
  ///////////////////////////////////////////////////////////////////////////////
  //
  //  function_prologue
  //  function_epilogue
  //
  //  Together these two functions implement the callee-side of the calling
  //  convention.  A stack frame has the following layout:
  //
  //                          <- SP (before pre-call / after epilogue)
  //  high -----------------
  //	   | actual arg 1  |
  //	   | ...           |
  //	   | actual arg n  |
  //	   -----------------
  //	   |  Return Addr  | 
  //	   =================
  //	   | temporary 1   |    <- SP (when starting prologue)
  //	   | ...           |
  //	   | temporary n   | 
  //  low -----------------   <- SP (when done prologue)
  //
  //
  //			  ||		
  //			  ||
  //			 \  /
  //			  \/
  //
  //
  //  The caller is responsible for placing the actual arguments
  //  and the return address on the stack. Actually, the return address
  //  is put automatically on the stack as part of the x86 call instruction.
  //
  //  On function entry, the callee
  //
  //  (1) allocates space for the callee's temporaries on the stack
  //  
  //  (2) saves callee-saved registers (see below) - including the previous activation record pointer (%ebp)
  //
  //  (3) makes the activation record pointer (frame pointer - %ebp) point to the start of the temporary region
  //
  //  (4) possibly copies the actual arguments into the temporary variables to allow easier access
  //
  //  On function exit, the callee:
  //
  //  (1) pops the callee's activation record (temporay area) off the stack
  //  
  //  (2) restores the callee-saved registers, including the activation record of the caller (%ebp)	 
  //
  //  (3) jumps to the return address (using the x86 "ret" instruction, this automatically pops the 
  //	  return address off the stack
  //
  //////////////////////////////////////////////////////////////////////////////
  //
  // Since we are interfacing with code produced by GCC, we have to respect the 
  // calling convention that GCC demands:
  //
  // Contract between caller and callee on x86: 
  //	 * after call instruction: 
  //		   o %eip points at first instruction of function 
  //		   o %esp+4 points at first argument 
  //		   o %esp points at return address 
  //	 * after ret instruction: 
  //		   o %eip contains return address 
  //		   o %esp points at arguments pushed by caller 
  //		   o called function may have trashed arguments 
  //		   o %eax contains return value (or trash if function is void) 
  //		   o %ecx, %edx may be trashed 
  //		   o %ebp, %ebx, %esi, %edi must contain contents from time of call 
  //	 * Terminology: 
  //		   o %eax, %ecx, %edx are "caller save" registers 
  //		   o %ebp, %ebx, %esi, %edi are "callee save" registers 
  ////////////////////////////////////////////////////////////////////////////////
  
  void init()
  {
    fprintf( m_outputfile, ".text\n\n");
    fprintf( m_outputfile, ".comm %s,4,4\n", heapStart);
    fprintf( m_outputfile, ".comm %s,4,4\n\n", heapTop);
    
    fprintf( m_outputfile, "%s:\n", printFormat);
    fprintf( m_outputfile, "       .string \"%%d\\n\"\n");
    fprintf( m_outputfile, "       .text\n");
    fprintf( m_outputfile, "       .globl  %s\n",printFun);
    fprintf( m_outputfile, "       .type   %s, @function\n\n",printFun);
    fprintf( m_outputfile, ".global %s\n",printFun);
    fprintf( m_outputfile, "%s:\n",printFun);
    fprintf( m_outputfile, "       pushl   %%ebp\n");
    fprintf( m_outputfile, "       movl    %%esp, %%ebp\n");
    fprintf( m_outputfile, "       movl    8(%%ebp), %%eax\n");
    fprintf( m_outputfile, "       pushl   %%eax\n");
    fprintf( m_outputfile, "       pushl   $.LC0\n");
    fprintf( m_outputfile, "       call    printf\n");
    fprintf( m_outputfile, "       addl    $8, %%esp\n");
    fprintf( m_outputfile, "       leave\n");
    fprintf( m_outputfile, "       ret\n\n");
  }

  void start(int programSize)
  {
    fprintf( m_outputfile, "# Start Function\n");
    fprintf( m_outputfile, ".global Start\n");
    fprintf( m_outputfile, "Start:\n");
    fprintf( m_outputfile, "        pushl   %%ebp\n");
    fprintf( m_outputfile, "        movl    %%esp, %%ebp\n");
    fprintf( m_outputfile, "        movl    8(%%ebp), %%ecx\n");
    fprintf( m_outputfile, "        movl    %%ecx, %s\n",heapStart);
    fprintf( m_outputfile, "        movl    %%ecx, %s\n",heapTop);
    fprintf( m_outputfile, "        addl    $%d, %s\n",programSize,heapTop);
    fprintf( m_outputfile, "        pushl   %s \n",heapStart);
    fprintf( m_outputfile, "        call    Program_start \n");
    fprintf( m_outputfile, "        leave\n");
    fprintf( m_outputfile, "        ret\n");
  }

  void allocSpace(int size)
  {
	// Optional WRITE ME
  }

////////////////////////////////////////////////////////////////////////////////
public:
  
  Codegen(FILE * outputfile, SymTab * st, ClassTable* ct)
  {
    m_outputfile = outputfile;
    m_symboltable = st;
    m_classtable = ct;
    label_count = 0;
    currMethodOffset=currClassOffset=NULL;
  }
  //=====================================================================================================================
  void visitProgramImpl(ProgramImpl *p) {

	init();
    // Visit the children
    p->visit_children(this);

    start(m_classtable->lookup("Program")->offset->getTotalSize());

  }
  //=====================================================================================================================
  void visitClassImpl(ClassImpl *p) {

      fprintf(m_outputfile, "############ CLASS\n");

      // Set current class name for function labels
      currClassName = dynamic_cast<ClassIDImpl*>(p->m_classid_1)->m_classname->spelling();

      // Create this class's classnode and insert into class table
      ClassNode* node = new ClassNode();
      node->name = new ClassName(currClassName);
      node->superClass = NULL;
      node->scope = new SymScope(); // only used for holding methods
      node->p = p;
      if(p->m_classid_2!=NULL) {
          const char* superclass = dynamic_cast<ClassIDImpl*>(p->m_classid_2)->m_classname->spelling();
          assert(m_classtable->exist(superclass));
          node->superClass = new ClassName(superclass);
          m_classtable->lookup(superclass)->offset->copyTo(node->offset);
      }
      currClassOffset = node->offset;
      m_classtable->insert(node->name->spelling(), node);

      inMethod = false;

      // Visit the children
      p->visit_children(this);

      fprintf(m_outputfile, "############\n\n");
  }
  //=====================================================================================================================
  void visitDeclarationImpl(DeclarationImpl *p) {

      fprintf(m_outputfile, "#### DECLARATION\n");

      // Visit the children
      p->visit_children(this);

      // Get type and decide on allocated size
      Basetype type = p->m_type->m_attribute.m_type.baseType;
      assert(type == bt_boolean || type == bt_integer || type == bt_object);
      int varSize = 4;

      // Iterate through list of variables, allocating and inserting into offset table
      list<VariableID_ptr> *l = p->m_variableid_list;
      list<VariableID_ptr>::iterator it;
      OffsetTable* table; bool isClassDec; int offsetDir;
      if(inMethod) { table = currMethodOffset; isClassDec = false; offsetDir = -1; }
      else{ table = currClassOffset; isClassDec = true; offsetDir = 1; }
      VariableIDImpl* var; VariableID_ptr ptr;
      int curroffset;
      for(it=l->begin(); it!=l->end(); ++it) {
            ptr = (VariableID_ptr)*it;
            var = dynamic_cast<VariableIDImpl*> (ptr);
            // Allocate space in stack/heap if not a class declaration
            if(type == bt_object && !isClassDec) {
                const char* c = p->m_type->m_attribute.m_type.classType.classID;
                assert(c != "");
                assert(m_classtable->exist(c));
                ClassNode* node = m_classtable->lookup(c);
                fprintf( m_outputfile, "  pushl   %s\n",heapTop); // push current heap top to stack as pointer to obj
                fprintf( m_outputfile, "  addl    $%d, %s\n",node->offset->getTotalSize(),heapTop); // allocate in heap
            } else if(!isClassDec) {
                fprintf(m_outputfile, "  subl $%i, %%esp\n", varSize);
            }
            // Insert into and update offset table
            curroffset = table->getTotalSize();
            table->insert(var->m_symname->spelling(), offsetDir*curroffset, varSize, p->m_type->m_attribute.m_type.classType);
            //fprintf( m_outputfile, "%i, %i, %i, %i\n", inMethod, offsetDir, curroffset, varSize);
            table->setTotalSize(offsetDir*(offsetDir*curroffset+offsetDir*varSize));
      }

      fprintf(m_outputfile, "####\n");
  }
  //=====================================================================================================================
  void visitMethodImpl(MethodImpl *p) {

      inMethod = true;

      fprintf(m_outputfile, "######## METHOD\n");

      // Create function label from class name and method name
      const char* funcname = dynamic_cast<MethodIDImpl*>(p->m_methodid)->m_symname->spelling();
      fprintf(m_outputfile, "%s_%s:\n", currClassName, funcname);


      // Prologue - Push old ebp to stack, update ebp to current stack pointer
      fprintf(m_outputfile, "  pushl %%ebp\n");
      fprintf(m_outputfile, "  movl %%esp, %%ebp\n");

      // Set offset tables
      assert(m_classtable->exist(currClassName));
      ClassNode* node = m_classtable->lookup(currClassName);
      currClassOffset = node->offset;
      currMethodOffset = new OffsetTable();
      currMethodOffset->setTotalSize(4); // make space in table for %ebp

      // Add function to classnode's scope
      node->scope->insert(funcname, new Symbol());
      assert(node->scope->exist(funcname));

      // Iterate through parameters, saving to offset table
      list<Parameter_ptr> *l = p->m_parameter_list;
      list<Parameter_ptr>::iterator it;
      OffsetTable* table = currMethodOffset;
      ParameterImpl* param; Parameter_ptr ptr;
      int varSize = 4;
      int curroffset = 12; // leave space for return address and the 'this object' pointer
      for(it=l->begin(); it!=l->end(); ++it) {
            ptr = (Parameter_ptr)*it;
            param = dynamic_cast<ParameterImpl*> (ptr);
            table->insert(dynamic_cast<VariableIDImpl*>(param->m_variableid)->m_symname->spelling(), curroffset, varSize, param->m_type->m_attribute.m_type.classType);
            curroffset += varSize;
      }

      // Visit the children
      p->visit_children(this);

      // Epilogue - deallocate locals, set ebp to old ebp, return
      curroffset = currMethodOffset->getTotalSize();
      fprintf(m_outputfile, "  addl $%i, %%esp\n", curroffset-4); // everything but old ebp
      fprintf( m_outputfile,"  leave\n");
      fprintf( m_outputfile,"  ret\n");
      fprintf(m_outputfile, "########\n");

      // Dereference the local offset table
      currMethodOffset == NULL;

      inMethod = false;
  }
  //=====================================================================================================================
  void visitMethodBodyImpl(MethodBodyImpl *p) {

      // Visit the children
      p->visit_children(this);

  }
  //=====================================================================================================================
  void visitParameterImpl(ParameterImpl *p) {
		
      // Visit the children
      p->visit_children(this);

  }
  //=====================================================================================================================
  void visitAssignment(Assignment *p) {

      fprintf(m_outputfile, "#### ASSIGN\n");

      // Visit the children
      p->visit_children(this);

      // Look up variable offset in offset table
      OffsetTable* table = currMethodOffset; bool inClass = false;
      const char* name = dynamic_cast<VariableIDImpl*>(p->m_variableid)->m_symname->spelling();
      if(!table->exist(name)) {table = currClassOffset; inClass = true;}
      assert(table->exist(name));
      int offset = table->get_offset(name);

      // Pop from stack and save to either stack or heap
      if(!inClass) {
          fprintf(m_outputfile, "  popl %%eax\n");
          fprintf(m_outputfile, "  movl %%eax, %i(%%ebp)\n", offset);
      } else {
          fprintf(m_outputfile, "  popl %%eax\n");
          fprintf(m_outputfile, "  movl 8(%%ebp), %%ebx\n");
          fprintf(m_outputfile, "  movl %%eax, %i(%%ebx)\n", offset);
      }
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitIf(If *p) {

      fprintf(m_outputfile, "#### CNDTL\n");

      // Get int for next branch flag
      int loc = new_label();

      // Visit expression child to push value, create conditional command
      p->m_expression->accept(this);
      fprintf( m_outputfile, "  popl %%eax\n");
      fprintf( m_outputfile, "  cmp  $1, %%eax\n");
      fprintf( m_outputfile, "  jne L%i\n", loc);

      // Visit statement child to produce branched statement
      p->m_statement->accept(this);

      // Create label
      fprintf( m_outputfile, "L%i:\n", loc);
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitPrint(Print *p) {

      fprintf(m_outputfile, "#### PRINT\n");
	
      // Visit the children
      p->visit_children(this);

      // Call print on pushed result of child expression
      fprintf(m_outputfile, "  call Print\n");
      fprintf(m_outputfile, "  addl $4, %%esp\n"); // clean up parameter
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitReturnImpl(ReturnImpl *p) {

      // Visit the children
      p->visit_children(this);

      fprintf(m_outputfile, "#### RETRN\n");

      // Pop result of child expression to %ebx, which won't be used before the end of the function
      if(p->m_attribute.m_type.baseType != bt_nothing) fprintf(m_outputfile, "  popl %%ebx\n");
      else fprintf(m_outputfile, "  movl $0, %%ebx\n"); // Return 0 if value is a Nothing

      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitTInteger(TInteger *p) {

      // Visit the children
      p->visit_children(this);

  }
  //=====================================================================================================================
  void visitTBoolean(TBoolean *p) {

      // Visit the children
      p->visit_children(this);

  }
  //=====================================================================================================================
  void visitTNothing(TNothing *p) {

      // Visit the children
      p->visit_children(this);

  }
  //=====================================================================================================================
  void visitTObject(TObject *p) {

      // Visit the children
      p->visit_children(this);

  }
  //=====================================================================================================================
  void visitClassIDImpl(ClassIDImpl *p) {

      // Visit the children
      p->visit_children(this);

  }
  //=====================================================================================================================
  void visitVariableIDImpl(VariableIDImpl *p) {

      // Visit the children
      p->visit_children(this);

  }
  //=====================================================================================================================
  void visitMethodIDImpl(MethodIDImpl *p) {

      // Visit the children
      p->visit_children(this);

  }
  //=====================================================================================================================
  void visitPlus(Plus *p) {

     fprintf(m_outputfile, "#### ADD\n");

     // Visit the children
     p->visit_children(this);

     fprintf( m_outputfile, "  popl %%ebx\n");
     fprintf( m_outputfile, "  popl %%eax\n");
     fprintf( m_outputfile, "  addl %%ebx, %%eax\n");
     fprintf( m_outputfile, "  pushl %%eax\n");
     fprintf(m_outputfile, "####\n");
  }
  //=====================================================================================================================
  void visitMinus(Minus *p) {

      fprintf(m_outputfile, "#### SUB\n");

      // Visit the children
      p->visit_children(this);

      fprintf( m_outputfile, "  popl %%ebx\n");
      fprintf( m_outputfile, "  popl %%eax\n");
      fprintf( m_outputfile, "  subl %%ebx, %%eax\n");
      fprintf( m_outputfile, "  pushl %%eax\n");
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitTimes(Times *p) {

      fprintf(m_outputfile, "#### MLT\n");

      // Visit the children
      p->visit_children(this);

      fprintf( m_outputfile, "  popl %%ebx\n");
      fprintf( m_outputfile, "  popl %%eax\n");
      fprintf( m_outputfile, "  imul %%ebx, %%eax\n");
      fprintf( m_outputfile, "  pushl %%eax\n");
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitDivide(Divide *p) {

      fprintf(m_outputfile, "#### DIV\n");

      // Visit the children
      p->visit_children(this);

      fprintf( m_outputfile, "  movl $0, %%edx\n"); // clear dividend
      fprintf( m_outputfile, "  popl %%ebx\n");
      fprintf( m_outputfile, "  popl %%eax\n");
      fprintf( m_outputfile, "  cdq\n"); // sign extend eax into edx
      fprintf( m_outputfile, "  idiv %%ebx\n");
      fprintf( m_outputfile, "  pushl %%eax\n");
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitAnd(And *p) {

      fprintf(m_outputfile, "#### AND\n");

      // Get two label locations
      int loc1 = new_label(); int loc2 = new_label();

      // Visit the children
      p->visit_children(this);

      fprintf( m_outputfile, "  popl %%ebx\n");
      fprintf( m_outputfile, "  popl %%eax\n");
      fprintf( m_outputfile, "  cmp $0, %%eax\n");
      fprintf( m_outputfile, "  je L%i\n", loc1);
      fprintf( m_outputfile, "  cmp $0, %%ebx\n");
      fprintf( m_outputfile, "  je L%i\n", loc1);
      fprintf( m_outputfile, "  mov $1, %%eax\n");
      fprintf( m_outputfile, "  jmp L%i\n", loc2);
      fprintf( m_outputfile, "L%i:\n", loc1);
      fprintf( m_outputfile, "  mov $0, %%eax\n");
      fprintf( m_outputfile, "L%i:\n", loc2);
      fprintf( m_outputfile, "  pushl %%eax\n");
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitLessThan(LessThan *p) {

      fprintf(m_outputfile, "#### LT\n");

      // Get two label locations
      int loc1 = new_label(); int loc2 = new_label();

      // Visit the children
      p->visit_children(this);

      fprintf( m_outputfile, "  popl %%ebx\n");
      fprintf( m_outputfile, "  popl %%eax\n");
      fprintf( m_outputfile, "  cmp %%ebx, %%eax\n");
      fprintf( m_outputfile, "  jl L%i\n", loc1);
      fprintf( m_outputfile, "  pushl $0\n");
      fprintf( m_outputfile, "  jmp L%i\n", loc2);
      fprintf( m_outputfile, "L%i:\n", loc1);
      fprintf( m_outputfile, "  pushl $1\n");
      fprintf( m_outputfile, "L%i:\n", loc2);
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitLessThanEqualTo(LessThanEqualTo *p) {

      fprintf(m_outputfile, "#### LT\n");

      // Get two label locations
      int loc1 = new_label(); int loc2 = new_label();

      // Visit the children
      p->visit_children(this);

      fprintf( m_outputfile, "  popl %%ebx\n");
      fprintf( m_outputfile, "  popl %%eax\n");
      fprintf( m_outputfile, "  cmp %%ebx, %%eax\n");
      fprintf( m_outputfile, "  jle L%i\n", loc1);
      fprintf( m_outputfile, "  pushl $0\n");
      fprintf( m_outputfile, "  jmp L%i\n", loc2);
      fprintf( m_outputfile, "L%i:\n", loc1);
      fprintf( m_outputfile, "  pushl $1\n");
      fprintf( m_outputfile, "L%i:\n", loc2);
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitNot(Not *p) {

      fprintf(m_outputfile, "#### NOT\n");

      // Get two label locations
      int loc1 = new_label(); int loc2 = new_label();

      // Visit the children
      p->visit_children(this);

      fprintf( m_outputfile, "  popl %%eax\n");
      fprintf( m_outputfile, "  cmp $0, %%eax\n");
      fprintf( m_outputfile, "  jne L%i\n", loc1);
      fprintf( m_outputfile, "  mov $1, %%eax\n");
      fprintf( m_outputfile, "  jmp L%i\n", loc2);
      fprintf( m_outputfile, "L%i:\n", loc1);
      fprintf( m_outputfile, "  mov $0, %%eax\n");
      fprintf( m_outputfile, "L%i:\n", loc2);
      fprintf( m_outputfile, "  pushl %%eax\n");
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitUnaryMinus(UnaryMinus *p) {

      fprintf(m_outputfile, "#### NEG\n");

      // Visit the children
      p->visit_children(this);

      fprintf( m_outputfile, "  popl %%eax\n");
      fprintf( m_outputfile, "  negl %%eax\n");
      fprintf( m_outputfile, "  pushl %%eax\n");
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitMethodCall(MethodCall *p) {

      fprintf(m_outputfile, "#### METHC\n");

      // Visit variable and method name
      p->m_methodid->accept(this);
      p->m_variableid->accept(this);

      // Grab variable's classname (for jump label) from offset table
      OffsetTable* table = currMethodOffset; bool inClass=false;
      const char* varname = dynamic_cast<VariableIDImpl*>(p->m_variableid)->m_symname->spelling();
      if(!table->exist(varname)) {table = currClassOffset; inClass=true;}
      assert(table->exist(varname));
      CompoundType type = table->get_type(varname);
      const char* name;
      name = type.classID;

      // Visit parameters in reverse order, so their results will end up on the stack in x86 convention order
      int numparams = 0;
      list<Expression_ptr> *l = p->m_expression_list;
      list<Expression_ptr>::iterator it;
      Expression* exp; Expression_ptr ptr;
      for(it=l->end(); it!=l->begin();) {
            --it;
            ptr = (Expression_ptr)*it;
            exp = dynamic_cast<Expression*> (ptr);
            exp->accept(this);
            numparams++;
      }

      // Push referenced object's pointer to stack as final parameter
      if(!inClass) {
        fprintf( m_outputfile, "  pushl %i(%%ebp)\n", table->get_offset(varname));
      } else {
        fprintf( m_outputfile, "  movl 8(%%ebp), %%ebx\n");
        fprintf( m_outputfile, "  pushl %i(%%ebx)\n", table->get_offset(varname));
      }
      numparams++;

      // Find class/superclass that contains the function
      const char* funcname = dynamic_cast<MethodIDImpl*>(p->m_methodid)->m_symname->spelling();
      ClassNode* node = m_classtable->lookup(name);
      assert(node!=NULL);
      while(!(node->scope->exist(funcname))) {
          assert(node->superClass != NULL);
          node = m_classtable->lookup(node->superClass);
      }

      // Call the function
      fprintf(m_outputfile, "  call %s_%s\n", node->name->spelling(), funcname);

      // Clean up parameters
      fprintf(m_outputfile, "  addl $%i, %%esp\n", numparams*4);

      // Push return value
      fprintf(m_outputfile, "  pushl %%ebx\n");
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitSelfCall(SelfCall *p) {

      fprintf(m_outputfile, "#### SELFC\n");

      // Visit method name
      p->m_methodid->accept(this);

      // Visit parameters in reverse order, so their results will end up on the stack in x86 convention order
      int numparams = 0;
      list<Expression_ptr> *l = p->m_expression_list;
      list<Expression_ptr>::iterator it;
      Expression* exp; Expression_ptr ptr;
      for(it=l->end(); it!=l->begin();) {
            --it;
            ptr = (Expression_ptr)*it;
            exp = dynamic_cast<Expression*> (ptr);
            exp->accept(this);
            numparams++;
      }

      // Push current object's pointer to stack as final parameter
      fprintf( m_outputfile, "  pushl 8(%%ebp)\n");
      numparams++;

      // Find class/superclass that contains the function
      const char* funcname = dynamic_cast<MethodIDImpl*>(p->m_methodid)->m_symname->spelling();
      ClassNode* node = m_classtable->lookup(currClassName);
      assert(node!=NULL);
      while(!(node->scope->exist(funcname))) {
          assert(node->superClass != NULL);
          node = m_classtable->lookup(node->superClass);
      }

      // Call the function
      fprintf(m_outputfile, "  call %s_%s\n", node->name->spelling(), funcname);

      // Clean up parameters
      fprintf(m_outputfile, "  addl $%i, %%esp\n", numparams*4);

      // Push return value
      fprintf(m_outputfile, "  pushl %%ebx\n");
      fprintf(m_outputfile, "####\n");

  }
  //=====================================================================================================================
  void visitVariable(Variable *p) {

      fprintf(m_outputfile, "## VAR\n");

      // Visit the children
      p->visit_children(this);

      // Look up variable offset and type in offset table
      OffsetTable* table = currMethodOffset;
      bool inClass=false;
      const char* name = dynamic_cast<VariableIDImpl*>(p->m_variableid)->m_symname->spelling();
      if(!table->exist(name)) {table = currClassOffset; inClass=true;}
      assert(table->exist(name));
      int offset = table->get_offset(name);

      // Push from either stack or heap, depending on where variable is located
      if(!inClass) fprintf(m_outputfile, "  pushl %i(%%ebp)\n", offset);
      else {
          fprintf(m_outputfile, "  mov 8(%%ebp), %%eax\n", offset);
          fprintf(m_outputfile, "  pushl %i(%%eax)\n", offset);
      }
      fprintf(m_outputfile, "##\n");
  }
  //=====================================================================================================================
  void visitIntegerLiteral(IntegerLiteral *p) {
      // Visit the children
      p->visit_children(this);
  }
  //=====================================================================================================================
  void visitBooleanLiteral(BooleanLiteral *p) {
      // Visit the children
      p->visit_children(this);
  }
  //=====================================================================================================================
  void visitNothing(Nothing *p) {}
  //=====================================================================================================================
  void visitSymName(SymName *p) {}
  //=====================================================================================================================
  void visitPrimitive(Primitive *p) {
      fprintf(m_outputfile, "  pushl $%i\n", p->m_data);
  }
  //=====================================================================================================================
  void visitClassName(ClassName *p) {}
  //=====================================================================================================================
  void visitNullPointer() {}
  //=====================================================================================================================
};




