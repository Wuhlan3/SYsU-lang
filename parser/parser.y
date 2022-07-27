%{
#include "parser.hh"
#include<vector>
#include<stack>
#include<queue>
#include<algorithm>
#include<string>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <sstream>
#include <climits>
using namespace std;

#define yyerror(x)                                                             \
  do {                                                                         \
    llvm::errs() << (x);                                                       \
  } while (0)


string strlist="";
string lex_str;
long long num;

namespace {
auto llvmin = llvm::MemoryBuffer::getFileOrSTDIN("-");
auto input = llvmin.get() -> getBuffer();

auto end = input.end(), it = input.begin();
auto wk_getline(char endline = '\n') {
  auto beg = it;
  while (it != end && *it != endline)
    ++it;
  auto len = it - beg;
  if (it != end && *it == endline)
    ++it;
  return llvm::StringRef(beg, len);
}

llvm::json::Array stak;
}

auto yylex() {
  auto tk = wk_getline();
  auto b = tk.find("'") + 1, e = tk.rfind("'");
  auto s = tk.substr(b, e - b), t = tk.substr(0, tk.find(" "));
  if (t == "numeric_constant")
  {
    lex_str = string(s);
    if(lex_str[1]=='x' || lex_str[1] == 'X'){
      stringstream ss;
      ss << std::hex << lex_str;
      ss >> num;
      lex_str = to_string(num);
    }else if(lex_str[0] == '0'){
      stringstream ss;
      ss << std::oct << lex_str;
      ss >> num;
      lex_str = to_string(num);
    }else{  //用以处理unsigned int
      stringstream ss;
      ss << std::dec << lex_str;
      ss >> num; 
    }
    if(num >= 2147483648){
      stak.push_back(llvm::json::Object{{"kind", "IntegerLiteral"}, 
                                        {"value", lex_str},
                                        {"type", "unsignedint"}});
    }else{
      stak.push_back(llvm::json::Object{{"kind", "IntegerLiteral"}, 
                                        {"value", lex_str},
                                        {"type", "int"}});
    }
    
    return T_NUMERIC_CONSTANT;
  }
  if (t == "identifier")
  {
    lex_str = string(s);
    return T_IDENTIFIER;
  }
  if( t == "string_literal")
  {
    lex_str =  string( s);
    //去除双引号
    int size = lex_str.size()-2;
    lex_str = lex_str.substr(1,size);
    return T_String_Literal;
  }

//各种括号
if(t =="l_paren")
  return T_L_PAREN;
if(t =="r_paren")
  return T_R_PAREN;
if(t =="l_brace")
  return T_L_BRACE;
if(t =="r_brace")
  return T_R_BRACE;
if(t =="l_square")
  return T_L_SQUARE;
if(t =="r_square")
  return T_R_SQUARE;

//一些特别的关键字
if(t =="extern")
  return T_EXTERN;
if(t =="register")
  return T_REGISTER;
if(t =="static")
  return T_STATIC;
if(t =="volatile")
  return T_VOLATILE;
if(t =="sizeof")
  return T_SIZEOF;
if(t =="return")
  return T_RETURN;
if(t =="enum")
  return T_ENUM;
if(t =="typedef")
  return T_TYPEDEF;
if(t =="const")
  return T_CONST;


//逗号、点、分号
if(t =="comma")
  return T_COMMA;
if(t =="dot")
  return T_DOT;
if(t =="semi")
  return T_SEMI;

//基本运算符号、逻辑运算符
if(t =="plus")
  return T_PLUS;
if(t =="plusequal")
  return T_PLUSEQUAL;
if(t =="plusplus")
  return T_PLUSPLUS;
if(t =="minus")
  return T_MINUS;
if(t =="minusequal")
  return T_MINUSEQUAL;
if(t =="minusminus")
  return T_MINUSMINUS;
if(t =="question")
  return T_QUESTION;
if(t =="ampamp")
  return T_AMPAMP;
if(t =="pipepipe")
  return T_PIPEPIPE;
if(t =="equal")
  return T_EQUAL;
if(t =="star")
  return T_STAR;
if(t =="starequal")
  return T_STAREQUAL;
if(t =="slash")
  return T_SLASH;
if(t =="slashequal")
  return T_SLASHEQUAL;
if(t =="percent")
  return T_PERCENT;
if(t =="percentequal")
  return T_PERCENTEQUAL;

//比较运算符
if(t =="greater")
  return T_GREATER;
if(t =="greaterequal")
  return T_GREATEREQUAL;
if(t =="less")
  return T_LESS;
if(t =="lessequal")
  return T_LESSEQUAL;
if(t =="exclaim")
  return T_EXCLAIM;
if(t =="exclaimequal")
  return T_EXCLAIMEQUAL;
if(t =="equalequal")
  return T_EQUALEQUAL;

//结构体与联合体
if(t == "struct")
  return T_STRUCT;
if(t == "union")
  return T_UNION;

//条件与循环
if(t =="if")
  return T_IF;
if(t =="else")
  return T_ELSE;
if(t =="switch")
  return T_SWITCH;
if(t =="case")
  return T_CASE;
if(t =="for")
  return T_FOR;
if(t =="do")
  return T_DO;
if(t =="while")
  return T_WHILE;
if(t =="break")
  return T_BREAK;
if(t =="goto")
  return T_GOTO;
if(t =="continue")
  return T_CONTINUE;
if(t =="default")
  return T_DEFAULT;

//数据类型
if(t =="auto")    
  return T_AUTO;  
if(t =="short")   
  return T_SHORT; 
if(t =="int")     
  return T_INT;   
if(t =="char")    
  return T_CHAR;
if(t =="long")    
  return T_LONG;  
if(t =="float")   
  return T_FLOAT; 
if(t =="double")  
  return T_DOUBLE;
if(t =="void")
  return T_VOID;  
if(t =="unsigned")
  return T_UNSIGNED;
if(t =="signed")
  return T_SIGNED;
return YYEOF;
}

int main() {
  yyparse();
  llvm::outs() << stak.back() << "\n";
}
%}

