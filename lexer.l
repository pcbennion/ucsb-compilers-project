%option yylineno
%pointer

%{
    #include <stdlib.h>
    #include <string.h>
    #include "ast.hpp"
    #include "primitive.hpp"
    #include "symtab.hpp"
    #include "classhierarchy.hpp"
    #include "parser.hpp"
    
    void yyerror(const char *);
%}

/* WRITEME: Put your definitions here, if you have any */
%%

"/*"([^"*"]|[\r\n]|("*"+([^"*/"]|[\r\n])))*"*"+"/"     ; /* I now hate multiline comments */

[\+\-\.\{\}\;\(\)\:\=\/\*\<\>\,] { return *yytext; }

"<="                  {return LTE;}
"Nothing"             {return VOID;}
"Int"                 {return INTTYPE;}
"Bool"                {return BOOLTYPE;}
"return"              {return RETURN;}
"from"                {return EXTEND;}
"true"/[ \t\n]*[\,\;]? {return TRUETOK;}
"false"/[ \t\n]*[\,\;]? {return FALSETOK;}
"print"               {return PRINT;}
"if"                  {return IFTOK;}
"not"                 {return NOTTOK;}
"then"                {return THENTOK;}
"and"                 {return ANDTOK;}
"or"                  {return ORTOK;}

[A-Z][A-Za-z0-9_]*    {yylval.u_base_charptr = strdup(yytext); return CLASSID;}
[a-z_][A-Za-z0-9_]*/[ \t\n]*[\:\,]   {yylval.u_base_charptr = strdup(yytext); return VARID;}
[a-z_][A-Za-z0-9_]*/[ \t\n]*"("   {yylval.u_base_charptr = strdup(yytext); return METHODID;}
[a-z_][A-Za-z0-9_]*   {yylval.u_base_charptr = strdup(yytext); return IDENTIFIER;}
0|([1-9][0-9]*)       {yylval.u_base_int = atoi(yytext); return NUMBER;}

[ \t\n]        ; /* skip whitespace*/

.              yyerror((char *) "invalid character");

%%

int yywrap(void) {
    return 1;
}
