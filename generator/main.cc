#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Alignment.h>
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <map>
#include <iostream>
#include <algorithm>
using namespace std;



namespace {
llvm::LLVMContext TheContext; //用于保存全局的状态，在多线程执行的时候，可以每个线程一个LLVMContext，避免竞争
                              //一个不透明对象，拥有许多核心的LLVM数据结构，不需了解
llvm::Module TheModule("-", TheContext); //LLVM IR程序的顶层结构，它将拥有我们生成的所有IR内存
llvm::IRBuilder<> Builder(TheContext);  //定义全局的Builder

map<string, pair<llvm::Value*,bool>> VarEnv;  //局部变量符号表，bool类型指的是是不是函数参数
//**********用于break和continue的跳转
 
 
stack<llvm::BasicBlock*>  BreakBB;
stack<llvm::BasicBlock*>  ContinueBB;

// llvm::BasicBlock* ShortCircuitElseBB;  
// llvm::BasicBlock* ShortCircuitThenBB; 
// llvm::BasicBlock* ShortCircuitIfContBB;

//用于短路处理
stack<llvm::BasicBlock*> ShortCircuitThen;
stack<llvm::BasicBlock*> ShortCircuitSkip;


bool ShortCircuitHasElse = false;
// bool isBreakOrRet = false;
//***********************************


//函数声明(因为可能出现函数之间相互调用)**********************************************
llvm::Value* BuildImplicitCastExpr(const llvm::json::Object *O);

llvm::Value* BuildFuncParmsImplicitCastExpr(const llvm::json::Object *O);


void BuildBreakStmt(const llvm::json::Object *O);

void BuildInitListExpr(const llvm::json::Object *O, llvm::Value* array, llvm::Type* type, vector<llvm::Value*> idxList);

void BuildCompoundStmt(const llvm::json::Object *O);

void BuildContinueStmt(const llvm::json::Object *O);

void BuildArrList( const llvm::json ::Object*O, vector<llvm::Value*> &idxList);

string ParseName(const llvm::json::Object *O);

string ParseArrName(const llvm::json::Object *O);

string ParseArrId(const llvm::json::Object *O);

string ParseId(const llvm::json::Object *O);

llvm::Value * BuildArrGEP( const llvm::json:: Object*O);

llvm:: Value * BuildLocalVar( string &name);

llvm::Value* BuildBinaryExpr(const llvm::json::Object *O);

llvm::Value* BuildUnaryExpr(const llvm::json::Object *O);

llvm::Value * BuildAny(const llvm::json::Object *O);

llvm:: Value* BuildParenExpr(const llvm::json::Object *O);

llvm::Value* BuildStringLiteral(const llvm::json::Object *O, string name);
//********************************************************************


llvm::APInt BuildIntegerLiteral(const llvm::json::Object *O){
  //int类型
  if(O->getString("kind")->str() == "IntegerLiteral"){
    string value = O->getString("value")->str();
    return llvm::APInt(32, value, 10);
  }
  return llvm::APInt(32, "0", 10);
}

llvm::Value * BuildDeclRefExpr(const llvm::json::Object *O){  
  auto referencedDecl = O->getObject("referencedDecl");
  string name = referencedDecl-> getString("name")->str();
  string kind = referencedDecl->getString("kind")->str();
  string id = referencedDecl->getString("id")->str();
  //函数声明
  if(kind == "FunctionDecl"){
    llvm::Function *func = TheModule.getFunction(name);
    return func;
  }else if(kind == "VarDecl" || kind == "ParmVarDecl"){ 
    if(auto ptr = BuildLocalVar(id)){ //局部变量
      // return Builder.CreateLoad(ptr); 
      return ptr;
    }else{  //全局变量
      llvm::GlobalVariable *globalVar = TheModule.getNamedGlobal(name); 
      // return Builder.CreateLoad(globalVar);
      return globalVar;
    }
  }
  
  return nullptr;
  
}

llvm::Value * BuildFuncParmsDeclRefExpr(const llvm::json::Object *O){
  
  if(O == nullptr)return nullptr;
  assert(O->getString("kind")->str() == "DeclRefExpr");
  
  auto referencedDecl = O->getObject("referencedDecl");
  string name = referencedDecl->getString("name")->str();
  string kind = referencedDecl->getString("kind")->str();
  string id = referencedDecl->getString("id")->str();
  if(kind == "VarDecl"){
    if(auto ptr = BuildLocalVar(id)){
      return ptr;
    }else{ //查找全局变量，
      llvm::GlobalVariable *globalVar = TheModule.getNamedGlobal(name);
      auto retval = Builder.CreateLoad(globalVar, name);
      return retval;
    }
  }
  return nullptr;
}

llvm::Value* i32Toi1(llvm::Value* val){
  if(val->getType()==llvm::Type::getInt32Ty(TheContext) ){
    val = Builder.CreateICmpNE(val, llvm::ConstantInt::get(TheContext,llvm::APInt(32,  "0",10)));
  }
  return val;
}

llvm::Value* i1Toi32(llvm::Value* val){
  if(val->getType()==llvm::Type::getInt1Ty(TheContext) ){
    val = Builder.CreateZExt(val, llvm::Type::getInt32Ty(TheContext)); 
  }
  return val;
}

llvm::Value* i8Toi32(llvm::Value* val){
  if(val->getType()==llvm::Type::getInt8Ty(TheContext) ){
    val = Builder.CreateZExt(val, llvm::Type::getInt32Ty(TheContext)); 
  }
  return val;
}

llvm::Value* i8Toi64(llvm::Value* val){
  if(val->getType()==llvm::Type::getInt8Ty(TheContext) ){
    val = Builder.CreateZExt(val, llvm::Type::getInt64Ty(TheContext)); 
  }
  return val;
}

llvm::Value* i32Toi64(llvm::Value* val){
  if(val->getType()==llvm::Type::getInt32Ty(TheContext) ){
    val = Builder.CreateZExt(val, llvm::Type::getInt64Ty(TheContext)); 
  }
  return val;
}

llvm::Value* i64Toi32(llvm::Value* val){
  if(val->getType()==llvm::Type::getInt64Ty(TheContext) ){
    val = Builder.CreateTrunc(val, llvm::Type::getInt32Ty(TheContext)); 
  }
  return val;
}

//根据qualType解析数据类型
llvm::Type * ParseType(const string &qualType, bool& isArray, bool& isConst){
    int size = qualType.size();
    isArray = false;
    llvm::Type * type;
    
    //不是数组
    if(qualType == "int"){
      return llvm::Type::getInt32Ty(TheContext);
    }else if(qualType == "const int"){
      isConst = true;
      return llvm::Type::getInt32Ty(TheContext);
    }else if(qualType == "void"){
      return llvm::Type::getVoidTy(TheContext);
    }else if(qualType == "long long"){
      return llvm::Type::getInt64Ty(TheContext);
    }else if(qualType == "const long long"){
      isConst = true;
      return llvm::Type::getInt64Ty(TheContext);
    }else if(qualType == "long"){
      return llvm::Type::getInt64Ty(TheContext);
    }else if(qualType == "const long"){
      isConst = true;
      return llvm::Type::getInt64Ty(TheContext);
    }else if(qualType == "char"){
      return llvm::Type::getInt8Ty(TheContext);
    }else if(qualType == "const char"){
      isConst = true;
      return llvm::Type::getInt8Ty(TheContext);
    }
    isArray = true;
    //是数组
    if(qualType.substr(0,3) == "int"){
      type = llvm::Type::getInt32Ty(TheContext);
    }else if(qualType.substr(0,9) == "const int"){
      type = llvm::Type::getInt32Ty(TheContext);
      isConst = true;
    }else if(qualType.substr(0,4) == "char"){
      type = llvm::Type::getInt8Ty(TheContext);
    }else if(qualType.substr(0,10) == "const char"){
      type = llvm::Type::getInt8Ty(TheContext);
      isConst = true;
    }else if(qualType.substr(0,9) == "long long"){
      type = llvm::Type::getInt64Ty(TheContext);
    }else if(qualType.substr(0,15) == "const long long"){
      type = llvm::Type::getInt64Ty(TheContext);
      isConst = true;
    }else if(qualType.substr(0,4) == "long"){
      type = llvm::Type::getInt64Ty(TheContext);
    }else if(qualType.substr(0,10) == "const long"){
      type = llvm::Type::getInt64Ty(TheContext);
      isConst = true;
    }

    for (int i = size; i >= 0; i --){
        char c = qualType[i];
        if(c == ']'){
            int num_end = i;
            while(c!='['){
                c = qualType[i];
                i--;
            }
            int num_start = i+2;
            int num = stoi(qualType.substr(num_start, num_end-num_start), nullptr, 10);
            type = llvm::ArrayType::get(type,num);
            i++;
        }
    }
    return type;
}
//函数返回值类型
llvm::Type * GetfuncRetType(string &qualType){
  if(qualType.substr(0,3) == "int"){
    return llvm::Type::getInt32Ty(TheContext);
  }else if(qualType.substr(0,4) == "void"){
    return llvm::Type::getVoidTy(TheContext);
  }else if(qualType.substr(0,9) == "long long"){
    return llvm::Type::getInt64Ty(TheContext);
  }
  // bool isArray = false;
  // bool isConst = false;
  // return ParseType(qualType, isArray, isConst);

  return nullptr;
}


vector<int> ParseArrDims(string &qualType){
  int num = 0;
  vector<int> arr;
  for(auto c : qualType){
    if(c == '['){
      num = 0;
    }else if(c == ']'){
      arr.push_back(num);
    }else if(c >= '0' && c <= '9'){
      num = num * 10 + c-'0';
    } 
  }
  // cout<<num<<endl;
  return arr;
}

llvm:: Value * BuildLocalVar( string &id){
  // cout<<"id:"<<id<<endl;
  if(VarEnv.find(id) == VarEnv.end()){
    return nullptr;
  }else{
    
    return VarEnv[id].first;
  }
}


void BuildArrList( const llvm::json ::Object*O, vector<llvm::Value*> &idxList){
    assert(O->getString("kind")->str() == "ArraySubscriptExpr");
    auto Arr = O->getArray("inner");
    auto ImplicitCastExpr = Arr->begin()->getAsObject();
    //取出 下标
    auto index_obj =(Arr->begin()+1)->getAsObject();
    string kind = index_obj->getString("kind")->str();
    
    idxList.push_back(BuildAny(index_obj));

    Arr = ImplicitCastExpr->getArray("inner");

    if (Arr->begin()->getAsObject()->getString("kind")->str() == "ArraySubscriptExpr")
    {
      BuildArrList(Arr->begin()->getAsObject(),idxList);
    }
}

string ParseId(const llvm::json::Object *O){
  auto inner = O->getArray("inner");
  auto Arr = *O->getArray("inner");
  auto kind = Arr[0].getAsObject()->getString("kind")->str();
  if( kind == "DeclRefExpr"){
    auto referencedDecl = Arr[0].getAsObject()->getObject("referencedDecl");
    string id = referencedDecl-> getString("id")->str();
    return id;
  }else if( kind == "ArraySubscriptExpr"){
      return ParseArrId(Arr[0].getAsObject());
  }else if( kind == "ParenExpr"){
    auto inner = Arr[0].getAsObject()->getArray("inner");
    auto next = ((*inner)[0]).getAsObject();
    return ParseId(next);
  }else{
    return "";
  }
}

string ParseArrId(const llvm::json::Object *O){
    assert(O->getString("kind")->str() == "ArraySubscriptExpr");
    auto inner = O->getArray("inner");
    auto ImplicitCastExpr = ((*inner)[0]).getAsObject();
    //取出 DeclRefExpr
    inner = ImplicitCastExpr->getArray("inner");
    auto son = ((*inner)[0]).getAsObject();
    if(son->getString("kind")->str() == "DeclRefExpr"){
      return ParseId(ImplicitCastExpr);
    }else{
      return ParseArrId(son) ;
    }
}

string ParseName(const llvm::json::Object *O){
  auto inner = O->getArray("inner");
  auto Arr = *O->getArray("inner");
  auto kind = Arr[0].getAsObject()->getString("kind")->str();
  if( kind == "DeclRefExpr"){
    auto referencedDecl = Arr[0].getAsObject()->getObject("referencedDecl");
    string name = referencedDecl-> getString("name")->str();
    return name;
  }else if( kind == "ArraySubscriptExpr"){
      return ParseArrName(Arr[0].getAsObject());
  }else if( kind == "ParenExpr"){
    auto inner = Arr[0].getAsObject()->getArray("inner");
    auto next = ((*inner)[0]).getAsObject();
    return ParseName(next);
  }else{
    return "";
  }
}

string ParseArrName(const llvm::json::Object *O){
    assert(O->getString("kind")->str() == "ArraySubscriptExpr");
    auto inner = O->getArray("inner");
    auto ImplicitCastExpr = ((*inner)[0]).getAsObject();
    //取出 DeclRefExpr
    inner = ImplicitCastExpr->getArray("inner");
    auto son = ((*inner)[0]).getAsObject();
    if(son->getString("kind")->str() == "DeclRefExpr"){
      return ParseName(ImplicitCastExpr);
    }else{
      return ParseArrName(son) ;
    }
}

llvm::Constant* BuildGlobalArrayInit(const llvm::json::Object *O ){
  // assert_type(O,"InitListExpr");   
  auto qualType = O->getObject("type")-> getString("qualType")->str();
  vector<int> DimList = ParseArrDims(qualType);
  int DimNum = DimList.size();
  if(DimNum == 1){
    int FinalDim = DimList[0];
    auto array_type= llvm::ArrayType::get(llvm::Type::getInt32Ty(TheContext),FinalDim);
    auto array_filler =  O->getArray("array_filler");
    if(array_filler == nullptr){
        vector<llvm::Constant*> value_list;
        auto inner = O->getArray("inner");
        int size = inner->size();
        for(int i = 0; i < size; ++i){
          auto value = BuildAny(((*inner)[i]).getAsObject());
          value_list.push_back(llvm::dyn_cast<llvm::Constant>(value));
        }
        return  llvm::ConstantArray::get(array_type,value_list);
    }else{
      vector<llvm::Constant*> value_list;
      int array_filler_size = array_filler->size();
      for(int i = 1; i < array_filler_size; ++i){
        auto value = BuildAny(((*array_filler)[i]).getAsObject());
        value_list.push_back(llvm::dyn_cast<llvm::Constant>(value));
      }
      while( value_list.size() < FinalDim){
        value_list.push_back(llvm::ConstantInt::get(TheContext,llvm::APInt(32,  "0" , 10)));
      }
      return  llvm::ConstantArray::get(array_type, value_list);
    }
  }
  else{
    int FinalDim = DimList[DimNum-1];
    auto array_type= llvm::ArrayType::get(llvm::Type::getInt32Ty(TheContext),FinalDim) ;
    for(int i = DimNum-2;i>=0; --i ){
        array_type =  llvm::ArrayType::get( array_type,DimList[i] ) ;
    }
      auto array_filler =  O->getArray("array_filler");
      int cur_dim = DimList[0];
      if(array_filler == nullptr){
          vector<llvm::Constant*> value_list;
          auto inner = O->getArray("inner");
          int size = inner->size();
          for(int i = 0; i < size; ++i){
            auto InitListExpr  = ((*inner)[i]).getAsObject();
            auto val = BuildGlobalArrayInit(InitListExpr);
            value_list.push_back(val);
          }
          return  llvm::ConstantArray::get(array_type,value_list);
      }else {
        vector<llvm::Constant*> value_list;
        for(int i = 1; i <array_filler->size(); ++i ){
          auto InitListExpr  = ((*array_filler)[i]).getAsObject();
          auto val = BuildGlobalArrayInit(InitListExpr);
          value_list.push_back(val);
        }
        while(value_list.size() < cur_dim){
          value_list.push_back(llvm::ConstantAggregateZero::get(llvm::Type::getInt32Ty(TheContext) ));
        }
        return llvm::ConstantArray::get(array_type,value_list);
      }
  }
  return nullptr;
}

void BuildGlobalVarDecl(const llvm::json::Object *O){
  if(O == nullptr)return;
  assert(O->getString("kind")->str() == "VarDecl");

  //if(O->getString("qualType")->str() == "int"){

  string name = O->getString("name")->str();
  string qualType = O->getObject("type")->getString("qualType")->str();
  //获取类型
  bool isArray = false;
  bool isConst = false;
  auto type = ParseType(qualType, isArray, isConst);
  TheModule.getOrInsertGlobal(name,type);
  llvm::GlobalVariable *globalVar = TheModule.getNamedGlobal(name);

  if(auto inner = O->getArray("inner")){
    if(isArray){
      // auto Arr = *O->getArray("inner");
      // string kind = Arr[0].getAsObject()->getString("kind")->str();
      // if(kind == "InitListExpr"){
      //   llvm::Constant* const_array = BuildGlobalInitListExpr(Arr[0].getAsObject(), type);
      //   globalVar->setInitializer(const_array);
      // }
      // globalVar->setAlignment(llvm::MaybeAlign(16));
      vector<int> DimList = ParseArrDims(qualType);
      int DimNum = DimList.size();
      int FinalDim = DimList[DimNum-1];
      auto array_type= llvm::ArrayType::get(llvm::Type::getInt32Ty(TheContext),FinalDim) ;
      for(int i = DimNum-2; i>=0 ; i--){
        array_type =  llvm::ArrayType::get( array_type,DimList[i] ) ;
      }
      auto inner = O->getArray("inner");
      if(!inner){
        TheModule.getOrInsertGlobal(name,array_type);
        llvm::GlobalVariable *globalVar = TheModule.getNamedGlobal(name);
        globalVar->setInitializer(llvm::ConstantAggregateZero::get(llvm::Type::getInt32Ty(TheContext) ));
      }else{
        TheModule.getOrInsertGlobal(name,array_type);
        llvm::GlobalVariable *globalVar = TheModule.getNamedGlobal(name);
        auto InitListExpr = ((*inner)[0]).getAsObject();
        globalVar->setInitializer( BuildGlobalArrayInit(InitListExpr));
      }
    }else{
      for(const auto & it : *inner){
        auto value = BuildAny(it.getAsObject());
        assert(value->getType() == llvm::Type::getInt32Ty(TheContext));
        globalVar->setInitializer(dyn_cast<llvm::Constant>(value));
      }
      // globalVar->setAlignment(llvm::MaybeAlign(4));
    }
    if(isConst)globalVar->setConstant(true);
    
    globalVar->setDSOLocal(true);
  }else{
    //初始化为零
    if(isArray){
      llvm::ConstantAggregateZero* const_array_zero = llvm::ConstantAggregateZero::get(type);
      globalVar->setInitializer(const_array_zero);
      // globalVar->setAlignment(llvm::MaybeAlign(16));
    }else{
      auto value = llvm::ConstantInt::get(TheContext, llvm::APInt(32, "0", 10));
      globalVar->setInitializer(value);
      // globalVar->setAlignment(llvm::MaybeAlign(4));
    }
    if(isConst)globalVar->setConstant(true);
    globalVar->setDSOLocal(true);
  }
  
  return;
}


llvm::Value * BuildArraySubscriptExpr( const llvm::json:: Object*O){
  auto arr = BuildArrGEP(O); 
  return Builder.CreateLoad(arr);
}


llvm::Value * BuildArrGEP( const llvm::json:: Object*O){
  assert(O->getString("kind")->str() == "ArraySubscriptExpr");
  auto name = ParseArrName(O);
  auto id = ParseArrId(O);
  llvm:: Value * baseptr = nullptr;
  bool is_para_array = false;
  
  if(VarEnv.find(id) == VarEnv.end()){
    baseptr = nullptr;
  }else{
    baseptr = VarEnv[id].first;
    is_para_array = VarEnv[id].second;
  }
  if(baseptr == nullptr)baseptr = TheModule.getNamedGlobal(name);
  //**********************************************************************


  vector<llvm::Value *> indices;
  if(!is_para_array)indices.push_back( llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0)));
  
  vector<llvm::Value* > idxList;
  BuildArrList(O,idxList);
  reverse(idxList.begin(),idxList.end());
  for(int i = 0; i < idxList.size(); i++){
    indices.push_back(idxList[i]);
  }
  if(!is_para_array)return Builder.CreateInBoundsGEP( baseptr,indices);
  else{
      auto base = Builder.CreateLoad(baseptr);      
      auto arrayidx = Builder.CreateInBoundsGEP(base, {idxList[0]});
      for(int i = 1; i < idxList.size() ; i++){
        arrayidx = Builder.CreateInBoundsGEP(arrayidx, { llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0)) ,  idxList[i] });
      }
      return  arrayidx;                        
  }
  return  nullptr;
}