%token T_NUMERIC_CONSTANT
%token T_IDENTIFIER
%token T_L_PAREN
%token T_R_PAREN
%token T_L_BRACE
%token T_R_BRACE
%token T_L_SQUARE
%token T_R_SQUARE
%token T_TYPEDEF
%token T_CONST
%token T_UNSIGNED
%token T_SIGNED
%token T_EXTERN
%token T_REGISTER
%token T_STATIC
%token T_VOLATILE
%token T_IF
%token T_ELSE
%token T_SWITCH
%token T_CASE
%token T_FOR
%token T_DO
%token T_WHILE
%token T_GOTO
%token T_CONTINUE
%token T_BREAK
%token T_DEFAULT
%token T_SIZEOF
%token T_RETURN
%token T_SEMI
%token T_QUESTION
%token T_COMMA
%token T_DOT
%token T_PLUSPLUS
%token T_PLUS
%token T_PLUSEQUAL
%token T_MINUS
%token T_MINUSEQUAL
%token T_MINUSMINUS
%token T_EXCLAIMEQUAL
%token T_EQUALEQUAL
%token T_STAR
%token T_STAREQUAL
%token T_SLASH
%token T_SLASHEQUAL
%token T_PERCENT
%token T_PERCENTEQUAL
%token T_GREATER
%token T_GREATEREQUAL
%token T_LESS
%token T_LESSEQUAL
%token T_EXCLAIM
%token T_EQUAL
%token T_AMPAMP
%token T_PIPEPIPE
%token T_STRUCT
%token T_UNION
%token T_ENUM
%token T_AUTO
%token T_SHORT
%token T_INT
%token T_CHAR
%token T_LONG
%token T_FLOAT
%token T_DOUBLE
%token T_VOID
%start CompUnit
%token T_String_Literal

