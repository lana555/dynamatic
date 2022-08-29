#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "OptimizeBitwidth/OptimizeBitwidth.h"

/*
 * Author: Radhika Ghosal
 */

//----------------------------------------------------------------------------

void CircuitGenerator::phiSanityCheck(std::vector<ENode*>* enode_dag) {
    // Lana 17.10. 2018
    // detailed check of phi placement
    // check if phis are connected according to control flow
    // function can be used at any point after addPhi()
    // std::cout << "phiSanityCheck\n";
    for (auto& enode : *enode_dag) {
        // std::cout << enode->Name << enode->id<<"\n";
        if ((enode->type == Phi_n || enode->type == Phi_) && enode->isMux == false) {
            BBNode* phiBB = findBB(enode);
            // phi needs to have as many inputs as there are predecessor BBs

            assert(enode->CntrlPreds->size() == phiBB->CntrlPreds->size());

            // each pred bb has to contain a predecessor of phi
            for (auto& predBB : *phiBB->CntrlPreds) {
                bool found = false;
                for (auto& predNode : *enode->CntrlPreds) {
                    BBNode* predNodeBB = findBB(predNode);
                    if (predNodeBB == predBB)
                        found = true;
                }
                assert(found == true);
            }

            // phi should always have successors
            assert(enode->CntrlSuccs->size() > 0);
        }

        // same should hold for control phis (justCntrlPreds instead of CtrlPreds)
        if (enode->type == Phi_c && enode->isCntrlMg == false) {
            BBNode* phiBB = findBB(enode);
            // phi needs to have as many inputs as there are predecessor BBs
            assert(enode->JustCntrlPreds->size() == phiBB->CntrlPreds->size());

            // each pred bb has to contain a predecessor of phi
            for (auto& predBB : *phiBB->CntrlPreds) {
                bool found = false;
                for (auto& predNode : *enode->JustCntrlPreds) {
                    BBNode* predNodeBB = findBB(predNode);
                    if (predNodeBB == predBB)
                        found = true;
                }
                assert(found == true);
            }

            // phi should always have successors
            assert(enode->JustCntrlSuccs->size() > 0);
        }
    }
}

void CircuitGenerator::forkSanityCheck(std::vector<ENode*>* enode_dag) {
    // Lana 19.10.2018
    // detailed check of fork placement
    // function can be used at any point after addFork()

    // std::cout << "forkSanityCheck\n";
    for (auto& enode : *enode_dag) {

        if (enode->type == Fork_) {
            assert(enode->CntrlPreds->size() == 1);
            assert(enode->CntrlSuccs->size() >= 1);
            BBNode* forkBB = findBB(enode);

            // predecessor must be in same BB
            assert(findBB(enode->CntrlPreds->front()) == forkBB);

            // successors need to be in same BB or in its direct successors (before we insert
            // branches)
            for (auto& succNode : *enode->CntrlSuccs)
                assert(contains(forkBB->CntrlSuccs, findBB(succNode)) ||
                       findBB(succNode) == forkBB);
        }
    }
}

void CircuitGenerator::branchSanityCheck(std::vector<ENode*>* enode_dag) {
    // Lana 19.10.2018
    // detailed check of branch placement
    // function can be used at any point after addBranch()

    // std::cout << "branchSanityCheck\n";
    for (auto& enode : *enode_dag) {

        if (enode->type == Branch_n) {
            BBNode* branchBB = findBB(enode);
            // always data and condition
            assert(enode->CntrlPreds->size() == 2);
            // always has successor(s), up to bb successor count
            assert(enode->CntrlSuccs->size() >
                   0); // && (enode->CntrlSuccs->size() <= branchBB->CntrlSuccs->size()));

            for (auto& predNode : *enode->CntrlPreds)
                assert(findBB(predNode) == branchBB);
            for (auto& succNode : *enode->CntrlSuccs) {
                if (succNode->type != Sink_) {
                    assert(contains(branchBB->CntrlSuccs, findBB(succNode)));
                    assert(succNode->type == Phi_n || succNode->type == Phi_ ||
                           succNode->type == Buffera_ || succNode->type == Bufferi_);
                }
            }
        }

        // at this point, forks should have successors only in same bb
        if (enode->type == Fork_) {
            // assert (enode->CntrlSuccs->size() > 1);
            BBNode* forkBB = findBB(enode);
            // successors need to be in same BB or in its direct successors (before we insert
            // branches)
            for (auto& succNode : *enode->CntrlSuccs)
                assert(findBB(succNode) == forkBB);
        }

        // at this point, phi should have a single successor in same bb
        if (enode->type == Phi_n || enode->type == Phi_) {
            BBNode* phiBB = findBB(enode);
            assert(enode->CntrlSuccs->size() == 1);

            for (auto& succNode : *enode->CntrlSuccs)
                assert(findBB(succNode) == phiBB);
        }
    }
}