vector<int> GetIndexList(vector<int> &DimList,int index){
  vector<int>res;
  for(int i = 0; i < DimList.size(); ++i){
    int t = 1;
    for(int j = i+1; j<DimList.size(); ++j){
      t*=DimList[j];
    }
    res.push_back(index/t);
    index = index % t;
  }
  return res;
}

int GetArrSize(string qualtype){
  vector<int> res;
  int dim = 0;
  bool begin=false;
  int total = 1;
  for(int i = 0; i < qualtype.size();++i){
    if(qualtype[i]=='['){
      begin = true;
      dim = 0;
    }
    if(qualtype[i]==']'){
      total *= dim;
      begin = false;
      dim = 0;
    }
    if(true && qualtype[i]>='0' && qualtype[i]<='9'){
      dim = dim*10 + int(qualtype[i]-'0');
    }
  }
  return total;
}

void GetInitValues(const llvm::json::Object *O,vector<llvm::Value*> &value_list){
  auto qualtype = O->getObject("type")->getString("qualType")->str();
  vector<int> DimList = ParseArrDims(qualtype);
  int DimNum = DimList.size();
  if(DimNum==1){
    int FinalDim = DimList[0];
    auto filler =  O->getArray("array_filler");
    if(!filler){
      auto inner = O->getArray("inner");
      int size = inner->size();
      for(int i = 0; i < size; ++i){
        auto son = (inner->begin()+i)->getAsObject();
        value_list.push_back(BuildAny(son));
      }
    }else{
      int array_filler_size = filler->size();
      int cnt = array_filler_size - 1;
      for(int i = 1; i < array_filler_size; ++i){
        auto son = (filler->begin()+i)->getAsObject();
        if(son->getString("kind")->str() == "IntegerLiteral"){
          value_list.push_back(BuildAny(son));
        }
      }
      while(cnt < FinalDim){
        cnt++;
        value_list.push_back(llvm::ConstantInt::get(TheContext,llvm::APInt(32,  "0" , 10)));
      }
    }
  }else{
    auto filler =  O->getArray("array_filler");
    if(filler == nullptr){
      auto inner = O->getArray("inner");
      int size = inner->size();
      for(int i = 0; i < size; ++i){
        auto InitListExpr  = (inner->begin()+i)->getAsObject();
        GetInitValues(InitListExpr,value_list);
      }
    }else{
      int array_filler_size = filler->size();
      for(int i = 1; i < array_filler_size; ++i){
          auto InitListExpr  = (filler->begin()+i)->getAsObject();
          GetInitValues(InitListExpr,value_list);
      }
      auto ImplicitValueInitExpr = filler->begin()->getAsObject();
      auto ImplicitValueInitExpr_qualtype = ImplicitValueInitExpr->getObject("type")->getString("qualType")->str();
      int zeros = GetArrSize(ImplicitValueInitExpr_qualtype);
      int Implicit_num = DimList[0] - array_filler_size + 1;
      zeros *= Implicit_num;
      for(int i = 0; i < zeros; ++i){
        value_list.push_back(llvm::ConstantInt::get(TheContext,llvm::APInt(32,  "0" , 10)));
      }
    }

  }
  
}