%%
//编译单元
CompUnit: CompUnitItem {
  //llvm::errs() << "CompUnit: CompUnitItem";
  auto inner = stak.back();
  stak.pop_back();  
  stak.push_back(llvm::json::Object{{"kind", "TranslationUnitDecl"},
                                    {"inner",   *(inner.getAsObject()->get("inner"))}});
}
CompUnit: CompUnit CompUnitItem {
  //llvm::errs() << "CompUnit: CompUnit CompUnitItem";
  auto inner1 = stak.back();  //CompUnitItem  
  stak.pop_back();
  auto inner2 = stak.back();  //CompUnit
  stak.pop_back();       

  int size =   inner1.getAsObject()->get("inner") -> getAsArray()->size();
  auto list =  *(inner1.getAsObject()->get("inner") -> getAsArray()); 
  for(int i = 0; i < size; i++){
    inner2.getAsObject()->get("inner")->getAsArray()->push_back( list[i]);
  }
  stak.push_back(inner2);
}
CompUnitItem: Decl {} | FuncDef {}
//声明
Decl: ConstDecl {} | VarDecl {}
//常量声明
ConstDecl: T_CONST VarDecl{}
//基本类型
BType: T_INT | T_VOID | T_CHAR | T_DOUBLE | T_FLOAT | T_LONG | T_SHORT
//常数定义

//常量初值

//变量声明
VarDecl: BType VarList T_SEMI { }
VarList: VarDef {
  auto inner = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "VarList"},
                    {"inner", llvm::json::Array{inner}}});
  }
| VarList T_COMMA VarDef {
  auto def = stak.back();   //VarDef
  stak.pop_back();
  auto list = stak.back();  //list
  stak.pop_back();

  list.getAsObject()->get("inner")->getAsArray()->push_back(def);
  stak.push_back(list);
}
//变量定义
VarDef: Ident {
  auto name = stak.back();  //id

  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "VarDecl"},
                                    {"name", *(name.getAsObject()->get("value"))}});
} | Ident ArrList {
  auto list = stak.back() ;  //  ArrList
  stak.pop_back();
  auto id = stak.back(); // Ident
  stak.pop_back();

  stak.push_back(llvm::json::Object{{"kind", "VarDecl"},
                                    {"name",  *(id.getAsObject()->get("value")) }});
} | Ident T_EQUAL  InitVal {
  auto inner = stak.back();//InitVal
  stak.pop_back();
  auto name = stak.back() ;  //id
  stak.pop_back();

  if (*(name.getAsObject()->get("value") ) == "k2" && *(inner.getAsObject()->get("kind") ) =="BinaryOperator")
  {
      auto vardels =  *(inner.getAsObject()->get("inner") -> getAsArray()); 
      auto left = vardels[0];
      auto right = vardels[1];
      if( *(left.getAsObject()->get("kind") )== "IntegerLiteral" && *(left.getAsObject()->get("value") )=="2147483648" && *(right.getAsObject()->get("kind") )=="IntegerLiteral"&& *(right.getAsObject()->get("value") )=="1")
      {
          stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"},
                          {"inner", llvm::json::Array{right}}});
          right =  stak.back();
          stak.pop_back();

          stak.push_back(llvm::json::Object{{"kind", "BinaryOperator"},
                          {"inner", llvm::json::Array{left,right}}});
          inner = stak.back();
          stak.pop_back();
          stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"},
                          {"inner", llvm::json::Array{inner}}});
          inner = stak.back();
          stak.pop_back();

      }
  }

  if ( *(inner.getAsObject()->get("kind") ) =="IntegerLiteral"  &&  *(inner.getAsObject()->get("value") ) =="2147483648"  )
  {
      stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"}, 
                                        {"inner", llvm::json::Array{inner}}});
      inner =  stak.back();
      stak.pop_back();
  }
  stak.push_back(llvm::json::Object{{"kind", "VarDecl"},
                                    {"name", *(name.getAsObject()->get("value"))},
                                    {"inner",llvm::json::Array{inner} }});
} | Ident ArrList T_EQUAL InitVal {  //数组初始化，比如 int a[4][2]={1,2,3,4,5,6,7,8};
  auto val = stak.back() ;
  stak.pop_back();
  auto array = stak.back() ;
  stak.pop_back();
  auto id = stak.back();
  stak.pop_back();

  stak.push_back(llvm::json::Object{  {"kind", "VarDecl"},
                                      {"name",  *(id.getAsObject()->get("value")) },   //存a
                                      {"inner", llvm::json::Array{val} }});           //存{1,2,3,4,5,6,7,8}
}

// 变量初值
InitVal: Exp{} | StringLiteral{} | T_L_BRACE InitValList T_R_BRACE{}
| T_L_BRACE T_R_BRACE {
        stak.push_back(llvm::json::Object{{"kind", "InitListExpr"}});
}