void CircuitGenerator::instructionSanityCheck(std::vector<ENode*>* enode_dag) {
    // Lana 20.10.2018
    // detailed check of commonly used instructions
    // function can be used at any point after addBranch()

    // std::cout << "instructionSanityCheck\n";

    for (auto& enode : *enode_dag) {
        if (enode->type == Inst_) {
            // std::cout << enode->Name << enode->id<<"\n";
            if (enode->Instr->getOpcode() == Instruction::Load) {
                assert(enode->CntrlPreds->size() == 1);
                // assert(enode->CntrlSuccs->size() == 1);
            }

            else if (enode->Instr->getOpcode() == Instruction::Ret) {
                assert(enode->CntrlPreds->size() >= 0);
                // assert(enode->CntrlSuccs->size() == 0);
            } else if (enode->Instr->getOpcode() == Instruction::Add ||
                       enode->Instr->getOpcode() == Instruction::FAdd ||
                       enode->Instr->getOpcode() == Instruction::Sub ||
                       enode->Instr->getOpcode() == Instruction::FSub ||
                       enode->Instr->getOpcode() == Instruction::Mul ||
                       enode->Instr->getOpcode() == Instruction::FMul ||
                       enode->Instr->getOpcode() == Instruction::UDiv ||
                       enode->Instr->getOpcode() == Instruction::SDiv ||
                       enode->Instr->getOpcode() == Instruction::FDiv ||
                       enode->Instr->getOpcode() == Instruction::URem ||
                       enode->Instr->getOpcode() == Instruction::SRem ||
                       enode->Instr->getOpcode() == Instruction::FRem ||
                       enode->Instr->getOpcode() == Instruction::Shl ||
                       enode->Instr->getOpcode() == Instruction::LShr ||
                       enode->Instr->getOpcode() == Instruction::AShr ||
                       enode->Instr->getOpcode() == Instruction::And ||
                       enode->Instr->getOpcode() == Instruction::Or ||
                       enode->Instr->getOpcode() == Instruction::Xor ||
                       enode->Instr->getOpcode() == Instruction::ICmp ||
                       enode->Instr->getOpcode() == Instruction::FCmp ||
                       enode->Instr->getOpcode() ==
                           Instruction::Store) { // output to sink or to control
                assert(enode->CntrlPreds->size() == 2);
                // assert(enode->CntrlSuccs->size() == 1);
            } else if (enode->Instr->getOpcode() == Instruction::Select) {
                assert(enode->CntrlPreds->size() == 3);
                assert(enode->CntrlSuccs->size() == 1);
            } else if (enode->Instr->getOpcode() == Instruction::Call) {
                assert(enode->CntrlPreds->size() >= 0);
                assert(enode->CntrlSuccs->size() >= 0);
            } else {
                assert(enode->CntrlPreds->size() > 0);
                assert(enode->CntrlSuccs->size() > 0);
            }
        }
    }
}

