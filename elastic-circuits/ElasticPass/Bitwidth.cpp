#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "OptimizeBitwidth/OptimizeBitwidth.h"

void CircuitGenerator::setSizes() {

    for (auto& cur_node : *enode_dag) {
        std::string instrName = cur_node->Name;
        if (cur_node->type == Inst_) {
            if (instrName == "ret") {
            }
            // `switch`
            else if (instrName == "switch") {
            } else if (instrName == "add" || instrName == "sub" || instrName == "mul" ||
                       instrName == "fadd") {
                createSizes(cur_node, "alu");
            } else if (instrName == "and" || instrName == "or" || instrName == "xor") {
                createSizes(cur_node, "alu");
            } else if (instrName == "alloca") {
            } else if (instrName == "load") {
                createSizes(cur_node, "r");
            } else if (instrName == "store") {
                createSizes(cur_node, "w");
            } else if (instrName == "sext" || instrName == "zext") {
                createSizes(cur_node, "singleop");
            } else if (instrName == "icmp") {
                createSizes(cur_node, "alu");
            }
            // `select`
            else if (instrName == "select") {
            }
            // `call`
            else if (instrName == "call") {
            } else if (instrName == "getelementptr") {
                createSizes(cur_node, "gep");
            }

        } else {
            createSizes(cur_node);
        }
    }
}

Value* getPred(ENode* e, int num) {
    Value* arg = nullptr;
    if (e->CntrlPreds->at(num)->Instr != nullptr) {
        arg = dyn_cast<Value>(e->CntrlPreds->at(num)->Instr);
    } else if (e->CntrlPreds->at(num)->A != nullptr) {
        arg = dyn_cast<Value>(e->CntrlPreds->at(num)->A);
    } else if (e->CntrlPreds->at(num)->CI != nullptr) {
        arg = dyn_cast<Value>(e->CntrlPreds->at(num)->CI);
    } else if (e->CntrlPreds->at(num)->CF != nullptr) {
        arg = dyn_cast<Value>(e->CntrlPreds->at(num)->CF);
    }
    return arg;
}

Value* getRes(ENode* e) {
    Value* res = nullptr;
    if (e->Instr != nullptr) {
        res = dyn_cast<Value>(e->Instr);
    } else if (e->A != nullptr) {
        res = dyn_cast<Value>(e->A);
    } else if (e->CI != nullptr) {
        res = dyn_cast<Value>(e->CI);
    } else if (e->CF != nullptr) {
        res = dyn_cast<Value>(e->CF);
    }
    return res;
}

Value* getOperand(ENode* e, int num) {
    Instruction* I = dyn_cast<Instruction>(e->Instr);
    return I->getOperand(num);
}

