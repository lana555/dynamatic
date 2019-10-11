#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"

/*
 * Overall how this works:
 * 1. Add phi nodes correctly, following the data flow of the live-ins, and taking "false
 * connections" and "missing connections" into account.
 * 2. If the phi node is in the middle of the dataflow path, it certainly has a successor in the
 * next BB of the path; we hence mark it as a live-out. We mark last phi node in the dataflow path
 * only if it loops back to a previous BB in the path (ie. it has a successor in a different BB).
 */

// finds phi node corresponding to the `li` in BB corresponding to `bbnode`
// meant to be used only when only phis have been added till now
ENode* CircuitGenerator::find_phi(BBNode* bbnode, ENode* li) {
    for (auto& enode : *enode_dag) {
        if (enode->BB == bbnode->BB) { // for every enode in bbnode
            if (li->Instr != NULL) {
                if ((enode->type == Phi_n || enode->type == Phi_) && enode->Instr == li->Instr) {
                    return enode;
                } else if (enode->type == Phi_) {
                    for (auto& phiPred : *(enode->CntrlPreds)) {
                        if (phiPred->Instr == li->Instr) {
                            // return enode;
                        }
                    }
                }
            } else {
                if (enode->type == Phi_n && enode->A == li->A) {
                    return enode;
                }
            }
        }
    }

    return NULL;
}

void CircuitGenerator::printPath(std::vector<BBNode*>* path, FILE* f) {
    fprintf(f, "Path: ");
    for (auto& bbnode : *path) {
        fprintf(f, "%s -> ", bbnode->BB->getName().str().c_str());
    }
    fprintf(f, "\n");
}

void CircuitGenerator::printPaths(std::vector<std::vector<BBNode*>*>* paths, FILE* f) {
    fprintf(f, "printPaths:\n");
    for (auto& path : *paths) {
        printPath(path, f);
    }
}

// check if predecessor has already been added to CntrlPreds the appropriate number of times
// (avoids adding extra edges between two nodes)
bool containsAll(ENode* pred_node, ENode* enode) {
    int curr_count  = 0; // we count how many times we already added pred_node to preds
    int total_count = 1; // count we expect

    if (enode->type == Inst_)
        if (enode->Instr->getNumOperands() == 2)
            // check if inst has one node as pred 0 and pred 1
            if (enode->Instr->getOperand(0) == enode->Instr->getOperand(1))
                total_count++; // we expect an extra instance of pred_node in Preds

    for (auto& node : *enode->CntrlPreds)
        if (node == pred_node)
            curr_count++;

    return (curr_count == total_count);
}
//----------------------------------------------------------------------------