llvm::Value* BuildCallExpr(const llvm::json::Object *O){
  
  auto inner = O->getArray("inner");
  llvm::Function* func = nullptr;
  vector<llvm::Value*> argsarr;
  for(int i = 0; i < inner->size(); i ++){
    string kind = (inner->begin()+i)->getAsObject()->getString("kind")->str();
    if(i == 0){
      if(kind == "ImplicitCastExpr"){
        func = (llvm::Function*)BuildImplicitCastExpr((inner->begin()+i)->getAsObject());
        assert(func != nullptr);
      }
    }else{
      auto value = BuildAny((inner->begin()+i)->getAsObject());
      argsarr.push_back(value);
    }
  }
  auto value = Builder.CreateCall(func, argsarr);
  return value;
}

llvm::Value* BuildFuncParmsImplicitCastExpr(const llvm::json::Object *O){
  
  assert(O->getString("kind")->str() == "ImplicitCastExpr"); 
  auto inner = O->getArray("inner");
  if(inner->begin()->getAsObject()->getString("kind")){
    string kind = inner->begin()->getAsObject()->getString("kind")->str();
    if(kind == "DeclRefExpr"){
      auto a_ptr = BuildFuncParmsDeclRefExpr(inner->begin()->getAsObject());
      return a_ptr;
    }
  }
  return nullptr;
}

