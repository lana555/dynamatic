#include <cassert>
#include <stack>
#include "DFnetlist.h"
/*
 * This file contains the methods that calculate Basic Blocks (BB).
 * Every block is tagged with a BB number. It is assumed that each
 * netlist has only one entry and one exit point. By using flow
 * information, the BBs are inferred assuming that the merge and
 * branch blocks are at the boundaries of the BBs.
 *
 * A disjoint-set data structure (similar to the one of Kruskal's algorithm)
 * is used to represent the BBs. Initially, each instruction is assumed
 * to be in a different BB. The neighboring information of each blocks
 * is used to merge BBs.
 *
 * Note: the netlist is assumed not to have Demuxes (TBD).
 */

using namespace Dataflow;

void DFnetlist_Impl::invalidateBasicBlocks()
{
    BBG.clear();
    ForAllBlocks(b) setBasicBlock(b);
}

void DFnetlist_Impl::makeDisjointBB(blockID b)
{
    blocks[b].bbParent = b;
    blocks[b].bbRank = 0;
}

blockID DFnetlist_Impl::findDisjointBB(blockID b)
{
    blockID& parent = blocks[b].bbParent;
    if (b != parent) parent = findDisjointBB(parent);
    return parent;
}

bool DFnetlist_Impl::unionDisjointBB(blockID b1, blockID b2)
{
    blockID r1 = findDisjointBB(b1);
    blockID r2 = findDisjointBB(b2);
    if (r1 == r2) return false;
    if (blocks[r1].bbRank > blocks[r2].bbRank) blocks[r2].bbParent = r1;
    else {
        blocks[r1].bbParent = r2;
        if (blocks[r1].bbRank == blocks[r2].bbRank) ++blocks[r2].bbRank;
    }
    return true;
}

bool DFnetlist_Impl::calculateBasicBlocks()
{
    invalidateBasicBlocks();

    // Initial assertion: there are no demuxes (TBD)
    // Initialize the disjoint sets
    ForAllBlocks(b) {
        assert(getBlockType(b) != DEMUX);
        makeDisjointBB(b);
    }

    // All the entry blocks belong to the same BB.
    // The same for the exit blocks.
    blockID entry_repr = invalidDataflowID;
    //blockID exit_repr = invalidDataflowID;
    ForAllBlocks(b) {
        BlockType type = getBlockType(b);
        if (type == FUNC_ENTRY) {
            if (not validBlock(entry_repr)) entry_repr = b;
            else unionDisjointBB(entry_repr, b);
        } /*else if (type == FUNC_EXIT) {
            if (not validBlock(exit_repr)) exit_repr = b;
            else unionDisjointBB(exit_repr, b);
        }*/
    }

    // Propagate information (fixpoint algorithm)
    bool changes;
    do {
        changes = false;
        // Visit all blocks and propagate BB information.
        // The propagation is stopped by src branch blocks and dst merge blocks.
        // A merge to a merge considers that both merges are in the same BB.
        // For the moment we will ignore demuxes (to be done)
        ForAllChannels(c) {
            blockID src = getSrcBlock(c);
            BlockType type_src = getBlockType(src);
            if (type_src == BRANCH) continue;
            blockID dst = getDstBlock(c);
            if (getBlockType(dst) == MERGE and type_src != MERGE and type_src != ELASTIC_BUFFER) continue;
            changes = changes or unionDisjointBB(src, dst);
        }
    } while (changes);

    setBlocks repr; // Set of representative blocks
    ForAllBlocks(b) {
        if (b == findDisjointBB(b)) repr.insert(b);
    }

    // Visit all the representative BBs and assign a BB number
    map<blockID, bbID> block2bb;
    BBG.clear();
    for (auto b: repr) {
        bbID bb = BBG.createBasicBlock();
        block2bb[b] = bb++;
    }

    // Assign BB number to each block
    ForAllBlocks(b) setBasicBlock(b, block2bb[findDisjointBB(b)]);

    // Check only the control channels that go from branches to merges
    ForAllChannels(c) {
        if (not isControlPort(getSrcPort(c))) continue;
        blockID src = getSrcBlock(c);
        BlockType type_src = getBlockType(src);
        bbID bb_src = getBasicBlock(src);
        blockID dst = getDstBlock(c);
        BlockType type_dst = getBlockType(dst);
        bbID bb_dst = getBasicBlock(dst);

        if ((bb_src != bb_dst) or
            (type_src == BRANCH and type_dst == MERGE)) {
            // External channel or self-loop
            BBG.findOrAddArc(bb_src, bb_dst);
        }
    }

    // Find the entry point
    entryBB = block2bb[findDisjointBB(entryControl)];
    BBG.setEntryBasicBlock(entryBB);
    BBG.calculateBackArcs();

    // Define all back arcs in the netlist using the BB information.
    // All back arcs in the BB graph are translated into back arcs in
    // the netlist.
    ForAllChannels(c) {
        blockID src = getSrcBlock(c);
        bbID bb_src = getBasicBlock(src);
        blockID dst = getDstBlock(c);
        bbID bb_dst = getBasicBlock(dst);

        bbArcID a = BBG.findArc(bb_src, bb_dst);
        if (a == invalidDataflowID or not BBG.isBackArc(a)) {
            setBackEdge(c, false);
            continue;
        }

        // Now we have a back arc in the BB graph. It is only a
        // back edge in the netlist if the BBs are different or
        // the edge goes from branch to merge
        BlockType type_src = getBlockType(src);
        BlockType type_dst = getBlockType(dst);
        setBackEdge(c, (bb_src != bb_dst) or (type_src == BRANCH and type_dst == MERGE));
    }

    // And now calculate the BB frequencies
    BBG.calculateBasicBlockFrequencies();
    return true;
}