void CircuitGenerator::addPhi() {
    FILE* f = fopen(PRINT_FILE, "a+");
    fprintf(f, "\n//--------------- Adding Phi Nodes... ------------------//\n\n");
    fclose(f);

    // for every BB
    for (auto& bbnode : *bbnode_dag) {
        f = fopen(PRINT_FILE, "a+");
        fprintf(f, "\nEntering BB: **%s**\n", bbnode->BB->getName().str().c_str());
        fclose(f);

        // for every li of the BB
        for (auto& li : *(bbnode->Live_in)) {

            f = fopen(PRINT_FILE, "a+");
            fprintf(f, "\nCurrent livein: %s\n", getNodeDotNameNew(li).c_str());
            fclose(f);

            for (auto it = li->CntrlSuccs->begin(); it != li->CntrlSuccs->end();) {

                ENode* li_succ = *it;

                f = fopen(PRINT_FILE, "a+");
                fprintf(f, "Current livein succ: %s\n", getNodeDotNameNew(li_succ).c_str());
                fclose(f);

                if (li_succ->BB == bbnode->BB && // we need the `li_succ` to be in the same `bbnode`
                                                 // we are investigating
                    li_succ->type != Phi_n && // if the phi node for `li` has already been added, we
                                              // ignore the phi nodes
                    li_succ->type != Phi_ && // we treat LLVM phis the same as our elastic phi nodes
                    li->BB != li_succ->BB) { // we don't want phi nodes between `li` and successors
                                             // in its own BB

                    f = fopen(PRINT_FILE, "a+");
                    fprintf(f, "Entering IF for li_succ:\n");
                    fclose(f);

                    // we find all possible paths from which we can reach `bbnode` from the BB of
                    // `li` but we first need to find the BBNode corresponding to `li->BB`, `libb`.
                    BBNode* libb = findBB(li);

                    std::vector<std::vector<BBNode*>*>* paths =
                        new std::vector<std::vector<BBNode*>*>;
                    std::vector<BBNode*>* visited = new std::vector<BBNode*>;

                    resetDFS(bbnode_dag);
                    visited->push_back(libb);

                    // remember, `libb` is BB of `li`; (to, from) - (bbnode, libb)
                    findAllPaths(bbnode, libb, visited, paths, libb);

                    f = fopen(PRINT_FILE, "a+");
                    printPaths(paths, f);
                    fclose(f);

                    // for every path
                    for (auto& path : *paths) {
                        // printPath(path, f);
                        // we must propagate `li` through all paths possible wrt
                        // control flow using phi nodes (and elastic buffers later on)
                        f = fopen(PRINT_FILE, "a+");
                        fprintf(f, "Making phi dag...\n");
                        fclose(f);
                        makePhiDag(path, li);

                        // we join phi nodes together when we see loops in the
                        // control-flow graph of the BBs
                        // addPhiLoops(path, li); //Lana removed 19.10.2018

                        //-----------------------------------
                        // Lana 17.10.2018
                        // special case to add phis to backwards paths (loops) when li is not livein
                        // to pred block (but we need to connect it anyway to follow control flow)
                        // for instance, for control flow bb0-bb1-bb2-bb3-bb1, a var produced in bb0
                        // and used in bb2 requires a phi in bb3 as well to send the data back to
                        // the phi in bb1 The code below checks if there is a circular path to any
                        // phi on path and connects the blocks accordingly

                        for (auto startIt = std::begin(*path); startIt != std::end(*path);
                             ++startIt) { // for all bbs on path

                            BBNode* mybbnode = *startIt;

                            // if bb has multiple predecessors
                            // if (mybbnode->CntrlPreds->size() > 1 && li->type!= Phi_ && mybbnode
                            // != libb) {
                            if (mybbnode->CntrlPreds->size() > 1 && mybbnode != libb) {
                                for (auto& pred_bb : *mybbnode->CntrlPreds) {

                                    if (libb->BB != pred_bb->BB) { // bbnode has a predecessor which
                                                                   // is not our li origin
                                        std::vector<std::vector<BBNode*>*>* paths2 =
                                            new std::vector<std::vector<BBNode*>*>;
                                        std::vector<BBNode*>* visited2 = new std::vector<BBNode*>;

                                        resetDFS(bbnode_dag);
                                        visited2->push_back(libb);

                                        // find path from libb to predecessor bb
                                        findAllPaths(pred_bb, libb, visited2, paths2, libb);

                                        f = fopen(PRINT_FILE, "a+");
                                        fprintf(f, "----------- Adding new paths-------------\n");
                                        printPaths(paths2, f);
                                        fprintf(f, "------------------------\n");
                                        fclose(f);

                                        // for every path
                                        for (auto& path2 : *paths2) {
                                            // make phi dag on this path(s) as well, as usual
                                            f = fopen(PRINT_FILE, "a+");
                                            fprintf(f, "Making phi dag for new paths\n");
                                            fclose(f);
                                            makePhiDag(path2, li);
                                        }
                                    }
                                }
                            }
                        }
                        //----------------------------------
                    }

                    // disconnecting `li_succ`, ie. current iterator node, from `li`
                    removeNode(li_succ->CntrlPreds, li);
                    it = removeNode(li->CntrlSuccs, li_succ);
                } else if (li_succ->type == Phi_) {
                    f = fopen(PRINT_FILE, "a+");
                    fprintf(f, "Entering ELSE for li_succ (li_succ was phi):\n");
                    fclose(f);

                    // LLVM Phi nodes are a bit of a messed-up border case.
                    //
                    // Just because a node is connected to an LLVM Phi node,
                    // doesn't mean the connection is correct according to
                    // our elastic scheme; we have to fix them in the following situations.
                    //
                    // 1. `li` is generated in the same block as its LLVM Phi consumer.
                    //      a* Either it is a genuine connection, ie. there is a
                    //        self-loop in CFG as well and the BB does indeed
                    //        loop back to itself even in control flow, or;
                    //      b* The `li` is generated in the one block, but
                    //        flows in from another. Such a situation is as
                    //        follows (mentioned in `formLiveBBNodes()` in
                    //        `AddComp.cpp` as well):
                    //
                    //        Snippet 1:
                    //        ```
                    //        block1:                                           ; preds = %block4,
                    //        %block0
                    //          %i.0.in = phi i32 [ %n, %block0 ], [ %i.0, %block4 ]
                    //          %i.0 = add nsw i32 %i.0.in, -1
                    //          ...
                    //        block4:                                           ; preds = %block2
                    //          br label %block1
                    //        ```
                    //
                    // %i.0 is not generated in block4, yet it serves as the originating
                    // block of %i.0 in the phi node.
                    //
                    // We must fix this so that the input node corresponding to %i.0
                    // of the LLVM phi node comes from block4 of the elastic circuit,
                    // instead of block1 where it is generated.
                    //
                    // This situation can be generalized to: the `li` doesn't flow into
                    // its LLVM phi node in LLVM IR from the same block where `li` is generated.
                    //
                    // 2. Standard "false connection" case, where the LLVM Phi-predecessor
                    //    connection doesn't follow control-flow.
                    //      * This case is complicated in the following situation:
                    //
                    //      Snipper 2:
                    //      ```
                    //      define void
                    //      CircuitGenerator::CircuitGenerator::@_Z10triangularPiPA10_ii(i32* %x,
                    //      [10 x i32]* %A, i32 %n) #0
                    //        block0:
                    //          br label %block1

                    //        block1:                                           ; preds = %block4,
                    //        %block0
                    //          %i.0.in = phi i32 [ %n, %block0 ], [ %i.0, %block4 ]
                    //          %i.0 = add nsw i32 %i.0.in, -1
                    //          %"2" = icmp sgt i32 %i.0.in, 0
                    //          br i1 %"2", label %block2, label %block5
                    //
                    //        block2:                                           ; preds = %block3,
                    //        %block1
                    //          %k.0 = phi i32 [ %"19", %block3 ], [ %n, %block1 ]
                    //      ```
                    //      Notice that %n serves as an input to the LLVM phi nodes for %i.0.in
                    //      and %k.0, but the originating blocks for both are different, ie.
                    //      %n is inputed to %i.0.in via block0, and is inputed to %k.0 via
                    //      block1. (note that %n is generated in block0)
                    //
                    //      Control flow follows block0 -> block1 -> block2.
                    //
                    //      Hence we must disconnect %n in block0 from %k.0, and propagate
                    //      %n to block2 from block0 via an elastic Phi for %n in block1.
                    //
                    //      Lana 17.10.2018. Case 2 now handled correctly as well
                    //		- we now place independent phis in the same BB for the live-in
                    //		and have independent control flow paths for the two variables
                    //      origninating from the same input node

                    // We first check if `li` is indeed connected correctly to its LLVM Phi node.
                    bool isRemoved    = false;
                    llvm::PHINode* PI = dyn_cast<llvm::PHINode>(li_succ->Instr);
                    for (size_t i = 0; i < PI->getNumIncomingValues(); i++) {
                        // Lana 28.04.19 Added second term (argument) in 'if' clause
                        if (PI->getOperand(i) == li->Instr || PI->getOperand(i) == li->A) {
                            BasicBlock* llvmIncomingBlock = PI->getIncomingBlock(i);
                            // If not...
                            if (li->BB != llvmIncomingBlock) {
                                printf("Found false live-in!:\n");

                                // We disconnect the connection between the `li` and its LLVM Phi
                                // node. We'll do this later in [1].

                                // Then we construct the path of phi nodes propagating `li` to the
                                // BB which branches `li` to its LLVM Phi node, and connect it to
                                // the LLVM Phi node.

                                // we find all possible paths from which we can reach `bbnode` from
                                // the BB of `li` but we first need to find the BBNode corresponding
                                // to `li->BB`, `findbb`.
                                BBNode* llvmInBlockbb = NULL;
                                for (auto& llvmInBlockbbi : *bbnode_dag) {
                                    if (llvmInBlockbbi->BB == llvmIncomingBlock) {
                                        llvmInBlockbb = llvmInBlockbbi;
                                        break;
                                    }
                                }

                                // Lana 28.04.19. Find bb of live-in
                                BBNode* liBBnode = NULL;
                                for (auto& bb : *bbnode_dag) {
                                    if (bb->BB == li->BB) {
                                        liBBnode = bb;
                                        break;
                                    }
                                }

                                std::vector<std::vector<BBNode*>*>* paths =
                                    new std::vector<std::vector<BBNode*>*>;
                                std::vector<BBNode*>* visited = new std::vector<BBNode*>;

                                resetDFS(bbnode_dag);

                                // Lana 28.04.19.
                                // visited->push_back(bbnode);
                                visited->push_back(liBBnode);

                                // Lana 28.04.19.
                                // findAllPaths(llvmInBlockbb, bbnode, visited, paths, bbnode);
                                findAllPaths(llvmInBlockbb, liBBnode, visited, paths, liBBnode);

                                f = fopen(PRINT_FILE, "a+");
                                printf("from: %s\n", bbnode->BB->getName().str().c_str());
                                printPaths(paths, f);
                                fprintf(f, "Making phi dag...to: %s\n",
                                        llvmInBlockbb->BB->getName().str().c_str());
                                fclose(f);

                                // for every path
                                for (auto& path : *paths) {

                                    // we must propagate `li` through all paths possible wrt
                                    // control flow using phi nodes (and elastic buffers later on)

                                    makePhiDag(path, li);

                                    // we join phi nodes together when we see loops in the
                                    // control-flow graph of the BBs
                                    // f = fopen(PRINT_FILE, "a+");
                                    // fprintf(f, "Adding loops to phi dag...\n");
                                    // fclose(f);
                                    // addPhiLoops(path, li);
                                }

                                // We must now connect the elastic Phi node, `phi_llvmInBlock`, for
                                // `li` in `llvmIncomingBlock` to its LLVM phi in `bbnode`,
                                // `li_succ`.

                                ENode* phi_llvmInBlock = find_phi(llvmInBlockbb, li);
                                if (!contains(phi_llvmInBlock->CntrlSuccs, li_succ))
                                    phi_llvmInBlock->CntrlSuccs->push_back(li_succ);
                                if (!contains(li_succ->CntrlPreds, phi_llvmInBlock))
                                    li_succ->CntrlPreds->push_back(phi_llvmInBlock);

                                FILE* f = fopen(PRINT_FILE, "a+");
                                fprintf(f, "\n---- Connecting nodes in else:%s\n",
                                        getNodeDotNameNew(phi_llvmInBlock).c_str());
                                fprintf(f, " |\n\\_/\n%s", getNodeDotNameNew(li_succ).c_str());

                                if (li->type != Phi_) { // Lana: this is how it was before
                                    // [1]: Later meaning, now.
                                    removeNode(li_succ->CntrlPreds, li);
                                    it        = removeNode(li->CntrlSuccs, li_succ);
                                    isRemoved = true;

                                    fprintf(f, "\n---- Disconnecting nodes in else:%s",
                                            getNodeDotNameNew(li).c_str());
                                    fprintf(f, " |\n\\_/\n%s", getNodeDotNameNew(li_succ).c_str());
                                }

                                if (li->type == Phi_ &&
                                    !contains(findBB(li)->CntrlSuccs, findBB(li_succ))) {
                                    // Lana added this for Phi_-Phi_ connection-->if they are in
                                    // succ BBs, do not disconnect
                                    // [1]: Later meaning, now.
                                    removeNode(li_succ->CntrlPreds, li);
                                    it        = removeNode(li->CntrlSuccs, li_succ);
                                    isRemoved = true;

                                    fprintf(f, "\n---- Disconnecting nodes in else:%s",
                                            getNodeDotNameNew(li).c_str());
                                    fprintf(f, " |\n\\_/\n%s", getNodeDotNameNew(li_succ).c_str());
                                }

                                fclose(f);

                                break;
                            }
                        }
                    }
                    if (!isRemoved) {
                        ++it;
                    }
                } else {
                    ++it;
                }
            }
        }
    }

    f = fopen(PRINT_FILE, "a+");
    fprintf(f, "\n//--------------- Finished adding phi nodes ------------------//\n\n");
    fclose(f);

    setPhiLiveOuts();
}

