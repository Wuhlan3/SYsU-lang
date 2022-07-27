#include "optimizer.hh"

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <iostream>

llvm::PreservedAnalyses
sysu::StaticCallCounterPrinter::run(llvm::Module &M,
                                    llvm::ModuleAnalysisManager &MAM) {

  auto DirectCalls = MAM.getResult<sysu::StaticCallCounter>(M);

  OS << "=================================================\n";
  OS << "sysu-optimizer: static analysis results\n";
  OS << "=================================================\n";
  const char *str1 = "NAME", *str2 = "#N DIRECT CALLS";
  OS << llvm::format("%-20s %-10s\n", str1, str2);
  OS << "-------------------------------------------------\n";

  for (auto &CallCount : DirectCalls) {
    OS << llvm::format("%-20s %-10lu\n",
                       CallCount.first->getName().str().c_str(),
                       CallCount.second);
  }

  OS << "-------------------------------------------------\n\n";
  return llvm::PreservedAnalyses::all();
}

sysu::StaticCallCounter::Result
sysu::StaticCallCounter::run(llvm::Module &M, llvm::ModuleAnalysisManager &) {
  llvm::MapVector<const llvm::Function *, unsigned> Res;

  for (auto &Func : M) {
    for (auto &BB : Func) {
      for (auto &Ins : BB) {

        // If this is a call instruction then CB will be not null.
        auto *CB = llvm::dyn_cast<llvm::CallBase>(&Ins);
        if (nullptr == CB) {
          continue;
        }

        // If CB is a direct function call then DirectInvoc will be not null.
        auto DirectInvoc = CB->getCalledFunction();
        if (nullptr == DirectInvoc) {
          continue;
        }

        // We have a direct function call - update the count for the function
        // being called.
        auto CallCount = Res.find(DirectInvoc);
        if (Res.end() == CallCount) {
          CallCount = Res.insert({DirectInvoc, 0}).first;
        }
        ++CallCount->second;
      }
    }
  }

  return Res;
}


bool isCommon(llvm::BasicBlock::iterator i, llvm::BasicBlock::iterator j) {
  std::string OpName1 = i->getOpcodeName();
  std::string OpName2 = j->getOpcodeName();
	if(OpName1 == OpName2){
		if(i->getType()==j->getType()){
      int num1 = i->getNumOperands();
      int num2 = j->getNumOperands();
			if(num1 == num2){
				for(auto k=0;k<num1;k++){
					if(i->getOperand(k)!=j->getOperand(k))return 0;
				}
				return 1;
			}
    }
  }
  return 0;
}

llvm::PreservedAnalyses
sysu::CommonSubexpressionElimination::run(llvm::Module &M,llvm::ModuleAnalysisManager &MAM) {

  for (auto &Func : M) {
    for (auto &BB : Func) {
      for (auto i = BB.begin();i!=BB.end();i++) {
        if(strcmp(i->getOpcodeName(),"alloca")==0||strcmp(i->getOpcodeName(),"call")==0){
          continue;
        }
        auto j = i;
        j++;
        for(;j!=BB.end();){
          //若出现下一个store,说明变量会发生改变
          bool changed = false;
          std::string opName = j->getOpcodeName();
          if(opName == "store"){
            for(int k=0;k<i->getNumOperands();k++){
              if(j->getOperand(1)==i->getOperand(k)){
                changed = true;
              }
            }
          }
          if(changed)break;
          //判断语句是否相似
          if(isCommon(i,j)){
            j++;
            auto temp = j;
            j--;
            llvm::ReplaceInstWithValue(j->getParent()->getInstList(),j,&*i);
            j = temp;
          }
          else{
            j++;
          }
        }
      }
    }
  }

  return llvm::PreservedAnalyses::none();
}

llvm::AnalysisKey sysu::StaticCallCounter::Key;

extern "C" {
llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "sysu-optimizer-pass", LLVM_VERSION_STRING,
          [](llvm::PassBuilder &PB) {
            // #1 REGISTRATION FOR "opt -passes=sysu-optimizer-pass"
            PB.registerPipelineParsingCallback(
                [&](llvm::StringRef Name, llvm::ModulePassManager &MPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                  if (Name == "sysu-optimizer-pass") {
                    MPM.addPass(sysu::StaticCallCounterPrinter(llvm::errs()));
                    return true;
                  }
                  return false;
                });
            // #2 REGISTRATION FOR
            // "MAM.getResult<sysu::StaticCallCounter>(Module)"
            PB.registerAnalysisRegistrationCallback(
                [](llvm::ModuleAnalysisManager &MAM) {
                  MAM.registerPass([&] { return sysu::StaticCallCounter(); });
                });
          }};
}
}