//返回变量指针
llvm::Value* BuildImplicitCastExpr(const llvm::json::Object *O){
  
  assert(O->getString("kind")->str() == "ImplicitCastExpr"); 
  auto inner = O->getArray("inner");
  string castkind = O->getString("castKind")->str();
  string qualType = O->getObject("type")->getString("qualType")->str();
  if(inner->begin()->getAsObject()->getString("kind")){
    string kind = inner->begin()->getAsObject()->getString("kind")->str();
    
    auto son = inner->begin()->getAsObject();
     if(kind == "ArraySubscriptExpr"){
      if (auto cast = O->get("castKind")->getAsString()){
        if(cast->str()=="ArrayToPointerDecay"){   
            string name = ParseArrName(son);
            string id = ParseArrId(son);
            auto ptr = BuildLocalVar(id);
            if(ptr==nullptr)
            ptr = TheModule.getGlobalVariable(name);
            vector<llvm::Value*> idxList;
            BuildArrList(son,idxList);
            idxList.push_back(llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0)));
            // idxList.push_front(llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0)));
            idxList.insert(idxList.begin(), llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0)));
            return  Builder.CreateInBoundsGEP(ptr, idxList, "arrayidx");
        }
      }
      auto val =  BuildArraySubscriptExpr(son);
      return val;
    }else if(kind == "StringLiteral"){
      auto baseptr = BuildStringLiteral(inner->begin()->getAsObject(), "str");
      auto ptr = Builder.CreateInBoundsGEP(baseptr, { llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0)) , llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0))});
      return ptr;
    }else {
      auto val = BuildAny(inner->begin()->getAsObject());
      if(castkind == "LValueToRValue"){
        return Builder.CreateLoad(val);
      }else if(castkind == "IntegralCast"){
        if(qualType=="int"){
          if(val->getType()==llvm::Type::getInt8Ty(TheContext)){
            val = i8Toi32(val);
          }else if(val->getType()==llvm::Type::getInt64Ty(TheContext)){
            val = i64Toi32(val);
          }
        }else if(qualType == "long long" || qualType == "long"){
          if(val->getType()==llvm::Type::getInt32Ty(TheContext)){
            val = i32Toi64(val);
          }else if(val->getType()==llvm::Type::getInt8Ty(TheContext)){
            val = i8Toi64(val);
          }
        }
      }else if(castkind == "ArrayToPointerDecay"){
        return  Builder.CreateInBoundsGEP(val, 
        {llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0)),
        llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0))}, 
        "arrayidx");
      }
      return val;
    }
  }
  return nullptr;
}