void CircuitGenerator::sanityCheckVanilla(std::vector<ENode*>* enode_dag) {
    // This function sanity-checks every Enode in `enode_dag`
    // for certain properties it must satisfy for correctness
    // after any transformation in `enode_dag`.
    // Expect to modify this function every time a new
    // transformation is done in `enode_dag`, because
    // properties required for correctness may change after
    // any transformation.
    // This current function is valid only for a naively-created
    // elastic circuit with no optimizations or removed nodes,
    // hence it is suffixed with a 'Vanilla'.

    // std::cout << "----------------------------------------" << std::endl;
    // std::cout << "Checking the elastic circuit for correctness..." << std::endl;

    // repeat detailed sanity checks, most of it is checked below as well but just in case

    phiSanityCheck(enode_dag);
    forkSanityCheck(enode_dag);
    branchSanityCheck(enode_dag);
    instructionSanityCheck(enode_dag);

    for (auto& enode : *enode_dag) {
        // printNode(enode, stdout);

        if (enode->type == Inst_) {

            // select has 3, SingleOp instructions have 1, rest have 2
            // assert(instEnode->CntrlPreds->size() <= 3);
            // assert(instEnode->CntrlSuccs->size() <= 1);
        } else if (enode->type == Argument_) {
            ENode* argEnode = enode;

            assert(argEnode->CntrlPreds->size() == 0);

            assert(argEnode->CntrlSuccs->size() <= 1); // has zero (arg unused) or a single
                                                       // successor

            if (argEnode->CntrlSuccs->size() == 1) {
                ENode* argSucc = argEnode->CntrlSuccs->front();
                assert(argSucc->type == Branch_n || argSucc->type == Inst_ ||
                       argSucc->type == Fork_ || argSucc->type == Bufferi_ ||
                       argSucc->type == Buffera_);
            }
        } else if (enode->type == Cst_) {
            ENode* cstEnode = enode;

            assert(cstEnode->CntrlPreds->size() == 1 || cstEnode->JustCntrlPreds->size() == 1);

            assert(cstEnode->CntrlSuccs->size() == 1 || cstEnode->JustCntrlSuccs->size() == 1);
        } else if (enode->type == Branch_) {
            ENode* brEnode = enode;

            // can be unconditional branch with no predecessor as well
            assert(brEnode->CntrlPreds->size() <= 1);

            // always followed by elastic Branch node or elastic Fork
            // assert(brEnode->CntrlSuccs->size() == 1 || brEnode->JustCntrlSuccs->size() == 1);
            // //llvm Branch can have regular or control successor Lana 20.10.2018. inequality can
            // also hold when no control nodes
            assert(brEnode->CntrlSuccs->size() <= 1 || brEnode->JustCntrlSuccs->size() <= 1);

            if (brEnode->CntrlSuccs->size() == 1) {
                ENode* brSucc = brEnode->CntrlSuccs->front();
                assert(brSucc->type == Branch_n || brSucc->type == Fork_ ||
                       brSucc->type == Bufferi_ || brSucc->type == Buffera_);
            } else if (brEnode->JustCntrlSuccs->size() == 1) {
                ENode* brSucc = brEnode->JustCntrlSuccs->front();
                assert(brSucc->type == Phi_c || brSucc->type == Buffera_ ||
                       brSucc->type == Bufferi_ || brSucc->type == Branch_c);
            }
        } else if (enode->type == Buffera_ || enode->type == Bufferi_) {
            ENode* buffEnode = enode;

            // A buffer can only have a single phi/buff node as a predecessor
            assert(buffEnode->CntrlPreds->size() == 1 || buffEnode->JustCntrlPreds->size() == 1);

            ENode* buffPred;
            if (buffEnode->CntrlPreds->size() == 1) {
                buffPred = buffEnode->CntrlPreds->front();
            } else {
                buffPred = buffEnode->JustCntrlPreds->front();
            }

            assert(buffPred->type == Phi_n || buffPred->type == Phi_ || buffPred->type == Fork_ ||
                   buffPred->type == Branch_n || buffPred->type == Inst_ ||
                   buffPred->type == Argument_ || buffPred->type == Fifoa_ ||
                   buffPred->type == Fifoi_ || buffPred->type == Branch_ ||
                   buffPred->type == Fork_c || buffPred->type == Branch_c ||
                   buffPred->type == Phi_c);

            // A buffer can only have a single successor
            assert(buffEnode->CntrlSuccs->size() == 1 || buffEnode->JustCntrlSuccs->size() == 1);

            ENode* buffSucc;
            if (buffEnode->CntrlSuccs->size() == 1) {
                buffSucc = buffEnode->CntrlSuccs->front();
            } else {
                buffSucc = buffEnode->JustCntrlSuccs->front();
            }

            // std::cout<<getNodeDotName(buffSucc)<<std::endl;

            assert(buffSucc->type == Phi_n || buffSucc->type == Phi_ || buffSucc->type == Fork_ ||
                   buffSucc->type == Branch_n || buffSucc->type == Inst_ ||
                   buffSucc->type == Argument_ || buffSucc->type == Fifoa_ ||
                   buffSucc->type == Fifoi_ || buffSucc->type == Branch_ ||
                   buffSucc->type == Fork_c || buffSucc->type == Branch_c ||
                   buffSucc->type == Phi_c || buffSucc->type == Sink_);
        } else if (enode->type == Phi_n || enode->type == Phi_) {
            ENode* mergeEnode = enode;

            // Merge nodes are always followed by a single node
            assert(mergeEnode->CntrlSuccs->size() == 1);

            for (auto& mergePred : *(mergeEnode->CntrlPreds)) {
                // predecessors come from different basic blocks,
                // hence are elastic Branch nodes, or are constant values
                assert(mergePred->type == Branch_n || mergePred->type == Cst_ ||
                       mergePred->type == Bufferi_ || mergePred->type == Buffera_ ||
                       mergePred->type == Fork_ || mergePred->type == Phi_c);
            }
        } else if (enode->type == Phi_c) {
            ENode* mergeCEnode = enode;

            assert(mergeCEnode->JustCntrlSuccs->size() == 1);

            ENode* mergeCSucc = mergeCEnode->JustCntrlSuccs->front();
            assert(mergeCSucc->type == Fork_c || mergeCSucc->type == Buffera_ ||
                   mergeCSucc->type == Bufferi_ || mergeCSucc->type == Branch_c ||
                   mergeCSucc->type == End_ || mergeCSucc->type == Sink_ ||
                   mergeCSucc->type == Inst_);

            for (auto& mergeCPred : *(mergeCEnode->JustCntrlPreds)) {
                // predecessors come from different basic blocks, hence are
                // elastic Branch nodes, or are constant values
                assert(mergeCPred->type == Branch_c || mergeCPred->type == Buffera_ ||
                       mergeCPred->type == Bufferi_);
            }
        } else if (enode->type == Fork_) {
            ENode* forkEnode = enode;

            // A Fork node can only have a single predecessor
            assert(forkEnode->CntrlPreds->size() == 1);
            ENode* forkPred = forkEnode->CntrlPreds->front();

            assert(forkPred->type == Buffera_ || forkPred->type == Bufferi_ ||
                   forkPred->type == Inst_ || forkPred->type == Branch_ ||
                   forkPred->type == Argument_ || forkPred->type == Phi_ ||
                   forkPred->type == Phi_n || forkPred->type == Phi_c);

            assert(forkEnode->CntrlSuccs->size() + forkEnode->JustCntrlSuccs->size() >
                   1); // at this point, no single-succ forks
            for (auto& forkSucc : *(forkEnode->CntrlSuccs)) {
                assert(forkSucc->type == Branch_n || forkSucc->type == Inst_ ||
                       forkSucc->type == Buffera_ || forkSucc->type == Bufferi_ ||
                       forkSucc->type == Phi_ || forkSucc->type == Phi_n ||
                       forkSucc->type == Fifoa_ || forkSucc->type == Fifoi_);
            }
        } else if (enode->type == Fork_c) {
            ENode* forkCEnode = enode;

            // A Fork node can only have a single predecessor
            assert(forkCEnode->JustCntrlPreds->size() == 1);
            ENode* forkCPred = forkCEnode->JustCntrlPreds->front();

            assert(forkCPred->type == Phi_c || forkCPred->type == Start_ ||
                   forkCPred->type == Buffera_ || forkCPred->type == Bufferi_);

            assert(forkCEnode->JustCntrlSuccs->size() >=
                   1); // ok to have fork with single sucessor; we can just remove later
            for (auto& forkCSucc : *(forkCEnode->JustCntrlSuccs)) {
                assert(forkCSucc->type == Branch_c || forkCSucc->type == Branch_ ||
                       forkCSucc->type == LSQ_ || forkCSucc->Name.compare("ret") == 0 ||
                       forkCSucc->type == Buffera_ || forkCSucc->type == Bufferi_ ||
                       forkCSucc->type == Cst_ || forkCSucc->type == Sink_);
            }
        } else if (enode->type == Branch_n) {
            ENode* branchEnode = enode;

            // A branch can only have <= 2 phi/merge successors
            assert(branchEnode->CntrlSuccs->size() <= 2 && branchEnode->CntrlSuccs->size() > 0);

            // A branch can only have 2 predecessors, the branch data and
            // the branch condition
            assert(branchEnode->CntrlPreds->size() == 2);

            for (auto& branchSucc : *(branchEnode->CntrlSuccs)) {
                assert(branchSucc->type == Phi_n || branchSucc->type == Phi_ ||
                       branchSucc->type == Buffera_ || branchSucc->type == Bufferi_ ||
                       branchSucc->type == Sink_);
            }
        } else if (enode->type == Branch_c) {
            ENode* branchCEnode = enode;

            // A branchC can only have <= 2 phi/merge successors
            assert(branchCEnode->JustCntrlSuccs->size() <= 2);
            assert(branchCEnode->JustCntrlSuccs->size() > 0);

            // A branchC can only have 2 predecessors, the branchC data and
            // the branchC condition
            assert(branchCEnode->JustCntrlPreds->size() == 2);

            for (auto& branchCSucc : *(branchCEnode->JustCntrlSuccs)) {
                assert(branchCSucc->type == Phi_c || branchCSucc->type == Buffera_ ||
                       branchCSucc->type == Bufferi_ || branchCSucc->type == Sink_);
            }
        }
    }

    // std::cout << "Done sanity checking." << std::endl;
    // std::cout << "----------------------------------------" << std::endl;
}
