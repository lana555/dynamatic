#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

using namespace llvm;
using namespace std;

namespace {
struct FrequencyCounterPass : public llvm::FunctionPass {
    static char ID;

    static const string init_log_name;
    static const string cond_log_name;
    static const string uncond_log_name;
    static const string report_log_name;

    int func_num = 0;
    map<string, int> func_name_to_ID;
    map<string, int> BB_name_to_ID;

    ofstream mapping_file;

    FrequencyCounterPass() : FunctionPass(ID) {
        mapping_file.open("mapping.txt");
        assert(mapping_file.is_open());
    }

    void add_function_mapping(string function_name) {
        func_name_to_ID.insert({function_name, func_num});
        mapping_file << function_name << " " << func_num << "\n";
        func_num++;
    }

    void add_function_BB_mapping(Function& F) {
        int BB_no = 1;
        for (auto& BB : F) {
            string name = BB.getName().str() + "_" + F.getName().str();
            BB_name_to_ID.insert({name, BB_no});
            BB_no++;
        }
    }

    virtual bool runOnFunction(Function& F) {
        // adding the function mapping to file for next stage
        string func_name = F.getName().str();
        add_function_mapping(func_name);

        // define the functions to insert in file
        LLVMContext& context = F.getContext();
        Constant* init_func =
            F.getParent()->getOrInsertFunction(init_log_name, Type::getVoidTy(context),
                                               Type::getInt32Ty(context), // function ID
                                               Type::getInt32Ty(context)  // number of BBs
            );
        Constant* cond_func =
            F.getParent()->getOrInsertFunction(cond_log_name, Type::getVoidTy(context),
                                               Type::getInt32Ty(context), // function ID
                                               Type::getInt1Ty(context),  // condition
                                               Type::getInt32Ty(context), // source
                                               Type::getInt32Ty(context), // true destination
                                               Type::getInt32Ty(context)  // false destination
            );
        Constant* uncond_func =
            F.getParent()->getOrInsertFunction(uncond_log_name, Type::getVoidTy(context),
                                               Type::getInt32Ty(context), // function ID
                                               Type::getInt32Ty(context), // source
                                               Type::getInt32Ty(context)  // destination
            );
        Constant* report_func =
            F.getParent()->getOrInsertFunction(report_log_name, Type::getVoidTy(context));

        // adding the BB mapping so as to achieve BB IDs
        add_function_BB_mapping(F);

        bool flag = false;
        for (auto& BB : F) {
            // get BB and func IDs
            string BB_name = BB.getName().str() + "_" + func_name;
            assert(func_name_to_ID.find(func_name) != func_name_to_ID.end());
            assert(BB_name_to_ID.find(BB_name) != BB_name_to_ID.end());
            int func_ID = func_name_to_ID[func_name], BB_ID = BB_name_to_ID[BB_name];

            // insert the initiation function (allocate space for frequencies)
            if (BB_ID == 1) {
                IRBuilder<> init_builder(&BB);
                init_builder.SetInsertPoint(BB.getFirstNonPHI());

                Value* init_args[] = {init_builder.getInt32(func_ID),
                                      init_builder.getInt32(F.size())};

                init_builder.CreateCall(init_func, init_args);
            }

            for (auto& I : BB) {
                if (auto* op = dyn_cast<BranchInst>(&I)) {
                    IRBuilder<> builder(op);
                    builder.SetInsertPoint(&BB, builder.GetInsertPoint());

                    if (op->isConditional()) {
                        string BB_dst_true_name =
                            op->getOperand(2)->getName().str() + "_" + func_name;
                        string BB_dst_false_name =
                            op->getOperand(1)->getName().str() + "_" + func_name;

                        Value* cond      = op->getOperand(0);
                        Value* src       = builder.getInt32(BB_ID);
                        Value* dst_true  = builder.getInt32(BB_name_to_ID[BB_dst_true_name]);
                        Value* dst_false = builder.getInt32(BB_name_to_ID[BB_dst_false_name]);

                        Value* args[] = {builder.getInt32(func_ID), cond, src, dst_true, dst_false};

                        builder.CreateCall(cond_func, args);
                    } else {
                        string BB_dst_name = op->getOperand(0)->getName().str() + "_" + func_name;

                        Value* src = builder.getInt32(BB_ID);
                        Value* dst = builder.getInt32(BB_name_to_ID[BB_dst_name]);

                        Value* args[] = {builder.getInt32(func_ID), src, dst};

                        builder.CreateCall(uncond_func, args);
                    }
                    flag = true;
                }
            }
        }

        // write the frequency results to file only at the end of main (i.e. the end of the program)
        if (func_name == "main") {
            for (auto& BB : F) {
                string BB_name = BB.getName().str() + "_" + func_name;
                if (BB_name_to_ID[BB_name] == F.size()) {
                    IRBuilder<> report_builder(&BB);
                    report_builder.SetInsertPoint(BB.getTerminator());

                    report_builder.CreateCall(report_func);
                }
            }
        }

        return flag;
    }
};
} // namespace

const string FrequencyCounterPass::init_log_name   = "init";
const string FrequencyCounterPass::cond_log_name   = "cond";
const string FrequencyCounterPass::uncond_log_name = "uncond";
const string FrequencyCounterPass::report_log_name = "report";

char FrequencyCounterPass::ID = 0;

static void register_FrequencyCounterPass(const PassManagerBuilder&, legacy::PassManagerBase& PM) {
    PM.add(new FrequencyCounterPass());
}

static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                                             register_FrequencyCounterPass);