llvm::Value* BuildBinaryExpr(const llvm::json::Object *O){
  auto inner = O->getArray("inner");
  llvm::Value * L;
  llvm::Value * R;

  string Op = O->getString("opcode")->str();

  if(Op == "&&"){

    llvm::BasicBlock * thenblock = ShortCircuitThen.top();
    llvm::BasicBlock * skipblock = ShortCircuitSkip.top();
    //右边成为一个新的块
    llvm::BasicBlock *rightBB = llvm::BasicBlock::Create(TheContext, "and.right");
    
    //处理左边的语句
    ShortCircuitThen.push(rightBB);
    ShortCircuitSkip.push(skipblock);

    L = BuildAny(inner->begin()->getAsObject());

    ShortCircuitThen.pop();
    ShortCircuitSkip.pop();

    if(L)Builder.CreateCondBr(i32Toi1(L), rightBB, skipblock);
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
    TheFunction->getBasicBlockList().push_back(rightBB);
    Builder.SetInsertPoint(rightBB);

    R = BuildAny((inner->begin()+1)->getAsObject());

    if(R)Builder.CreateCondBr(i32Toi1(R), thenblock, skipblock);


    return nullptr;
    
    
  }else if(Op == "||"){

    llvm::BasicBlock * thenblock = ShortCircuitThen.top();
    llvm::BasicBlock * skipblock = ShortCircuitSkip.top();

    //右边成为一个新的块
    llvm::BasicBlock *rightBB = llvm::BasicBlock::Create(TheContext, "or.right");
    //处理左边的语句
    ShortCircuitThen.push(thenblock);
    ShortCircuitSkip.push(rightBB);

    L = BuildAny(inner->begin()->getAsObject());

    ShortCircuitThen.pop();
    ShortCircuitSkip.pop();
    

    if(L)Builder.CreateCondBr(i32Toi1(L), thenblock, rightBB);
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
    TheFunction->getBasicBlockList().push_back(rightBB);
    Builder.SetInsertPoint(rightBB);

    R = BuildAny((inner->begin()+1)->getAsObject());

    if(R)Builder.CreateCondBr(i32Toi1(R), thenblock, skipblock);
    return nullptr;
    
  }else{
    L = BuildAny(inner->begin()->getAsObject());
  }
  
  R = BuildAny((inner->begin()+1)->getAsObject());


  assert(L != nullptr);
  assert(R != nullptr);

  
  if(Op == "+"){
    return Builder.CreateNSWAdd(L, R, "addtmp");
  }else if(Op == "="){
    Builder.CreateStore(R, L);
    return nullptr;
  }else if(Op == "-"){
    return Builder.CreateNSWSub(L, R, "subtmp");
  }else if(Op == "*"){
    return Builder.CreateNSWMul(L, R, "multmp");
  }else if(Op == "%"){
    return Builder.CreateSRem(L, R, "modtmp");
  }else if(Op == "/"){
    return Builder.CreateSDiv(L, R,"divtmp");
  }else if(Op == "=="){
    L = i1Toi32(L);
    R = i1Toi32(R);
    return Builder.CreateICmpEQ(L, R, "eqtmp");
  }else if(Op == "!="){
    L = i1Toi32(L);
    R = i1Toi32(R);
    return Builder.CreateICmpNE(L, R, "netmp");
  }else if(Op == ">"){
    L = i1Toi32(L);
    R = i1Toi32(R);
    return Builder.CreateICmpSGT(L, R, "sgttmp");
  }else if(Op == "<"){
    L = i1Toi32(L);
    R = i1Toi32(R);
    return Builder.CreateICmpSLT(L, R, "slttmp");
  }else if(Op == "<="){
    L = i1Toi32(L);
    R = i1Toi32(R);
    return Builder.CreateICmpSLE(L, R, "sletmp");
  }else if(Op == ">="){
    L = i1Toi32(L);
    R = i1Toi32(R);
    return Builder.CreateICmpSGE(L, R, "sgetmp");
  }else{
    return nullptr;
  }
}

llvm:: Value* BuildParenExpr(const llvm::json::Object *O){
  assert(O->getString("kind")->str() == "ParenExpr");
  auto Arr = O->getArray("inner");
  string kind =  Arr->begin()->getAsObject()->getString("kind")->str();
  return BuildAny(Arr->begin()->getAsObject());
}

llvm:: Value* BuildUnaryExpr(const llvm::json::Object *O){
  assert(O->getString("kind")->str() == "UnaryOperator");
  
  string Op = O->getString("opcode")->str();
  auto inner = O->getArray("inner");
  string kind = inner->begin()->getAsObject()->getString("kind")->str();
  
  if(kind == "IntegerLiteral"){
    string valuestr = inner->begin()->getAsObject()->getString("value")->str();
    long long val = stoll(valuestr);
    if(Op == "-"){
      return llvm::ConstantInt::get(TheContext, llvm::APInt(32, (-1)*val));
    }else if(Op == "+"){
      return llvm::ConstantInt::get(TheContext, llvm::APInt(32, val));
    }else if(Op == "!"){
      if(val == 0){
        return llvm::ConstantInt::get(TheContext, llvm::APInt(1, 1));
      }else{
        return llvm::ConstantInt::get(TheContext, llvm::APInt(1, 0));
      }
    }
  }else{
    auto val = BuildAny(inner->begin()->getAsObject());
    if (Op=="-"){
      val = i1Toi32(val);
      return Builder.CreateNeg( val, "neg");
    }else if (Op == "+"){
      val = i1Toi32(val);
      return val;
    }else if(Op == "!"){
      auto temptrue = llvm::ConstantInt::get(TheContext, llvm::APInt(1, 1));
      return  Builder.CreateXor(temptrue, i32Toi1(val), "nottmp");
    }
  }
  
  return nullptr;
  
}

void BuildReturnStmt(const llvm::json::Object *O){
  
  assert(O->getString("kind")->str() == "ReturnStmt");
  //**********************************************************
  //这个暂时不清楚是什么，好像是retval
  // auto a_ptr = Builder.CreateAlloca(llvm::Type::getInt32Ty(TheContext), nullptr);
  // Builder.CreateStore(llvm::ConstantInt::get(TheContext, llvm::APInt(32, "0", 10)), a_ptr);
  //**********************************************************
  //有返回内容
  if(O->getArray("inner")){
    auto Arr = *O->getArray("inner");
    if(auto inner = Arr[0].getAsObject()->getArray("inner")){
      auto kind = Arr[0].getAsObject()->getString("kind")->str();
      auto ptr = BuildAny(Arr[0].getAsObject());
      Builder.CreateRet(ptr);
    }else{ //直接返回数值
      auto value = Arr[0].getAsObject();
      if (auto RetVal = llvm::ConstantInt::get(TheContext, BuildIntegerLiteral(value))) {
        // Finish off the function.
        Builder.CreateRet(RetVal);  //返回值指令语句，具体IR的API
      }
    }
  }else{ //返回void
    Builder.CreateRetVoid();
  }
  return;
}

void BuildInitListExpr(const llvm::json::Object *O, llvm::Value* array, llvm::Type* type, vector<llvm::Value*> idxList){
  
  if(O->getArray("inner")){
    auto inner = O->getArray("inner");
    for(int i = 0; i < inner->size(); i ++){
      if((inner->begin()+i)->getAsObject()->getString("kind")){
        auto kind = (inner->begin()+i)->getAsObject()->getString("kind")->str();
        if(kind == "InitListExpr"){
          llvm::Value* idx = llvm::ConstantInt::get(TheContext, llvm::APInt(32, i));
          idxList.push_back(idx);
          BuildInitListExpr((inner->begin()+i)->getAsObject(), array, type, idxList);
          idxList.pop_back();
        }else{
          llvm::Value* value;
          value = BuildAny((inner->begin()+i)->getAsObject());
          llvm::Value* idx = llvm::ConstantInt::get(TheContext, llvm::APInt(32, i));
          idxList.push_back(idx);
          auto ptr = Builder.CreateInBoundsGEP(array, idxList);
          idxList.pop_back();
          Builder.CreateStore(value, ptr);
        }
      }
    }
  }
}