void DFnetlist_Impl::calculateDisjointCFDFCs() {
    cout << "Calculating disjoint sets of CFDFCs..." << endl;

    // main part of DSU is the next two loops

    cout << "\tinitialize" << endl;
    // initialize
    for (int i = 0; i < CFDFC.size(); i++) {
        CFDFC[i].setDSUParent(i);
        CFDFC[i].setDSURank(0);
    }

    cout << "\tunion" << endl;
    // join sets that have a CFDFC in common
    for (int i = 0; i < CFDFC.size(); i++) {
        for (int j = 0; j < CFDFC.size(); j++) {
            if (j == i)
                continue;
            if (CFDFC[i].haveCommonBasicBlock(CFDFC[j])) {
                unionCFDFC(i, j);
            }
        }
    }

    cout << "\tget set of unique dsu numbers" << endl;
    // finalize the DSU number and get all unique DSU numbers
    set<int> DSU_numbers;
    for (int i = 0; i < CFDFC.size(); i++) {
        CFDFC[i].setDSUnumber(findCFDFC(i));
        DSU_numbers.insert(CFDFC[i].getDSUnumber());
    }

    cout << "\tmerge and get sub-components" << endl;
    // merge all the CFDFCs which belong in the same DSU
    CFDFC_disjoint = vector<subNetlistBB>(DSU_numbers.size(), subNetlistBB());
    components = vector< vector<int> >(DSU_numbers.size(), vector<int>());
    for (int i = 0; i < CFDFC.size(); i++) {
        int curr_dsu = CFDFC[i].getDSUnumber();

        // find the dsu number in the set
        int index = distance(DSU_numbers.begin(), DSU_numbers.find(curr_dsu));

        CFDFC_disjoint[index].merge(CFDFC[i]);
        components[index].push_back(i);
    }

    for (int i = 0; i < CFDFC_disjoint.size(); i++) {
        for (bbArcID arc: CFDFC_disjoint[i].getBasicBlockArcs()) {
            BBG.setDSUnumberArc(arc, i + 1);
        }
    }

    cout << "Total number of Extracted Disjoint CFDFCs: " << CFDFC_disjoint.size() << endl;
}

int DFnetlist_Impl::findCFDFC(int i) {
    int parent_i = CFDFC[i].getDSUParent();
    if (parent_i != i) {
        CFDFC[i].setDSUParent(findCFDFC(parent_i));
    }
    return CFDFC[i].getDSUParent();
}

void DFnetlist_Impl::unionCFDFC(int x, int y) {
    int xset = findCFDFC(x), yset = findCFDFC(y);

    if (xset == yset)
        return;

    // xset must be the CFDFC with the highest rank. if not, swap.
    if (CFDFC[xset].getDSURank() < CFDFC[yset].getDSURank()) {
        int temp = xset;
        xset = yset;
        yset = temp;
    }

    CFDFC[yset].setDSUParent(xset);

    if (CFDFC[xset].getDSURank() == CFDFC[yset].getDSURank())
        CFDFC[xset].increaseDSURank();
}