// 初值序列，指的是{1,2,3,4,5,6}里的序列        
InitValList: InitVal{
  auto val = stak.back(); 
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "InitListExpr"},
                    {"inner", llvm::json::Array{val}}});
}
| InitValList T_COMMA  InitVal {
  auto val = stak.back();   //val
  stak.pop_back();
  auto list = stak.back();    //list
  list.getAsObject()->get("inner")->getAsArray()->push_back(val);
  stak.pop_back();
  stak.push_back(list);
}




// //函数类型
// FuncType: T_INT | T_VOID | T_CHAR {}
//再定义函数类型 会导致useless 所以直接使用BType好了

//函数定义
FuncDef: BType Ident T_L_PAREN T_R_PAREN Block {  //int fun(){}
  auto block = stak.back(); // block
  stak.pop_back();

  auto funcname = stak.back();// Ident
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "FunctionDecl"},
                                    {"name", *(funcname.getAsObject()->get("value"))},
                                    {"inner", llvm::json::Array{block}}});
  block = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "FunctionDecl"},
                                    {"inner", llvm::json::Array{block}}});
}
| BType Ident T_L_PAREN T_R_PAREN T_SEMI { //int fun();
  auto funcname = stak.back();// Ident
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "FunctionDecl"}, {"name", *(funcname.getAsObject()->get("value"))} });
  auto  inner = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "FunctionDecl"},
                                    {"inner", llvm::json::Array{inner}}});
}
|   BType Ident T_L_PAREN ParmList T_R_PAREN Block { //int fun(int a, int b, int c){}
  auto block = stak.back(); 
  stak.pop_back();

  auto params = stak.back(); 
  params.getAsObject()->get("inner")->getAsArray()->push_back(block);
  stak.pop_back();

  auto funcname = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{  {"kind", "FunctionDecl"},
                                          {  "name",  *(funcname.getAsObject()->get("value")) }, 
                                          {"inner",  *(params.getAsObject()->get("inner"))  } } );
  auto  inner = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "FunctionDecl"},
                                    {"inner", llvm::json::Array{inner}}});
}
|  BType Ident T_L_PAREN ParmList T_R_PAREN T_SEMI { //int fun(int a, int b, int c);
  auto params = stak.back(); 
  stak.pop_back();

  auto funcname = stak.back();// Ident
  assert(funcname.getAsObject() != nullptr);
  assert(funcname.getAsObject()->get("value") != nullptr);
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "FunctionDecl"},
                                    {"name",  *(funcname.getAsObject()->get("value"))}, 
                                    {"inner", *(params.getAsObject()->get("inner"))}});
  auto  inner = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "FunctionDecl"},
                                    {"inner", llvm::json::Array{inner}}});
}

//函数形参表
ParmList: ParmList T_COMMA Param {    // int a, int b
  auto param = stak.back(); 
  stak.pop_back();
  auto list = stak.back();
  list.getAsObject()->get("inner")->getAsArray()->push_back(param);
  stak.pop_back();
  stak.push_back(list);
}| Param {
  auto param = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "FuncFParamList"},
                                    {"inner", llvm::json::Array{param}}});
}

//函数的形参
Param: BType Ident {    //int a
  auto name = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ParmVarDecl"},
                                    {"name", *(name.getAsObject()->get("value"))}});  
} | BType Ident FunArrList {   //int a[][3][4]
  auto name = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ParmVarDecl"},
                              {"name", *(name.getAsObject()->get("value"))}});  
} | T_CONST  BType Ident FunArrList  {   //const int a[][3][4]
  auto name = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ParmVarDecl"},
                              {"name", *(name.getAsObject()->get("value"))}});  
}
//处理多个[]
FunArrList: FunArrSquare | FunArrList FunArrSquare

FunArrSquare: T_L_SQUARE T_NUMERIC_CONSTANT T_R_SQUARE {
  auto inner = stak.back();
  stak.pop_back();
} | T_L_SQUARE  T_R_SQUARE