llvm::Constant* ParseEscapeString(string &str, llvm::ArrayType* array_type, int size){
  vector<llvm::Constant*> value_list;

  for(int i = 1; i < str.size()-1; i ++){
    if(str[i] == '\\'){
      assert(i+1 < str.size());
      i++;
      if(str[i] == 'n'){
        auto val =  llvm::ConstantInt::get(TheContext,llvm::APInt(8, '\n', 10));
        value_list.push_back(val);
        continue;
      }
      auto val =  llvm::ConstantInt::get(TheContext,llvm::APInt(8, str[i], 10));
      value_list.push_back(val);
    }else{
      auto val =  llvm::ConstantInt::get(TheContext,llvm::APInt(8, str[i], 10));
      value_list.push_back(val);
    }
  }

  for(int i = value_list.size(); i < size; i ++){
    auto val =  llvm::ConstantInt::get(TheContext,llvm::APInt(8, '\0', 10));
    value_list.push_back(val);
  }

  return llvm::ConstantArray::get(array_type,value_list);;
}

llvm::Value* BuildStringLiteral(const llvm::json::Object *O, string name){
  assert(O->getString("kind")->str() == "StringLiteral");
  string str = O->getString("value")->str();
  string id = O->getString("id")->str();
  
  string qualType = O->getObject("type")->getString("qualType")->str();
  
  int size = GetArrSize(qualType);
  
  auto array_type= llvm::ArrayType::get(llvm::Type::getInt8Ty(TheContext),size);

  auto escape = ParseEscapeString(str, array_type, size);
  TheModule.getOrInsertGlobal(name,array_type);
  llvm::GlobalVariable *globalVar = TheModule.getNamedGlobal(name);
  globalVar->setInitializer(escape);
  globalVar->setConstant(true);
  return globalVar;
}

void BuildVarDecl(const llvm::json::Object *O){
  
  assert(O->getString("kind")->str() == "VarDecl");
  
  string qualType = O->getObject("type")->getString("qualType")->str();
  string mangledName = O->getString("mangledName")->str();
  string id = O->getString("id")->str();
  //获取类型
  bool isArray = false;
  bool isConst = false;
  auto type = ParseType(qualType, isArray, isConst);
  
  //分配空间并初始化
  llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
  llvm::IRBuilder<> tempBuilder(&TheFunction->getEntryBlock(),
                           TheFunction->getEntryBlock().begin());
  
  if(O->getArray("inner")){ //初始化
    auto inner = O->getArray("inner");
    string kind = inner->begin()->getAsObject()->getString("kind")->str();
    if(isArray){  //是数组类型
      // //分配空间
      if(kind == "StringLiteral"){
        auto value = BuildStringLiteral(inner->begin()->getAsObject(), mangledName);
        
      }else{
        string name = O->getString("name")->str();
        vector<int> DimList = ParseArrDims(qualType);
        int DimNum = DimList.size();
        int FinalDim = DimList[DimNum-1];
        auto array_type= llvm::ArrayType::get(llvm::Type::getInt32Ty(TheContext),FinalDim) ;
        for(int i = DimNum-2;i>=0; --i ){
          array_type =  llvm::ArrayType::get( array_type,DimList[i] ) ;
        }
        auto ptr = tempBuilder.CreateAlloca(array_type,nullptr,name); //创建
        VarEnv[id] = make_pair(ptr,false);
        if(auto inner = O->getArray("inner")){
            //获取初始化的 vector<llvm::value*>
            vector<llvm::Value*> initlist; 
            auto son = inner->begin()->getAsObject();
            GetInitValues(son,initlist);
            int size = GetArrSize(qualType);
            // 每个元素初始化
            for(int i = 0; i < size; i++){
                vector<llvm::Value *> indices;
                indices.push_back( llvm::ConstantInt::get(TheContext,llvm::APInt(32,  "0" , 10) ) );
                std :: vector<int> index_list = GetIndexList(DimList,i);
                for(int k = 0; k < index_list.size(); ++k){
                  indices.push_back( llvm::ConstantInt::get(TheContext,llvm::APInt(32,  to_string(index_list[k])  , 10) ) );
                }
                auto gep = Builder.CreateInBoundsGEP(ptr,indices,"geptmp" );
                Builder.CreateStore(initlist[i],gep );
            }
        } 
      }
      
    }else{  //不是数组类型
      llvm::Value* value; //根据kind的不同，获取value的方式不同
      value = BuildAny(inner->begin()->getAsObject());
      //分配空间
      string name = O->getString("name")->str();
      auto a_ptr = tempBuilder.CreateAlloca(type, nullptr, name);
      VarEnv[id] = make_pair(a_ptr,false);
      //赋值
      if(a_ptr->getType() == llvm::Type::getInt64PtrTy(TheContext)){
        value = i32Toi64(value);
      }
      Builder.CreateStore(value, a_ptr);
    }
  }else{ //没有初始化
    
    string name = O->getString("name")->str();
    auto a_ptr = tempBuilder.CreateAlloca(type, nullptr, name);
    
    VarEnv[id] = make_pair(a_ptr,false);
  }
  return;
}

void BuildDeclStmt(const llvm::json::Object *O){
  
  assert(O->getString("kind")->str() == "DeclStmt");
  auto Arr = *O->getArray("inner");
  for(auto &vardecl : Arr){
    if(vardecl.getAsObject()->getString("kind")->str() == "VarDecl"){
      BuildVarDecl(vardecl.getAsObject());  
    }
    // BuildAny(vardecl.getAsObject());
  }
}

void BuildIfStmt(const llvm::json::Object *O){
  assert(O->getString("kind")->str() == "IfStmt");
  llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
  llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(TheContext, "ifthen");
  llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(TheContext, "ifelse");
  llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(TheContext, "ifcont");
  
  // ShortCircuitElseBB = ElseBB;//保存一下
  // ShortCircuitThenBB = ThenBB;//保存一下
  // ShortCircuitIfContBB = MergeBB;//保存一下

  auto hasElse = O->get("hasElse");

  //短路设置*********************************************************
  if(hasElse){
    ShortCircuitThen.push(ThenBB);
    ShortCircuitSkip.push(ElseBB);
  }else{
    ShortCircuitThen.push(ThenBB);
    ShortCircuitSkip.push(MergeBB);
  }

  auto Arr = O->getArray("inner");
  llvm::Value* condition;
  //条件判断
  string conditionkind = Arr->begin()->getAsObject()->getString("kind")->str();
  condition = BuildAny(Arr->begin()->getAsObject());
  if(condition){
    if(condition->getType() == llvm::Type::getInt32Ty(TheContext)){
      condition = Builder.CreateICmpNE(condition, 
      llvm::ConstantInt::get(TheContext, llvm::APInt(32, "0", 10)));
    }else if(condition->getType() == llvm::Type::getInt8Ty(TheContext)){
      condition = Builder.CreateICmpNE(condition, 
      llvm::ConstantInt::get(TheContext, llvm::APInt(8, "0", 10)));
    }
  }
  
  ShortCircuitThen.pop();
  ShortCircuitSkip.pop();
  //短路设置*********************************************************


  if (hasElse){
    
    if(condition)Builder.CreateCondBr(condition, ThenBB, ElseBB);
    TheFunction->getBasicBlockList().push_back(ThenBB);
    Builder.SetInsertPoint(ThenBB);
    BuildAny((Arr->begin()+1)->getAsObject());
    if(!Builder.GetInsertBlock()->getTerminator()){
      Builder.CreateBr(MergeBB);
      ThenBB = Builder.GetInsertBlock();
    }
    //else
    TheFunction->getBasicBlockList().push_back(ElseBB);
    Builder.SetInsertPoint(ElseBB);

    BuildAny((Arr->begin()+2)->getAsObject());
    if(!Builder.GetInsertBlock()->getTerminator()){
      Builder.CreateBr(MergeBB);
      ElseBB = Builder.GetInsertBlock();
      
    }
    TheFunction->getBasicBlockList().push_back(MergeBB);
    Builder.SetInsertPoint(MergeBB);
    
  }else{

    if(condition)Builder.CreateCondBr(condition, ThenBB, MergeBB);
    TheFunction->getBasicBlockList().push_back(ThenBB);
    Builder.SetInsertPoint(ThenBB);
    
    string thenkind = (Arr->begin()+1)->getAsObject()->getString("kind")->str();
    // if(thenkind == "BreakStmt"){
    //   BuildBreakStmt(Arr[1].getAsObject());
    //   TheFunction->getBasicBlockList().push_back(MergeBB);
    //   Builder.SetInsertPoint(MergeBB);
    //   return;
    // }else if(thenkind == "ContinueStmt"){
    //   BuildContinueStmt(Arr[1].getAsObject());
    //   TheFunction->getBasicBlockList().push_back(MergeBB);
    //   Builder.SetInsertPoint(MergeBB);
    //   return;
    // }else{
      BuildAny((Arr->begin()+1)->getAsObject());
    // }
    
    if(!Builder.GetInsertBlock()->getTerminator()){
      Builder.CreateBr(MergeBB);
      ThenBB = Builder.GetInsertBlock();
    }
    
    TheFunction->getBasicBlockList().push_back(MergeBB);
    Builder.SetInsertPoint(MergeBB);
    
  }
}