void CircuitGenerator::createSizes(ENode* e, std::string inst_type) {

    switch (e->type) {
        /*
         * first size - size of result
         * the rest sizes - sizes of the inputs
         */
        case Phi_: {
            assert(e->CntrlPreds->size() >= 1);
            assert(e->CntrlSuccs->size() >= 1);
            assert(e->CntrlPreds->at(0)->Instr != nullptr || e->CntrlPreds->at(0)->A != nullptr ||
                   e->CntrlPreds->at(0)->type == Cst_ ||
                   e->CntrlPreds->at(0)->CntrlPreds->at(0)->type == Cst_);
            assert(e->Instr != nullptr || e->A != nullptr || e->type == Cst_);

            Value* res = getRes(e);

            unsigned size = (res == nullptr) ? DATA_SIZE : OB->getSize(res);
            e->sizes->push_back(size); // only one size
            for (size_t i = 0; i < e->CntrlPreds->size(); i++) {
                e->sizes->push_back((getPred(e, i) == nullptr) ? DATA_SIZE
                                                               : OB->getSize(getPred(e, i)));
            }
            break;
        }
        case Phi_n: {
            assert(e->CntrlPreds->size() >= 1);
            assert(e->CntrlSuccs->size() >= 1);
            assert(e->CntrlPreds->at(0)->Instr != nullptr || e->CntrlPreds->at(0)->A != nullptr ||
                   e->CntrlPreds->at(0)->type == Cst_);
            assert(e->Instr != nullptr || e->A != nullptr || e->type == Cst_);

            Value* res    = getRes(e);
            unsigned size = (res == nullptr) ? DATA_SIZE : OB->getSize(res);
            e->sizes->push_back(size); // only one size
            for (size_t i = 0; i < e->CntrlPreds->size(); i++) {
                e->sizes->push_back((getPred(e, i) == nullptr) ? DATA_SIZE
                                                               : OB->getSize(getPred(e, i)));
            }
            break;
        }
        case Phi_c: {
            for (size_t i = 0; i < e->JustCntrlPreds->size(); i++) {
                e->sizes->push_back(1); // dummy 1-bit data line for control-only nodes
            }
            break;
        }
        /*
         * only one size - size of Value in buffer
         */
        case Buffera_: {
            assert(e->CntrlPreds->size() == 1);
            assert(e->CntrlPreds->at(0)->Instr != nullptr || e->CntrlPreds->at(0)->A != nullptr ||
                   e->CntrlPreds->at(0)->type == Cst_);
            assert(e->Instr != nullptr || e->A != nullptr || e->type == Cst_);

            Value* res    = getRes(e);
            unsigned size = (res == nullptr) ? DATA_SIZE : OB->getSize(res);
            e->sizes->push_back(size); // only one size
            break;
        }
        case Bufferi_: {
            assert(e->CntrlPreds->size() == 1);
            assert(e->CntrlPreds->at(0)->Instr != nullptr || e->CntrlPreds->at(0)->A != nullptr ||
                   e->CntrlPreds->at(0)->type == Cst_);
            assert(e->Instr != nullptr || e->A != nullptr || e->type == Cst_);

            Value* res    = getRes(e);
            unsigned size = (res == nullptr) ? DATA_SIZE : OB->getSize(res);
            e->sizes->push_back(size); // only one size
            break;
        }
        /*
         * only one size - size which fork is duplicating
         */
        case Fork_: {
            assert(e->CntrlPreds->size() == 1);
            //             assert(e->CntrlSuccs->size() > 1);
            assert(e->CntrlPreds->at(0)->Instr != nullptr || e->CntrlPreds->at(0)->A != nullptr ||
                   e->CntrlPreds->at(0)->type == Cst_);
            assert(e->Instr != nullptr || e->A != nullptr || e->CI != nullptr ||
                   e->type == Cst_); // Lana 19.10.2-18

            Value* res = getRes(e);
            if (e->CntrlPreds->at(0)->Instr != nullptr) {
                Instruction* I = dyn_cast<Instruction>(e->CntrlPreds->at(0)->Instr);
                if (isa<BranchInst>(dyn_cast<Value>(I)) && (I->getNumOperands() <= 1)) {
                    // this will be always true "1" constant, we don't even need to propagate it.
                    unsigned size = 1;
                    e->sizes->push_back(size);
                    break;
                }
            }
            unsigned size = (res == nullptr) ? DATA_SIZE : OB->getSize(res);
            e->sizes->push_back(size); // only one size
            break;
        }
        case Fork_c: {
            e->sizes->push_back(1);
            break;
        }
        /*
         * only one size - size which branch is propagating
         */
        case Branch_n: {
            //          assert(e->CntrlPreds->size() >= 1);
            //         assert(e->CntrlPreds->at(0)->Instr != nullptr || e->CntrlPreds->at(0)->A !=
            //         nullptr || e->CntrlPreds->at(0)->type == Cst_);
            //          assert(e->Instr != nullptr || e->A != nullptr || e->type == Cst_);

            Value* res    = getRes(e);
            unsigned size = (res == nullptr) ? DATA_SIZE : OB->getSize(res);
            e->sizes->push_back(size);
            break;
        }
        case Branch_c: {
            e->sizes->push_back(1); // dummy 1-bit data line for control-only nodes
            break;
        }
        case Branch_: {
            //          assert(e->CntrlPreds->size() >= 1);
            //          assert(e->CntrlPreds->at(0)->Instr != nullptr || e->CntrlPreds->at(0)->A !=
            //          nullptr || e->CntrlPreds->at(0)->type == Cst_); assert(e->Instr != nullptr
            //          || e->A != nullptr || e->type == Cst_);

            Value* res    = getRes(e);
            unsigned size = (res == nullptr) ? DATA_SIZE : OB->getSize(res);
            e->sizes->push_back(size);
            break;
        }
        case Cst_: {
            Value* res    = getRes(e);
            unsigned size = (res == nullptr) ? DATA_SIZE : OB->getSize(res);
            e->sizes->push_back(size);
            break;
        }
        /*
         * alu:
         * 	  two first sizes - two operands;
         *    third - result
         *
         * singleop:
         * 	  first size - operands
         *    second size - result
         * r, w - sizes of data
         */
        case Inst_: {
            if (inst_type == "alu") {
                assert(e->CntrlPreds->size() == 2);
                assert(e->CntrlSuccs->size() == 1);
                assert(e->CntrlPreds->at(0)->Instr != nullptr ||
                       e->CntrlPreds->at(0)->A != nullptr || e->CntrlPreds->at(0)->type == Cst_);
                assert(e->CntrlPreds->at(1)->Instr != nullptr ||
                       e->CntrlPreds->at(1)->A != nullptr || e->CntrlPreds->at(1)->type == Cst_);
                assert(e->Instr != nullptr || e->A != nullptr);

                Value* arg0 = getOperand(e, 0);
                Value* arg1 = getOperand(e, 1);

                Value* res = getRes(e);

                e->sizes->push_back(OB->getSize(arg0));
                e->sizes->push_back(OB->getSize(arg1));
                e->sizes->push_back(OB->getSize(res));

            } else if (inst_type == "singleop") {

                assert(e->CntrlPreds->size() == 1);
                assert(e->CntrlSuccs->size() == 1);
                assert(e->A != nullptr || e->Instr != nullptr);
                assert(e->CntrlPreds->at(0)->A != nullptr ||
                       e->CntrlPreds->at(0)->Instr != nullptr ||
                       e->CntrlPreds->at(0)->type == Cst_);

                Value* arg0 = getOperand(e, 0);
                Value* res  = getRes(e);

                unsigned size1 = (arg0 == nullptr) ? DATA_SIZE : OB->getSize(arg0);
                unsigned size2 = (res == nullptr) ? DATA_SIZE : OB->getSize(res);
                e->sizes->push_back(size1);
                e->sizes->push_back(size2);

            } else if (inst_type == "r") {
                // load - we first push the result
                e->sizes->push_back((getRes(e) == nullptr) ? DATA_SIZE : OB->getSize(getRes(e)));
                // we then push the size of address
                e->sizes->push_back(OB->getSize(getOperand(e, 0)));
            } else if (inst_type == "w") {
                // store - we first push the size of the input
                e->sizes->push_back(OB->getSize(getOperand(e, 0)));
                // we then push the size of address
                e->sizes->push_back(OB->getSize(getOperand(e, 1)));
            } else if (inst_type == "gep") {
                // we push the resut first
                e->sizes->push_back(OB->getSize(e->Instr));
                // we then push size of all operands
                for (int i = 0; i < e->CntrlPreds->size(); i++) {
                    e->sizes->push_back(OB->getSize(getPred(e, i)));
                }
            }
            break;
        }
        default:
            break;
    }
}