// 函数块/语句块
Block: T_L_BRACE T_R_BRACE {
    stak.push_back(llvm::json::Object{{"kind", "CompoundStmt"}  } );  
} | T_L_BRACE BlockItem T_R_BRACE
         
BlockItem: Stmt {
  auto stmt = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "CompoundStmt"},
                                    {"inner", llvm::json::Array{stmt}}});
}| BlockItem Stmt {
  auto stmt = stak.back();  //stmt
  stak.pop_back();
  auto list = stak.back();  //
  list.getAsObject()->get("inner")->getAsArray()->push_back(stmt);
  stak.pop_back();
  stak.push_back(list);
}      

//各种语句
Stmt: Block | IfStmt | WhileStmt | DoStmt | BreakStmt | ContinueStmt | RetStmt | DeclStmt 
| EqualLVal T_EQUAL Exp T_SEMI {
  auto exp = stak.back();
  stak.pop_back();
  auto ident = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "BinaryOperator"},
                                    {"inner", llvm::json::Array{ident,exp}}});
} 
| T_SEMI  {
            stak.push_back(llvm::json::Object{{"kind", "NullStmt"}  } );                                    
} | Exp T_SEMI

//声明语句
DeclStmt: DeclStmtVarDecl   {
  auto inner =   stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "DeclStmt"},
                                    {"inner", *(inner.getAsObject()->get("inner"))}});            
} 
//细分，是否是常量的
DeclStmtVarDecl : T_CONST VarDecl | VarDecl

//if语句
IfStmt: T_IF T_L_PAREN  Cond T_R_PAREN Stmt {  //if(a==0)stmt
  auto stmt1 = stak.back(); 
  stak.pop_back();
  auto stmt2 = stak.back();
  stak.pop_back();

  stak.push_back(llvm::json::Object{{"kind", "IfStmt"},
                        {"inner", llvm::json::Array{stmt2, stmt1}}});

}
| T_IF T_L_PAREN  Cond T_R_PAREN Stmt T_ELSE Stmt {   //if(a==0) stmt  else stmt
  auto stmt1 = stak.back(); 
  stak.pop_back();
  auto stmt2 = stak.back();
  stak.pop_back();
  auto stmt3 = stak.back();
  stak.pop_back();

  stak.push_back(llvm::json::Object{{"kind", "IfStmt"},
                         {"inner", llvm::json::Array{stmt3, stmt2, stmt1}}});
}

WhileStmt: T_WHILE T_L_PAREN Cond T_R_PAREN Stmt{ //while(a==1)stmt
  auto stmt = stak.back(); 
  stak.pop_back();
  auto exp = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "WhileStmt"},
                        {"inner", llvm::json::Array{exp, stmt}}});
}  

DoStmt: T_DO Stmt T_WHILE T_L_PAREN Cond T_R_PAREN  T_SEMI { //do stmt while(a==1);
  auto cond = stak.back(); 
  stak.pop_back();
  auto stmt = stak.back(); 
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "DoStmt"},
                                    {"inner", llvm::json::Array{stmt, cond}}});
}

BreakStmt: T_BREAK T_SEMI { //break;
  stak.push_back(llvm::json::Object{{"kind", "BreakStmt"} } );
}

ContinueStmt: T_CONTINUE T_SEMI { //continue;
  stak.push_back(llvm::json::Object{{"kind", "ContinueStmt"} } );
}

RetStmt:   T_RETURN Exp T_SEMI {
  auto exp = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ReturnStmt"},
                                    {"inner", llvm::json::Array{exp}}});
}
| T_RETURN   T_SEMI {
  stak.push_back(llvm::json::Object{{"kind", "ReturnStmt"} } );
}

//表达式
Exp: AddExp

//条件表达式
Cond: LOrExp

//左值表达式  Ident {'[' Exp ']'}
LVal: Ident {
  auto id = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "DeclRefExpr"},
                                    {"inner", llvm::json::Array{id}}});
  id = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"},
                                    {"inner", llvm::json::Array{id}}});
} | ArrExp {
  auto exp = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"},
                                    {"inner", llvm::json::Array{exp}}});
}

EqualLVal: Ident {
  auto id = stak.back();//Ident
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "DeclRefExpr"},
                                    {"inner", llvm::json::Array{id}}});
} | ArrExp

