#include <iostream>
#include "ast.hpp"
#include "symtab.hpp"
#include "primitive.hpp"
#include "classhierarchy.hpp"
#include "assert.h"
#include <typeinfo>
#include <stdio.h>

/***********

    Typechecking has some code already provided, you need to fill in the blanks. You should only add code where WRITEME labels
    are inserted. You can just search for WRITEME to find all the instances.
    
    You can find descriptions of every type condition that you must check for in the project webpage. Each condition has its own
    error condition that should be thrown when it fails. Every error condition listed there and also in the "errortype" enum in
    this file must be used somewhere in your code.
    
    Be careful when throwing errors - always throw an error of the right type, using the m_attribute of the node you're visiting
    when performing the check. Sometimes you'll see errors being thrown at strange line numbers; that is okay, don't let that
    bother you as long as you follow the above principle.

*****/

class Typecheck : public Visitor {
    private:
    FILE* m_errorfile;
    SymTab* m_symboltable;
    ClassTable* m_classtable;
    
    const char * bt_to_string(Basetype bt) {
        switch (bt) {
            case bt_undef:    return "bt_undef";
            case bt_integer:  return "bt_integer";
            case bt_boolean:  return "bt_boolean";
            case bt_function: return "bt_function";
            case bt_object:   return "bt_object";
            default:
                              return "unknown";
        }
    }
    
    // the set of recognized errors
    enum errortype 
    {
        no_program,
        no_start,
        start_args_err,
        
        dup_ident_name,
        sym_name_undef,
        sym_type_mismatch,
        call_narg_mismatch,
        call_args_mismatch,
        ret_type_mismatch,
        
        incompat_assign,
        if_pred_err,
        
        expr_type_err,
        
        no_class_method,
    };
    
    // Throw errors using this method
    void t_error( errortype e, Attribute a ) 
    {
        fprintf(m_errorfile,"on line number %d, ", a.lineno );
        
        switch( e ) {
            case no_program: fprintf(m_errorfile,"error: no Program class\n"); break;
            case no_start: fprintf(m_errorfile,"error: no start function in Program class\n"); break;
            case start_args_err: fprintf(m_errorfile,"error: start function has arguments\n"); break;
            
            case dup_ident_name: fprintf(m_errorfile,"error: duplicate identifier name in same scope\n"); break;
            case sym_name_undef: fprintf(m_errorfile,"error: symbol by name undefined\n"); break;
            case sym_type_mismatch: fprintf(m_errorfile,"error: symbol by name defined, but of unexpected type\n"); break;
            case call_narg_mismatch: fprintf(m_errorfile,"error: function call has different number of args than the declaration\n"); break;
            case call_args_mismatch: fprintf(m_errorfile,"error: type mismatch in function call args\n"); break;
            case ret_type_mismatch: fprintf(m_errorfile, "error: type mismatch in return statement\n"); break;
            
            case incompat_assign: fprintf(m_errorfile,"error: types of right and left hand side do not match in assignment\n"); break;
            case if_pred_err: fprintf(m_errorfile,"error: predicate of if statement is not boolean\n"); break;
            
            case expr_type_err: fprintf(m_errorfile,"error: incompatible types used in expression\n"); break;
            
            case no_class_method: fprintf(m_errorfile,"error: function doesn't exist in object\n"); break;
            
            
            default: fprintf(m_errorfile,"error: no good reason\n"); break;
        }
        exit(1);
    }
    
    public:
    
    Typecheck(FILE* errorfile, SymTab* symboltable,ClassTable*ct) {
        m_errorfile = errorfile;
        m_symboltable = symboltable;
        m_classtable = ct;
    }

    //=====================================================================================================================

    void visitProgramImpl(ProgramImpl *p) {

        // Iterate through all classes for visiting
        char* const prog = (char*)"Program";
        list<Class_ptr>::iterator i;
        Class_ptr ptr;
        for(i=p->m_class_list->begin(); i!=p->m_class_list->end(); ++i) {
            ptr=*i;
            if(m_classtable->exist(prog)) // Program class cannot not be last
                t_error(no_program, p->m_attribute);
            ptr->accept(this);
        }

        // Check to see if program class exists
        if(!m_classtable->exist(prog))
            t_error(no_program, p->m_attribute);
    }

    //=====================================================================================================================

