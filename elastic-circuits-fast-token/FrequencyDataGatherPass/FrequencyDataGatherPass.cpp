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
struct FrequencyGatherDataPass : public llvm::FunctionPass {

    static char ID;

    ifstream mapping_file;
    map<string, string> function_to_file;

    map<string, int> bb_name_to_ID;

    map<pair<int, int>, int> edge_freqs;

    int previous_bb_count;

    FrequencyGatherDataPass() : FunctionPass(ID) {
        mapping_file.open("mapping.txt");
        if (!mapping_file.is_open()) {
            errs() << "could't open file mapping.txt\n";
            exit(0);
        }

        string line;
        while (getline(mapping_file, line)) {
            stringstream iss(line);
            string function_name;
            int function_number;
            iss >> function_name >> function_number;
            function_to_file[function_name] = to_string(function_number) + "_freq.txt";
        }

        previous_bb_count = 0;
        bb_name_to_ID.clear();
        edge_freqs.clear();
    }

    void map_bb_names_to_ID(Function& F) {
        bb_name_to_ID.clear();
        int BB_no = 1;
        for (auto& BB : F) {
            string name         = BB.getName().str() + "_" + F.getName().str();
            bb_name_to_ID[name] = BB_no;
            BB_no++;
        }
    }

    void init_edges(Function& F) {
        edge_freqs.clear();
        for (auto& BB : F) {
            for (auto& I : BB) {
                if (auto* op = dyn_cast<BranchInst>(&I)) {
                    string src_name = BB.getName().str() + "_" + F.getName().str();
                    int src_id      = bb_name_to_ID[src_name];
                    if (op->isConditional()) {
                        string dst_true_name =
                            op->getOperand(2)->getName().str() + "_" + F.getName().str();
                        string dst_false_name =
                            op->getOperand(1)->getName().str() + "_" + F.getName().str();

                        int dst_true_id  = bb_name_to_ID[dst_true_name];
                        int dst_false_id = bb_name_to_ID[dst_false_name];

                        edge_freqs[{src_id, dst_true_id}]  = 0;
                        edge_freqs[{src_id, dst_false_id}] = 0;
                    } else {
                        string dst_name =
                            op->getOperand(0)->getName().str() + "_" + F.getName().str();
                        int dst_id = bb_name_to_ID[dst_name];

                        edge_freqs[{src_id, dst_id}] = 0;
                    }
                }
            }
        }
    }

    void read_freq_from_file(string function_name) {
        if (function_to_file.find(function_name) == function_to_file.end()) {
            errs() << "don't know which file to read for function " << function_name << "\n";
            return;
        }

        ifstream file_freq(function_to_file[function_name].c_str());
        if (!file_freq)
            return;

        string line;
        while (getline(file_freq, line)) {
            stringstream iss(line);
            int src, dst, freq;
            iss >> src >> dst >> freq;
            if (edge_freqs.find({src, dst}) == edge_freqs.end()) {
                errs() << "there is no edge between " << src << " and " << dst << "\n";
                return;
            }
            edge_freqs[{src, dst}] = freq;
        }
    }

    void write_freq_to_file(string function_name) {
        string file_name = function_name + "_freq.txt";
        ofstream freq_file(file_name.c_str());
        if (!freq_file.is_open()) {
            errs() << "couldn't open " << file_name << "\n";
            return;
        }

        for (auto& edge : edge_freqs) {
            int src = edge.first.first, dst = edge.first.second, freq = edge.second;
            src += previous_bb_count;
            dst += previous_bb_count;
            string line =
                "block" + to_string(src) + " " + "block" + to_string(dst) + " " + to_string(freq);
            freq_file << line << endl;
        }
        freq_file.close();
    }

    void update_previous_bb_count(int function_bb_count) { previous_bb_count += function_bb_count; }

    virtual bool runOnFunction(Function& F) {

        map_bb_names_to_ID(F);

        init_edges(F);

        read_freq_from_file(F.getName().str());

        write_freq_to_file(F.getName().str());

        update_previous_bb_count(F.size());

        return false;
    }
};
} // namespace

char FrequencyGatherDataPass::ID = 0;

static void register_FrequencyGatherDataPass(const PassManagerBuilder&,
                                             legacy::PassManagerBase& PM) {
    PM.add(new FrequencyGatherDataPass());
}

static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                                             register_FrequencyGatherDataPass);