void BuildDoStmt(const llvm::json::Object *O){
    auto Arr = O->getArray("inner");
  llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
  llvm::BasicBlock * condBB = llvm::BasicBlock::Create(TheContext, "whilecond");
  llvm::BasicBlock * bodyBB = llvm::BasicBlock::Create(TheContext, "whilebody", TheFunction);
  llvm::BasicBlock * endBB = llvm::BasicBlock::Create(TheContext, "whileend");
  
  if(!Builder.GetInsertBlock()->getTerminator()){
    Builder.CreateBr(bodyBB);
  }
  Builder.SetInsertPoint(bodyBB);

  //设置breka和continue的目的地址
  BreakBB.push(endBB);
  ContinueBB.push(condBB);

  BuildAny(Arr->begin()->getAsObject());

  BreakBB.pop();
  ContinueBB.pop();

  Builder.CreateBr(condBB);
  TheFunction->getBasicBlockList().push_back(condBB);
  Builder.SetInsertPoint(condBB);

  string conditionkind = (Arr->begin()+1)->getAsObject()->getString("kind")->str();
  llvm::Value *condition;
  condition = BuildAny((Arr->begin()+1)->getAsObject());
  if(condition){
    if(condition->getType() == llvm::Type::getInt32Ty(TheContext)){
      condition = Builder.CreateICmpNE(condition, 
      llvm::ConstantInt::get(TheContext, llvm::APInt(32, "0", 10)));
    }else if(condition->getType() == llvm::Type::getInt8Ty(TheContext)){
      condition = Builder.CreateICmpNE(condition, 
      llvm::ConstantInt::get(TheContext, llvm::APInt(8, "0", 10)));
    }
  }

  if(condition)Builder.CreateCondBr(condition, bodyBB, endBB);

  if(!Builder.GetInsertBlock()->getTerminator()){
    Builder.CreateBr(bodyBB);
  }
  //之后的语句插入到endBB后面
  TheFunction->getBasicBlockList().push_back(endBB);
  Builder.SetInsertPoint(endBB);
}

void BuildWhileStmt(const llvm::json::Object *O){

  auto Arr = O->getArray("inner");

  llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
  llvm::BasicBlock * condBB = llvm::BasicBlock::Create(TheContext, "whilecond", TheFunction);
  llvm::BasicBlock * bodyBB = llvm::BasicBlock::Create(TheContext, "whilebody");
  llvm::BasicBlock * endBB = llvm::BasicBlock::Create(TheContext, "whileend");

  ShortCircuitThen.push(bodyBB);
  ShortCircuitSkip.push(endBB);

  Builder.CreateBr(condBB);
  Builder.SetInsertPoint(condBB);

  //这里的IR指令和条件判断有关
  string conditionkind = Arr->begin()->getAsObject()->getString("kind")->str();
  llvm::Value *condition;
  condition = BuildAny(Arr->begin()->getAsObject());
  if(condition){
    if(condition->getType() == llvm::Type::getInt32Ty(TheContext)){
      condition = Builder.CreateICmpNE(condition, 
      llvm::ConstantInt::get(TheContext, llvm::APInt(32, "0", 10)));
    }else if(condition->getType() == llvm::Type::getInt8Ty(TheContext)){
      condition = Builder.CreateICmpNE(condition, 
      llvm::ConstantInt::get(TheContext, llvm::APInt(8, "0", 10)));
    }
  }
  ShortCircuitThen.pop();
  ShortCircuitSkip.pop();

  // condBB = Builder.GetInsertBlock(); 

  //跳转，如果符合条件，跳转到body，否则结束循环
  if(condition)Builder.CreateCondBr(condition, bodyBB, endBB);

  
  TheFunction->getBasicBlockList().push_back(bodyBB);
  Builder.SetInsertPoint(bodyBB);


  //设置breka和continue的目的地址
  BreakBB.push(endBB);
  ContinueBB.push(condBB);

  string bodykind = (Arr->begin()+1)->getAsObject()->getString("kind")->str();
  BuildAny((Arr->begin()+1)->getAsObject());

  bodyBB = Builder.GetInsertBlock();
  //跳转回条件判断
  if(!Builder.GetInsertBlock()->getTerminator()){
    Builder.CreateBr(condBB);
  }
  //之后的语句插入到endBB后面
  TheFunction->getBasicBlockList().push_back(endBB);
  Builder.SetInsertPoint(endBB);
  
  BreakBB.pop();
  ContinueBB.pop();
}

void BuildBreakStmt(const llvm::json::Object *O){
  assert(O->getString("kind")->str() == "BreakStmt");

  Builder.CreateBr(BreakBB.top());
  
}

void BuildContinueStmt(const llvm::json::Object *O){
  assert(O->getString("kind")->str() == "ContinueStmt");
  Builder.CreateBr(ContinueBB.top());
}

void BuildCompoundStmt(const llvm::json::Object *O){
  assert(O->getString("kind")->str() == "CompoundStmt");
  if(O->getArray("inner")){
    // auto Arr = *O->getArray("inner");
    // for(int i = 0; i < Arr.size(); i ++){
    //   // string kind = Arr[i].getAsObject()->get("kind")->getAsString()->str();
    //   BuildAny(Arr[i].getAsObject());
    // }
    auto inner = O->getArray("inner");
    for(int i = 0; i < inner->size(); i ++){
      // string kind = Arr[i].getAsObject()->get("kind")->getAsString()->str();
      BuildAny((inner->begin()+i)->getAsObject());
    }
  }
  
}