    void visitClassImpl(ClassImpl *p) {

        // First, make a new scope


        // Fetch classname and check for duplicate declarations
        ClassName* id = ((dynamic_cast<ClassIDImpl *>(p->m_classid_1))->m_classname);
        if(m_classtable->exist(id)) t_error(dup_ident_name, p->m_attribute);

        // Check if there is a superclass, and add to table
        if(p->m_classid_2!=NULL) { // If superclass isn't null, check to see if it exists before adding to table
            ClassName* superclassID = ((dynamic_cast<ClassIDImpl*>(p->m_classid_2))->m_classname);
            ClassNode* superclass = m_classtable->lookup(superclassID);
            if(superclass==NULL) t_error(sym_name_undef, p->m_attribute);
            // add to table, open scope in superclass scope
            m_symboltable->open_scope(superclass->scope);
            m_classtable->insert(id, superclassID, p,  m_symboltable->get_current_scope());
        } else { // If no superclass, add to table in default scope with no parent
            m_symboltable->open_scope();
            m_classtable->insert(id, NULL, p, m_symboltable->get_current_scope());
        }

        // Visit the children
        p->visit_children(this);

        // If name is "Program", check for a function named "Start", with no arguments
        if(strcmp(id->spelling(), "Program") == 0) {
            Symbol* start = m_symboltable->lookup("start");
            if(start == NULL) t_error(no_start, p->m_attribute);
            else {
                if(start->methodType.returnType.baseType != bt_nothing) t_error(no_start, p->m_attribute);
                if(start->methodType.argsType.size()!= 0) t_error(start_args_err, p->m_attribute);
            }
        }

        // Debug dump
        // m_symboltable->get_current_scope()->dump(m_errorfile, 0);

        // Lastly, close the scope
        m_symboltable->close_scope();
    }

    //=====================================================================================================================

    void visitDeclarationImpl(DeclarationImpl *p) {


        // Visit the children
        p->visit_children(this);

        // Set type
        p->m_attribute.m_type=p->m_type->m_attribute.m_type;

        // Iterate through attached IDs and add to table using set type
        list<VariableID_ptr>::iterator i;
        VariableID_ptr v;
        SymName* n;
        for(i=p->m_variableid_list->begin(); i!=p->m_variableid_list->end(); ++i){
            v=*i;
            n = (dynamic_cast<VariableIDImpl *>(v))->m_symname;
            // Add to symbol table
            bool success = m_symboltable->insert(n->spelling(), (Symbol*)&(p->m_attribute.m_type));
            if(!success) t_error(dup_ident_name, p->m_attribute); // Ensure no duplicates in scope
        }
    }

    //=====================================================================================================================

    void visitMethodImpl(MethodImpl *p) {

        // Add to symbol table. Symbol type info will be altered as we go
        // Have to do it this way for method to be at class scope
        p->m_methodid->accept(this);
        bool success = m_symboltable->insert(   dynamic_cast<MethodIDImpl*>(p->m_methodid)->m_symname->spelling(),
                                                (Symbol*)&(p->m_attribute.m_type));
        if(!success) t_error(dup_ident_name, p->m_attribute); // Ensure no duplicates in scope

        // Make a new scope
        m_symboltable->open_scope();

        // Visit type and parameters only, saving types to symbol in table
        p->m_type->accept(this);
        p->m_attribute.m_type.baseType=bt_function;
        p->m_attribute.m_type.methodType.returnType.baseType=p->m_type->m_attribute.m_type.baseType;
        p->m_attribute.m_type.methodType.returnType.classID=p->m_type->m_attribute.m_type.classType.classID;
        list<Parameter_ptr>::iterator i;
        Parameter_ptr ptr;
        for(i = p->m_parameter_list->begin(); i != p->m_parameter_list->end(); ++i) {
            ptr=*i;
            ptr->accept(this);
            p->m_attribute.m_type.methodType.argsType.push_back(ptr->m_attribute.m_type.classType);
        }

        // Visit body now that the function is fully in the table. Return type checking
        p->m_methodbody->accept(this);
        if(p->m_methodbody->m_attribute.m_type.baseType!=p->m_type->m_attribute.m_type.baseType)
            t_error(ret_type_mismatch, p->m_attribute);
        else if(p->m_type->m_attribute.m_type.baseType==bt_object) {
            if(strcmp(  p->m_type->m_attribute.m_type.classType.classID,
                        p->m_methodbody->m_attribute.m_type.classType.classID)!=0)
                t_error(ret_type_mismatch, p->m_attribute);
        }

        // Lastly, close the scope
        m_symboltable->close_scope();
    }