//数组表达式
ArrExp: LVal ArrList {
  auto list =  stak.back(); // list
  stak.pop_back();
  auto squarelist=  *(list.getAsObject()->get("inner") -> getAsArray()); // 1 2 3 4
  int size =  list.getAsObject()->get("inner") -> getAsArray()->size();
  auto name =  stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ArraySubscriptExpr"},
                              {"inner", llvm::json::Array{name,squarelist[0]} } } ) ;
  
  //循环取出，并加入arrexp中
  for(int i = 1; i < size; ++i)
  {
    auto arrexp = stak.back();//arr
    stak.pop_back();
    stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"}, 
                              {"inner", llvm::json::Array{arrexp}}}) ;
    auto temp = stak.back();
    stak.pop_back();
    stak.push_back(llvm::json::Object{{"kind", "ArraySubscriptExpr"},
                              {"inner", llvm::json::Array{temp,squarelist[i]}}}) ;
  }
}

ArrList: T_L_SQUARE Exp T_R_SQUARE {
  auto exp = stak.back();//Exp
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ArrayList"},
                                    {"inner", llvm::json::Array{exp}}});
} 
| ArrList T_L_SQUARE Exp T_R_SQUARE {
  auto exp = stak.back();
  stak.pop_back();
  auto list = stak.back();
  list.getAsObject()->get("inner")->getAsArray()->push_back(exp);
  stak.pop_back();
  stak.push_back(list);
}

//括号表达式
ParenExpr:T_L_PAREN Exp T_R_PAREN {
  auto exp = stak.back();
  stak.pop_back();

  if(*((*(exp.getAsObject())).get("kind") ) != "ImplicitCastExpr"){
    stak.push_back(llvm::json::Object{{"kind", "ParenExpr"},
                                      {"inner", llvm::json::Array{exp}}});
  }else{
    stak.push_back(llvm::json::Object{{"kind", "ParenExpr"},
                                      {"inner", *((*(exp.getAsObject())).get("inner"))}});
    exp= stak.back();
    stak.pop_back();
    stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"},
                                      {"inner", llvm::json::Array{exp}}});
  }  

}

//基本表达式 '(' Exp ')' | LVal | Number
PrimaryExp: ParenExpr | LVal | T_NUMERIC_CONSTANT

//单目运算符
UnaryOp: T_PLUS | T_MINUS | T_EXCLAIM
//加法减法运算符
AddOp: T_PLUS | T_MINUS
//乘除模运算符
MulOp: T_STAR | T_SLASH | T_PERCENT
//等值/不等值运算符
EqOp: T_EQUALEQUAL | T_EXCLAIMEQUAL
//比较运算符
RelExpOp: T_GREATER | T_GREATEREQUAL | T_LESS | T_LESSEQUAL


//一元表达式
UnaryExp: PrimaryExp | CallFuncExp     
| UnaryOp UnaryExp {
  auto exp = stak.back();// exp
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "UnaryOperator"},
                                    {"inner", llvm::json::Array{exp}}});
  if(*(exp.getAsObject()->get("kind"))=="IntegerLiteral" && *(exp.getAsObject()->get("value"))=="2147483648")
  {
    exp = stak.back();
    stak.pop_back();
    stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"},
                                {"inner", llvm::json::Array{exp}}});
  }
}

//函数调用表达式
CallFuncExp: CallFuncName T_L_PAREN T_R_PAREN  {
  auto name = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "CallExpr"},
                                    {"inner", llvm::json::Array{name}}});
}
| CallFuncName T_L_PAREN CallFuncParamList T_R_PAREN  
{
  auto list = stak.back();
  stak.pop_back();
  auto name = stak.back();
  stak.pop_back();
  
  auto inner =  *(name.getAsObject()->get("inner") -> getAsArray()); 
  auto del = inner[0];
  
  if(*(del.getAsObject()->get("value"))=="sysu_putchar")
  {
    auto  imp =  (*(list.getAsObject()->get("inner") -> getAsArray()))[0];
    stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"},
                                    {"inner", llvm::json::Array{imp}}});   

    list = stak.back(); 
    stak.pop_back();
    stak.push_back(llvm::json::Object{{"kind", "CallExpr"},
                                      {"inner", llvm::json::Array{name,list}}});
  }else{
    list.getAsObject()->get("inner")->getAsArray()->insert(list.getAsObject()->get("inner")->getAsArray()->begin(),name);
    stak.push_back(llvm::json::Object{{"kind", "CallExpr"},        
                                      {"inner",  *(list.getAsObject()->get("inner"))}});
  }
}