bool IsPointerArr(string qualType){
  int flag1 = false;
  int flag2 = false;
  for(int i = 0; i < qualType.size(); i ++){
    if(qualType[i] == '*'){
      flag1 = true;
    }
    if(qualType[i] == '['){
      flag2 = true;
    }
  }
  return flag1 && flag2;
}

llvm::Value * BuildAny(const llvm::json::Object *O){
  string kind = O->getString("kind")->str();
  if(kind == "IntegerLiteral"){
    return llvm::ConstantInt::get(TheContext, BuildIntegerLiteral(O));
  }else if(kind == "BinaryOperator"){
    return BuildBinaryExpr(O);
  }else if(kind == "CallExpr"){
    return BuildCallExpr(O);
  }else if(kind == "ImplicitCastExpr"){
    return BuildImplicitCastExpr(O);
  }else if(kind == "UnaryOperator"){
    return BuildUnaryExpr(O);
  }else if(kind == "ParenExpr"){
    return BuildParenExpr(O);
  }else if(kind == "ArraySubscriptExpr"){
    return BuildArrGEP(O);
  }else if(kind == "DeclRefExpr"){
    return BuildDeclRefExpr(O);
  }else if(kind == "IfStmt"){
    BuildIfStmt(O);
    return nullptr;
  }else if(kind == "CompoundStmt"){
    BuildCompoundStmt(O);
    return nullptr;
  }else if(kind == "WhileStmt"){
    BuildWhileStmt(O);
    return nullptr;
  }else if(kind == "ReturnStmt"){
    BuildReturnStmt(O);
    return nullptr;
  }else if(kind == "DeclStmt"){
    BuildDeclStmt(O);
    return nullptr;
  }else if(kind == "BreakStmt"){
    BuildBreakStmt(O);
    return nullptr;
  }else if(kind == "ContinueStmt"){
    BuildContinueStmt(O);
    return nullptr;
  }else if(kind == "DoStmt"){
    BuildDoStmt(O);
    return nullptr;
  }


  return nullptr;
}

llvm::Function *buildFunctionDecl(const llvm::json::Object *O) {
  // First, check for an existing function from a previous declaration.
  if(O->getString("storageClass")){
    if(O->getString("storageClass")->str() == "extern")return nullptr;
  }
  auto TheName = O->get("name")->getAsString()->str();
  llvm::Function *TheFunction = TheModule.getFunction(TheName);

  const llvm::json::Object *inner = nullptr;
  vector<llvm::Type *> parm_types;
  vector<string> parm_names;
  vector<string> parm_ids;
  
  string typestr = O->get("type")->getAsObject()->get("qualType")->getAsString()->str();
  auto returntype = GetfuncRetType(typestr);
  bool hasCompoundStmt = false;
  //若函数不存在
  if (!TheFunction){

    if(auto arr = O->getArray("inner")){
      for (int i = 0; i < arr->size(); i ++){
        auto P = (arr->begin()+i)->getAsObject();
        string kind = P->get("kind")->getAsString()->str();
        if(kind == "ParmVarDecl"){
          string qualType = P->getObject("type")->get("qualType")->getAsString()->str();
          parm_names.push_back(P->getString("name")->str());
          parm_ids.push_back(P->getString("id")->str());
          if(qualType=="int"||qualType=="const int")parm_types.push_back(llvm:: Type::getInt32Ty(TheContext));
          else if(qualType=="char" || qualType=="const char")parm_types.push_back(llvm:: Type::getInt8Ty(TheContext));
          else if(qualType=="int *")parm_types.push_back(llvm:: Type::getInt32PtrTy(TheContext));
          else if(qualType=="const char *")parm_types.push_back(llvm:: Type::getInt8PtrTy(TheContext));
          else if(IsPointerArr(qualType)){
            vector<int> DimList = ParseArrDims(qualType);
            int FinalDim = DimList[DimList.size()-1];
            auto array_type= llvm::ArrayType::get(llvm:: Type::getInt32Ty(TheContext),FinalDim) ;
            for(int i = DimList.size()-2; i >= 0; --i){
              array_type = llvm::ArrayType::get(array_type,DimList[i]) ;
            }
            auto array_ptr =  array_type->getPointerTo();
            parm_types.push_back(array_ptr);
          }
        }else{
          hasCompoundStmt = true;
        }
      }
    }
    TheFunction = llvm::Function::Create(llvm::FunctionType::get(returntype, parm_types, false), llvm::Function::ExternalLinkage, TheName, &TheModule);  
    int idx = 0;
    for (auto &arg : TheFunction->args()){
      arg.setName(parm_names[idx++]);
    }
  }
                        
  if (!TheFunction)
    return nullptr;

  VarEnv.clear();
  if(O->getArray("inner") && hasCompoundStmt == true){
    // 为创建的Function添加Basic Block，“entry”是命名
    auto BB = llvm::BasicBlock::Create(TheContext, "entry", TheFunction);   
    Builder.SetInsertPoint(BB); //后面Builder生成的IR代码都将默认插到BB基本块上
    //使用IRBuilder插入指令到BB，比较重要，经常用到

    int idx = 0;
    for(auto &arg : TheFunction->args()){
      auto ptr = Builder.CreateAlloca(parm_types[idx], nullptr, arg.getName()+".addr");
      VarEnv[parm_ids[idx]] = make_pair(ptr,true);;
      idx++;
      Builder.CreateStore(&arg, ptr);
    }

  //遍历所有复合语句
    auto CompoundStmtArr = O->getArray("inner");
    for(int i = 0; i < CompoundStmtArr->size(); i ++){
      string kind = (CompoundStmtArr->begin()+i)->getAsObject()->getString("kind")->str();
      if(kind == "CompoundStmt"){
        BuildCompoundStmt((CompoundStmtArr->begin()+i)->getAsObject());
      }
    }
  

    if(!Builder.GetInsertBlock()->getTerminator()){
      if(typestr.substr(0,4)=="void"){
        Builder.CreateRet(nullptr);
      }else{
        auto val = llvm::ConstantInt::get(TheContext,llvm::APInt(32, "0", 10));
        Builder.CreateRet(val);
      }
    }
  }
  //设置dso_local
  TheFunction->setDSOLocal(true);
  // Validate the generated code, checking for consistency.
  llvm::verifyFunction(*TheFunction);
  return TheFunction;

  // Error reading body, remove function.
  TheFunction->eraseFromParent();
  return nullptr;
}

void buildTranslationUnitDecl(const llvm::json::Object *O) {
  //根结点
  if (O == nullptr)
    return;
  if (auto kind = O->get("kind")->getAsString()) {
    assert(*kind == "TranslationUnitDecl");
  } else {
    assert(0);
  }
  if (auto inner = O->getArray("inner"))
    for (const auto &it : *inner) //遍历内部结点
      if (auto P = it.getAsObject())
        if (auto kind = P->get("kind")->getAsString()) {
          if (*kind == "FunctionDecl")
            buildFunctionDecl(P); //具体IR生成
          else if (*kind == "VarDecl")
            BuildGlobalVarDecl(P);
        }
}
} // namespace

int main() {
  auto llvmin = llvm::MemoryBuffer::getFileOrSTDIN("-");
  auto json = llvm::json::parse(llvmin.get()->getBuffer());
  buildTranslationUnitDecl(json->getAsObject());
  TheModule.print(llvm::outs(), nullptr);
}