bbArcID BasicBlockGraph::findArc(bbID src, bbID dst) const
{
    if (src <= 0 || dst <= 0) return invalidDataflowID;

    for (bbArcID a: BBs[src - 1].succ) if (Arcs[a].dst == dst) return a;
    return invalidDataflowID;
}

bbArcID BasicBlockGraph::findOrAddArc(bbID src, bbID dst, double freq)
{
    // Check if it already exists
    bbArcID id = findArc(src, dst);
    if (id != invalidDataflowID) return id;

    // Create the arc
    id = Arcs.size();
    Arcs.push_back(BB_Arc {src, dst, false, -1, id, freq});
    BBs[src - 1].succ.insert(id);
    BBs[dst - 1].pred.insert(id);
    return id;
}

bool BasicBlockGraph::calculateBackArcs()
{
    assert(entryBB != invalidDataflowID);

    stack<bbID> S, listKids;
    int numbb = numBasicBlocks();
    vector<int> pre(numbb, 0);
    vector<int> post(numbb, 0);

    bbID entry = getEntryBasicBlock();
    S.push(entry);
    pre[entry] = 1;
    listKids.push(invalidDataflowID);
    for(bbArcID arc: successors(entryBB)) {
        bbID bb = getDstBB(arc);
        if (pre[bb] == 0) listKids.push(bb);
    }

    int time = 2;
    while (not S.empty()) {
        bbID v = S.top();
        bbID kid = listKids.top();
        listKids.pop();
        if (kid == invalidDataflowID) {
            S.pop();
            post[v] = time++;;
        } else {
            if (pre[kid] > 0) continue;
            S.push(kid);
            pre[kid] = time++;
            listKids.push(invalidDataflowID);
            for(bbArcID arc: successors(kid)) {
                bbID bb = getDstBB(arc);
                if (pre[bb] == 0) listKids.push(bb);
            }
        }
    }

    // Visit all BBs and calculate back edges
    // based on pre/post numbers
    ForAllBasicBlocks(u) {
        if (pre[u] == 0) continue;
        for (bbArcID arc: successors(u)) {
            bbID v = getDstBB(arc);
            if (pre[v] <= pre[u] and post[v] >= post[u]) setBackArc(arc);
        }
    }

    return true;
}

void BasicBlockGraph::setDefaultProbabilities(double back_prob)
{

    // Calculate the probabilities of the arcs
    ForAllBasicBlocks(bb) {
        int nsucc = 0, nback = 0;
        for (bbArcID arc: successors(bb)) {
            ++nsucc;
            if (isBackArc(arc)) ++nback;
        }

        // We consider two cases:
        // If there is a back edge, this gets prob = 0.9.
        // If not, all the edges have the same probability.
        double prob_fwd, prob_back;
        if (nback != 1) {
            prob_fwd = prob_back = 1.0/nsucc;
        } else {
            prob_back = back_prob;
            prob_fwd = (1-back_prob)/(nsucc-1);
        }

        for (bbArcID arc: successors(bb)) {
            setProbability(arc, isBackArc(arc) ? prob_back : prob_fwd);
        }
    }
}

bool BasicBlockGraph::calculateBasicBlockFrequencies(double back_prob)
{
    // If the frequency of the entry BB is defined, nothing to do
    if (getFrequency(entryBB) > 0) return true;

    // Check that the probabilities at the arcs are well defined
    bool correct = true;
    ForAllBasicBlocks(bb) {
        if (successors(bb).size() == 0) continue;
        double prob = 0;
        for (bbArcID arc: successors(bb)) {
            prob += getProbability(arc);
        }
        if (prob < 0.99 or prob > 1.01) {
            correct = false;
            break;
        }
    }

    // Define the default probabilities
    if (not correct) setDefaultProbabilities(back_prob);

    // We solve the Markov chain using LP.
    // We will have one equality for each state, plus
    // a constraint indicating that the frequency of
    // the entry point is 1.

    Milp_Model M;
    int numBB = numBasicBlocks();
    vector<int> bbVar(numBB);
    for (int& v: bbVar) v = M.newRealVar();

    // Flow equations
    ForAllBasicBlocks(bb) {

        // Special case for the entry point: entry = 1
        if (bb == entryBB) {
            M.newRow( {{1, bbVar[bb]}}, '=', 1);
            continue;
        }

        // Equation dst = p1*src1 + p2*src2 + ...
        int row = M.newRow( {{-1, bbVar[bb]}}, '=', 0);

        for (bbArcID arc: predecessors(bb)) {
            bbID src = getSrcBB(arc);
            M.newTerm(row, getProbability(arc), bbVar[src]);
        }
    }

    // Now we have the LP model ready. Let's solve it
    M.solve();

    ForAllBasicBlocks(bb) {
        setFrequency(bb, M[bbVar[bb]]);
        cout << "Freq BB " << bb << " = " << getFrequency(bb) << endl;
    }

    return true;
}