    //=====================================================================================================================

    void visitMethodBodyImpl(MethodBodyImpl *p) {

        // Visit the children
        p->visit_children(this);

        // Set type to return type for return type checking
        p->m_attribute.m_type=p->m_return->m_attribute.m_type;
    }

    //=====================================================================================================================

    void visitParameterImpl(ParameterImpl *p) {

        // Visit the children
        p->visit_children(this);

        // Get name
        const char* name = dynamic_cast<VariableIDImpl*>(p->m_variableid)->m_symname->spelling();

        // Add to symbol table under type specifed by type
        p->m_attribute.m_type=p->m_type->m_attribute.m_type;
        p->m_attribute.m_type.classType.baseType=p->m_attribute.m_type.baseType;
        bool success = m_symboltable->insert(name, (Symbol*)&(p->m_attribute.m_type));
        if(!success) t_error(dup_ident_name, p->m_attribute); // Ensure no duplicates in scope
    }

    //=====================================================================================================================

    void visitAssignment(Assignment *p) {

        // Make sure the assigned identifier exists and is not a function symbol
        Symbol* s = m_symboltable->lookup(dynamic_cast<VariableIDImpl*>(p->m_variableid)->m_symname->spelling());
        if(s==NULL) t_error(sym_name_undef, p->m_attribute);
        if(s->baseType==bt_function) t_error(sym_type_mismatch, p->m_attribute);

        // Visit the children
        p->visit_children(this);

        // Check the symbol type to the expression type
        if(s->baseType != p->m_expression->m_attribute.m_type.baseType)
            t_error(incompat_assign, p->m_attribute);
        else if(s->baseType == bt_function) {
            if(strcmp(s->classType.classID, p->m_expression->m_attribute.m_type.classType.classID) != 0)
                t_error(incompat_assign, p->m_attribute);
        }
    }

    //=====================================================================================================================

    void visitIf(If *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Make sure the conditional predicate is a bool
        if(p->m_expression->m_attribute.m_type.baseType != bt_boolean) t_error(if_pred_err, p->m_attribute);
    }

    //=====================================================================================================================

    void visitPrint(Print *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Typechecking to find printable values (ints, bools) handled by lower levels
    }

    //=====================================================================================================================

    void visitReturnImpl(ReturnImpl *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Set type to type of expression - return type checking will be handled at function
        if(p->m_expression==NULL)
            p->m_attribute.m_type.baseType=bt_nothing;
        else p->m_attribute.m_type=p->m_expression->m_attribute.m_type;
    }

    //=====================================================================================================================

    void visitTInteger(TInteger *p) {

        // Set type
        p->m_attribute.m_type.baseType=bt_integer;
        p->m_attribute.m_type.classType.baseType=bt_integer;

        // Visit the children
        p->visit_children(this);
    }

    //=====================================================================================================================

    void visitTBoolean(TBoolean *p) {

        // Set type
        p->m_attribute.m_type.baseType=bt_boolean;
        p->m_attribute.m_type.classType.baseType=bt_boolean;

        // Visit the children
        p->visit_children(this);
    }

    //=====================================================================================================================

    void visitTNothing(TNothing *p) {

        // Set type
        p->m_attribute.m_type.baseType=bt_nothing;
        p->m_attribute.m_type.classType.baseType=bt_nothing;

        // Visit the children
        p->visit_children(this);
    }

    //=====================================================================================================================

    void visitTObject(TObject *p) {

        // Make sure class exists
        ClassNode* n = m_classtable->lookup((dynamic_cast<ClassIDImpl*>(p->m_classid)->m_classname));
        if(n==NULL) t_error(sym_name_undef, p->m_attribute);

        // Set type
        p->m_attribute.m_type.baseType=bt_object;
        p->m_attribute.m_type.classType.baseType=bt_object;
        p->m_attribute.m_type.classType.classID=n->name->spelling();

        // Visit the children
        p->visit_children(this);
    }

    //=====================================================================================================================