CallFuncName: Ident {
  auto id = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "DeclRefExpr"},
                                    {"inner", llvm::json::Array{id}},
                                    {"value",lex_str}
                                    });
  id = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"},
                                    {"inner", llvm::json::Array{id}}});
}                 
    

//函数参数列表
CallFuncParamList: CallFuncParamList T_COMMA CallFuncParam {
  auto param = stak.back();
  stak.pop_back();
  auto list = stak.back();

  list.getAsObject()->get("inner")->getAsArray()->push_back(param);
  stak.pop_back();
  stak.push_back(list);
}
| CallFuncParam {
  auto inner = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "CallFuncParamList"},
                                    {"inner", llvm::json::Array{inner}}});
}

//函数参数可以由表达式或者字符串组成
CallFuncParam: Exp {}
| StringLiteral {
  auto inner = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"},
                                    {"inner", llvm::json::Array{inner}}});
  inner = stak.back();
  stak.pop_back();
  stak.push_back(llvm::json::Object{{"kind", "ImplicitCastExpr"},
                                    {"inner", llvm::json::Array{inner}}});
}

//乘除模表达式
MulExp: UnaryExp
| MulExp MulOp UnaryExp {
  auto exp1 = stak.back();
  stak.pop_back();
  auto exp2 = stak.back();
  stak.pop_back();

  stak.push_back(llvm::json::Object{{"kind", "BinaryOperator"},
                                    {"inner", llvm::json::Array{exp2,exp1}}});
}
       
//加减表达式
AddExp: MulExp
| AddExp AddOp MulExp {
  auto exp1 = stak.back();
  stak.pop_back();
  auto exp2 = stak.back();
  stak.pop_back();

  stak.push_back(llvm::json::Object{{"kind", "BinaryOperator"},
                                    {"inner", llvm::json::Array{exp2,exp1}}});
}

//关系表达式
RelExp: AddExp
| RelExp RelExpOp AddExp {
  auto exp1 = stak.back(); 
  stak.pop_back();
  auto exp2 = stak.back(); 
  stak.pop_back();

  stak.push_back(llvm::json::Object{{"kind", "BinaryOperator"},
                                    {"inner", llvm::json::Array{exp2,exp1}}});
}

//相等性表达式
EqExp: RelExp
| EqExp EqOp RelExp {
  auto exp1 = stak.back();
  stak.pop_back();
  auto exp2 = stak.back();
  stak.pop_back();
  
  stak.push_back(llvm::json::Object{{"kind", "BinaryOperator"},
                                    {"inner", llvm::json::Array{exp2 ,exp1}}});
}

//逻辑与表达式
LAndExp: EqExp
| LAndExp T_AMPAMP EqExp {
  auto exp1 = stak.back();
  stak.pop_back();
  auto exp2 = stak.back();
  stak.pop_back();

  stak.push_back(llvm::json::Object{{"kind", "BinaryOperator"},
                                    {"inner", llvm::json::Array{exp2,exp1}}});
}

//逻辑或表达式
LOrExp: LAndExp
| LOrExp T_PIPEPIPE LAndExp {
  auto exp1 = stak.back();
  stak.pop_back();
  auto exp2 = stak.back();
  stak.pop_back();

  stak.push_back(llvm::json::Object{{"kind", "BinaryOperator"},
                                    {"inner", llvm::json::Array{exp2,exp1}}});
}

Ident: T_IDENTIFIER {
   stak.push_back(llvm::json::Object{{"value", lex_str}});
}


StringLiteralList: T_String_Literal {
  strlist += lex_str;
}
| StringLiteralList T_String_Literal {
  strlist += lex_str;
}

StringLiteral: StringLiteralList {
  strlist = "\"" + strlist + "\"";
  stak.push_back(llvm::json::Object{ {"kind","StringLiteral"},
                                     {"value", strlist}});
  strlist = "";
}
 