void BasicBlockGraph::DFS() {

    // Vector to detect the blocks that have been visited
    // or have been processed.
    vector<bool> visited(numBasicBlocks(), false);

    // Visiting order of blocks (first: entry nodes). This is necessary
    // to properly identify the back edges. DFS must start from the
    // entry nodes (see theory in Compilers book)
    list<bbID> lBB;
    int clock = 0;
    for (bbID bb = 1; bb <= numBasicBlocks(); bb++) {
        setDFSorder(bb, -1);
        if (visited[bb]) continue;
        if (isEntryBB(bb)) lBB.push_front(bb);
        else lBB.push_back(bb);
        clock++;
    }

    // Now we start an iterative DFS
    DFSorder = vector<bbID>(clock);
    stack<bbID> S, listChildren;
    for (bbID bb: lBB) {
        if (visited[bb]) continue;
        S.push(bb);
        visited[bb] = true;

        // Put the children to the pending list. The lists of children
        // from different blocks are separated by an invalidDataflowID
        listChildren.push(invalidDataflowID);
        for (bbArcID succ: successors(bb)) {
            bbID kid = getDstBB(succ);
            if (not visited[kid]) listChildren.push(kid);
        }

        while (not S.empty()) {
            bbID kid = listChildren.top();
            listChildren.pop();
            if (kid == invalidDataflowID) {
                bbID v = S.top();
                S.pop();
                setDFSorder(v, --clock);
            } else {
                if (visited[kid]) continue;
                S.push(kid);
                visited[kid] = true;
                listChildren.push(invalidDataflowID);
                for (bbArcID succ: successors(kid)) {
                    bbID grand_kid = getDstBB(succ);
                    if (not visited[grand_kid]) listChildren.push(grand_kid);
                }
            }
        }
    }

    assert(clock == 0);

    for (bbID bb = 1; bb <= numBasicBlocks(); bb++) {
        int i = getDFSorder(bb);
        assert(i < DFSorder.size());
        if (i >= 0) DFSorder[i] = bb;
    }

/*    cout << "TESTING DFS RESULTS" << endl;
    cout << "-------------------" << endl;
    for (bbID order: DFSorder)
        cout << order << " -> ";
    cout << endl;*/
}

void BasicBlockGraph::computeSCC() {

    // set DFSorder of BBs
    DFS();


    vector<bool> visited(numBasicBlocks(), false);
/*
    for (bbID bb = 1; bb <= numBasicBlocks(); bb++) {
        setDFSorder(bb, -1);
    }
*/

    // Visit the nodes in DFS post-visit order
    // We add channels to the SCCs (the blocks are implicit)
    // Notice that SCCs require at least one channel.
    stack<bbID> s;
    int SCC_no = 0;
    for (bbID bb: DFSorder) {
        if (visited[bb]) continue;

        // A new component
        setSCCnumber(bb, ++SCC_no);
        s.push(bb);
        visited[bb] = true;
        while (not s.empty()) {
            bbID v = s.top();
            s.pop();
            for (bbArcID pred: predecessors(v)) {
                bbID parent = getSrcBB(pred);
                if (not visited[parent]) {
                    visited[parent] = true;
                    s.push(parent);
                    setSCCnumber(parent, SCC_no);
                }
            }
        }
    }

/*    cout << "TESTING SCC RESULTS" << endl;
    cout << "-------------------" << endl;
    for (bbID i = 1; i <= numBasicBlocks(); i++) {
        cout << "\tBB" << i << " belongs to SCC" << getSCCnumber(i) << endl;
    }*/
}