    void visitClassIDImpl(ClassIDImpl *p) {

        // We need to know the context before throwing undef errors. Handled one step up

        // Visit the children
        p->visit_children(this);
    }

    //=====================================================================================================================

    void visitVariableIDImpl(VariableIDImpl *p) {

        // Typechecking and looking up symbols will have to wait until we have some context

        // Visit the children
        p->visit_children(this);
    }

    //=====================================================================================================================

    void visitMethodIDImpl(MethodIDImpl *p) {

        // Can't really do anything here - we need the class name to type check

        // Visit the children
        p->visit_children(this);
    }

    //=====================================================================================================================

    void visitPlus(Plus *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Ensure right and left sides are integer types
        AllType type = p->m_expression_1->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);
        type = p->m_expression_2->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);

        // Set this type to integer
        p->m_attribute.m_type.baseType=bt_integer;
    }

    //=====================================================================================================================

    void visitMinus(Minus *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Ensure right and left sides are integer types
        AllType type = p->m_expression_1->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);
        type = p->m_expression_2->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);

        // Set this type to integer
        p->m_attribute.m_type.baseType=bt_integer;
    }

    //=====================================================================================================================

    void visitTimes(Times *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Ensure right and left sides are integer types
        AllType type = p->m_expression_1->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);
        type = p->m_expression_2->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);

        // Set this type to integer
        p->m_attribute.m_type.baseType=bt_integer;
    }

    //=====================================================================================================================

    void visitDivide(Divide *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Ensure right and left sides are integer types
        AllType type = p->m_expression_1->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);
        type = p->m_expression_2->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);

        // Set this type to integer
        p->m_attribute.m_type.baseType=bt_integer;
    }

    //=====================================================================================================================

    void visitAnd(And *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Ensure right and left sides are boolean types
        AllType type = p->m_expression_1->m_attribute.m_type;
        if(type.baseType!=bt_boolean && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_boolean))
            t_error(expr_type_err, p->m_attribute);
        type = p->m_expression_2->m_attribute.m_type;
        if(type.baseType!=bt_boolean && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_boolean))
            t_error(expr_type_err, p->m_attribute);

        // Set this type to boolean
        p->m_attribute.m_type.baseType=bt_boolean;
    }

    //=====================================================================================================================

    void visitLessThan(LessThan *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Ensure right and left sides are integer types
        AllType type = p->m_expression_1->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);
        type = p->m_expression_2->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);

        // Set this type to boolean
        p->m_attribute.m_type.baseType=bt_boolean;
    }

    //=====================================================================================================================

    void visitLessThanEqualTo(LessThanEqualTo *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Ensure right and left sides are integer types
        AllType type = p->m_expression_1->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);
        type = p->m_expression_2->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);

        // Set this type to boolean
        p->m_attribute.m_type.baseType=bt_boolean;
    }

    //=====================================================================================================================

    void visitNot(Not *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Ensure argument is of boolean type
        AllType type = p->m_expression->m_attribute.m_type;
        if(type.baseType!=bt_boolean && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_boolean))
            t_error(expr_type_err, p->m_attribute);

        // Set this type to boolean
        p->m_attribute.m_type.baseType=bt_boolean;
    }

    //=====================================================================================================================

    void visitUnaryMinus(UnaryMinus *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Ensure argument is of integer type
        AllType type = p->m_expression->m_attribute.m_type;
        if(type.baseType!=bt_integer && (type.baseType!=bt_function&&type.methodType.returnType.baseType!=bt_integer))
            t_error(expr_type_err, p->m_attribute);

        // Set this type to integer
        p->m_attribute.m_type.baseType=bt_integer;
    }

    //=====================================================================================================================

    void visitMethodCall(MethodCall *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Make sure called variable exists and is of type 'class'
        Symbol* s = m_symboltable->lookup(dynamic_cast<VariableIDImpl*>(p->m_variableid)->m_symname->spelling());
        if(s==NULL) {
            t_error(sym_name_undef, p->m_attribute);
        }
        char* const name = (char*)(s->classType.classID);
        if(s->baseType!=bt_object) t_error(sym_type_mismatch, p->m_attribute);
        else if(strcmp(s->classType.classID, name)!=0) t_error(sym_type_mismatch, p->m_attribute);

        // Grab referenced class
        ClassNode* c = m_classtable->lookup(name);
        assert(c!=NULL); // Class existence was checked in variable declaration

        // Make sure called function is a method in that class
        Symbol* func = c->scope->lookup(dynamic_cast<MethodIDImpl*>(p->m_methodid)->m_symname->spelling());
        if(func==NULL) t_error(no_class_method, p->m_attribute);
        if(func->baseType != bt_function) t_error(sym_type_mismatch, p->m_attribute);

        // Check types in parameters
        list<Expression_ptr>::iterator i;
        Expression_ptr e;
        Basetype b;
        int n;
        for(i=p->m_expression_list->begin(), n=0; i!=p->m_expression_list->end(); ++i, n++) {
            e=*i;
            if(n==func->methodType.argsType.size()) {n--; t_error(call_narg_mismatch, p->m_attribute);}
            b=func->methodType.argsType[n].baseType;
            if((e->m_attribute.m_type.baseType) != b)
                    t_error(call_args_mismatch, p->m_attribute);
        }
        if(n<func->methodType.argsType.size()) t_error(call_narg_mismatch, p->m_attribute);

        // Set type
        p->m_attribute.m_type.baseType=func->methodType.returnType.baseType;
        p->m_attribute.m_type.classType.baseType=func->methodType.returnType.baseType;
        p->m_attribute.m_type.classType.classID=func->methodType.returnType.classID;
    }

    //=====================================================================================================================

    void visitSelfCall(SelfCall *p) {

        // Visit the children first, so everything has its type
        p->visit_children(this);

        // Make sure called function is a method in this class
        Symbol* func = m_symboltable->lookup(dynamic_cast<MethodIDImpl*>(p->m_methodid)->m_symname);
        if(func==NULL) t_error(no_class_method, p->m_attribute);
        if(func->baseType != bt_function) t_error(sym_type_mismatch, p->m_attribute);

        // Check types in parameters
        list<Expression_ptr>::iterator i;
        Expression_ptr e;
        Basetype b;
        int n;
        for(i=p->m_expression_list->begin(), n=0; i!=p->m_expression_list->end(); ++i, n++) {
            e=*i;
            if(n==func->methodType.argsType.size()) {n--; t_error(call_narg_mismatch, p->m_attribute);}
            b=func->methodType.argsType[n].baseType;
            if((e->m_attribute.m_type.baseType) != (b))
                    t_error(call_args_mismatch, p->m_attribute);
        }
        if(n<func->methodType.argsType.size()) t_error(call_narg_mismatch, p->m_attribute);

        // Set Type
        p->m_attribute.m_type.baseType=func->methodType.returnType.baseType;
        p->m_attribute.m_type.classType.baseType=func->methodType.returnType.baseType;
        p->m_attribute.m_type.classType.classID=func->methodType.returnType.classID;
    }

    //=====================================================================================================================

    void visitVariable(Variable *p) {

        // Make sure VarID is in symbol table and is not a function
        Symbol *s = m_symboltable->lookup(dynamic_cast<VariableIDImpl*>(p->m_variableid)->m_symname);
        if(s==NULL) t_error(sym_name_undef, p->m_attribute);
        if(s->baseType == bt_function) t_error(sym_type_mismatch, p->m_attribute);

        // Visit the children
        p->visit_children(this);

        // Set type to type of looked up symbol
        p->m_attribute.m_type=(AllType)*s;
    }

    //=====================================================================================================================

    void visitIntegerLiteral(IntegerLiteral *p) {

        // Set type in attribute
        p->m_attribute.m_type.baseType=bt_integer;

        // Visit children
        p->visit_children(this);
    }

    //=====================================================================================================================

    void visitBooleanLiteral(BooleanLiteral *p) {

        // Set type in attribute
        p->m_attribute.m_type.baseType=bt_boolean;

        // Visit children
        p->visit_children(this);
    }

    //=====================================================================================================================

    void visitNothing(Nothing *p) {

        // Set type in attribute
        p->m_attribute.m_type.baseType=bt_nothing;

        // Visit children
        p->visit_children(this);
    }

    //=====================================================================================================================

    void visitSymName(SymName *p) {

    }

    //=====================================================================================================================

    void visitPrimitive(Primitive *p) {

    }

    //=====================================================================================================================

    void visitClassName(ClassName *p) {

    }


    void visitNullPointer() {}
};