void CircuitGenerator::setPhiLiveOuts() {

    for (auto& enode : *enode_dag) {
        if (enode->type == Phi_n || enode->type == Phi_) {
            BBNode* phiBB    = findBB(enode);
            enode->isMux     = false;
            enode->isCntrlMg = false;

            for (auto& succNode : *enode->CntrlSuccs)
                // phi has succ in different bb or loops back to its own bb to a phi
                if (findBB(succNode) != phiBB || succNode->type == Phi_n ||
                    succNode->type == Phi_) {
                    if (!contains(phiBB->Live_out, enode))
                        phiBB->Live_out->push_back(enode);
                    enode->is_live_out = true;
                }
        }
    }
}

void CircuitGenerator::makePhiDag(std::vector<BBNode*>* path, ENode* li) {

    FILE* f = fopen(PRINT_FILE, "a+");
    fprintf(f, "\n ========= Current LI in makePhiDag: %s ========= \n",
            getNodeDotNameNew(li).c_str());

    ENode* prevNode        = li;
    ENode* prevLLVMPhiNode = NULL;

    std::vector<ENode*> pathPhis;

    // we pick the next node because `li` is present in the first node of `path`;
    for (auto startIt = std::begin(*path) + 1; startIt != std::end(*path); ++startIt) {

        BBNode* bbnode = *startIt;
        fprintf(f, "Entering bb on path \n");

        bool found = false;

        ENode* phi_node = NULL;

        if (find_phi(bbnode, li) == NULL) {
            // if there was an llvm Phi on path, the relevant livein to bbnode is not li, but the
            // llvmphi we need to check if there is a phi already connected on the llvmphi path, if
            // yes do not add new phi
            if (prevLLVMPhiNode != li && prevLLVMPhiNode != NULL) {
                phi_node = find_phi(bbnode, prevLLVMPhiNode);
            }

            if (phi_node == NULL) {

                // check if there is an llvm phi in successor of bbnode that is on path
                // if so, check if bbnode already has a producer of value for the llvm phi
                // if there is a producer, no need to place phi in bbnode
                BBNode* bbnode2 = *(startIt + 1);
                ENode* nextPhi  = NULL;

                if (bbnode2 != NULL && (startIt + 1 != path->end())) {

                    // check for phi connected to li
                    if (find_phi(bbnode2, li) != NULL) {
                        nextPhi = find_phi(bbnode2, li);
                    }
                    // check for phi connected to last llvm phi we have seen on path
                    if (prevLLVMPhiNode != NULL)
                        if (find_phi(bbnode2, prevLLVMPhiNode) != NULL) {
                            nextPhi = find_phi(bbnode2, prevLLVMPhiNode);
                        }

                    if (nextPhi != NULL) {
                        if (nextPhi->type == Phi_)
                            for (auto& phi_pred : *(nextPhi->CntrlPreds)) {
                                if (findBB(phi_pred) == bbnode) {
                                    found = true;
                                    fprintf(f,
                                            "Found llvm phi in bb succ and its predecessor in "
                                            "bbnode %s\n",
                                            getNodeDotNameNew(nextPhi).c_str());

                                    // phi_pred->is_live_out = true;
                                }
                            }
                    }
                }

                if (!found) {
                    if (li->Instr != NULL)
                        phi_node = new ENode(Phi_n, "phi", li->Instr,
                                             bbnode->BB); // create new `phi_node` node
                    else
                        phi_node = new ENode(Phi_n, "phi", li->A, bbnode->BB);

                    phi_node->id = phi_id++;
                    enode_dag->push_back(phi_node);

                    fprintf(f, "Creating phi node: %s\n", getNodeDotNameNew(phi_node).c_str());
                }
            }

        } else {
            // there's already an existing phi node for `li` in `bbnode`
            phi_node = find_phi(bbnode, prevNode);

            // if the found node is llvm phi, remember it
            if (phi_node != NULL) {
                if (phi_node->type == Phi_) {
                    fprintf(f, "Found node: %s\n", getNodeDotNameNew(phi_node).c_str());
                    fprintf(f, "LLVMPHI\n");
                    prevLLVMPhiNode = phi_node;
                }
            }
        }

        if (phi_node != NULL) {
            pathPhis.push_back(phi_node);

            BBNode* fromBB = findBB(prevNode);
            BBNode* toBB   = findBB(phi_node);

            if (contains(fromBB->CntrlSuccs,
                         toBB)) { // extra test to ensure we are connecting nodes in direct succ bbs

                if (startIt + 1 != path->end()) {
                    // if `bbnode` is the last BB in `path`, we have no guarantee `phi_node` is a
                    // live-out until and unless we find it looping back to a previous BB in `path`
                    // we mark it only if we encounter a loop from it in `addPhiLoops`
                    // if (!contains(bbnode->Live_out, phi_node))
                    // bbnode->Live_out->push_back(phi_node); phi_node->is_live_out = true;
                }

                // if guards to prevent duplicate additions to lists
                if (!contains(prevNode->CntrlSuccs, phi_node)) {
                    prevNode->CntrlSuccs->push_back(phi_node);

                    fprintf(f, "\n---- Connecting nodes:%s\n", getNodeDotNameNew(prevNode).c_str());
                    fprintf(f, " |\n\\_/\n%s\n", getNodeDotNameNew(phi_node).c_str());
                }
                if (!contains(phi_node->CntrlPreds, prevNode))
                    phi_node->CntrlPreds->push_back(prevNode);

                prevNode = phi_node;

                if (startIt + 1 == path->end()) {
                    // last BB of the path, ie. the BB containing the "false connection" between
                    // `li_succ` and `li`
                    for (auto& li_succ : *(li->CntrlSuccs)) {
                        if (li_succ->type != Phi_n &&
                            li_succ->BB == bbnode->BB) { // we ignore all the phi nodes we've added
                            if (li_succ->type !=
                                Phi_) { // Lana: never connect phi to llvm phi in same bb

                                fprintf(f, "\n---- Connecting nodes:%s\n",
                                        getNodeDotNameNew(prevNode).c_str());
                                fprintf(f, " |\n\\_/\n%s\n", getNodeDotNameNew(li_succ).c_str());
                                // in this case, contains function is not enough because we need
                                // extra check for instruction
                                if (!containsAll(prevNode, li_succ)) {
                                    prevNode->CntrlSuccs->push_back(li_succ);
                                    li_succ->CntrlPreds->push_back(prevNode);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Lana 17.10.2018.
    // for circular paths
    // e.g., bb0-bb1-bb2-bb3-bb1: if bb3 doesn't have live in succ, still need to close loop in bb1
    for (auto& phi : pathPhis) { // traverse phis on path that we have just created
        BBNode* phiBB = findBB(phi);
        if (phiBB->CntrlPreds->size() >
            phi->CntrlPreds->size()) { // if phi has less predecessors than its bb has pred bbs
            for (auto& predBB : *phiBB->CntrlPreds) // traverse bb preds
                for (auto& predPhi : pathPhis)
                    if (findBB(predPhi) == predBB &&
                        predBB != findBB(li)) { // find corresponding phi in pred bb

                        fprintf(f, "\n---- Adding missing connections on circular path:%s\n",
                                getNodeDotNameNew(predPhi).c_str());
                        fprintf(f, " |\n\\_/\n%s\n", getNodeDotNameNew(phi).c_str());
                        // if phis not connected, add connection
                        if (!contains(predPhi->CntrlSuccs, phi))
                            predPhi->CntrlSuccs->push_back(phi);
                        if (!contains(phi->CntrlPreds, predPhi))
                            phi->CntrlPreds->push_back(predPhi);
                    }
        }
    }

    fprintf(f, "========= Exiting makePhiDag =========\n\n");
    fclose(f);
}

void CircuitGenerator::findAllPaths(BBNode* to, BBNode* from, std::vector<BBNode*>* visited,
                                    std::vector<std::vector<BBNode*>*>* paths, BBNode* currNode) {

    FILE* f = fopen(PRINT_FILE, "a+");
    fprintf(f, "In findAllPath\n");
    fclose(f);

    if (currNode == to) {
        std::vector<BBNode*>* viscpy = cpyList(visited);
        paths->push_back(viscpy); // we've reached `to` and we add `visited` onto `paths`, we copy
                                  // it because `visited` will soon be modified
    }

    std::vector<BBNode*>* nodes =
        currNode->CntrlSuccs; // adjacent nodes of the last node of `visited`

    for (auto& node : *nodes) {
        if (!contains(visited, node)) {
            visited->push_back(node);
            findAllPaths(to, from, visited, paths, node);
        } else if (node == to) { // some node ahead of `to` has a backedge to `to`
            std::vector<BBNode*>* viscpy = cpyList(visited);
            paths->push_back(viscpy); // we've reached `to` and we add `visited` onto `paths`, we
                                      // copy it because `visited` will soon be modified
        }
    }

    removeNode(visited, currNode);
}
