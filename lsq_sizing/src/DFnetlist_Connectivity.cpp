#include <algorithm>
#include <cassert>
#include <stack>
#include "DFnetlist.h"

using namespace Dataflow;
using namespace std;

void DFnetlist_Impl::DFS(bool forward, bool onlyMarked)
{
    // Vector to detect the blocks that have been visited
    // or have been processed.
    vector<bool> visited(blocks.size());

    // All the blocks are not visited, except those that are marked
    ForAllBlocks(b) visited[b] = onlyMarked and not isBlockMarked(b);

    // Visiting order of blocks (first: entry nodes). This is necessary
    // to properly identify the back edges. DFS must start from the
    // entry nodes (see theory in Compilers book)
    listBlocks lBlocks;
    int clock = 0;
    ForAllBlocks(b) {
        setDFSorder(b, -1);
        if (visited[b]) continue;
        if (getBlockType(b) == FUNC_ENTRY) lBlocks.push_front(b);
        else lBlocks.push_back(b);
        ++clock;
    }

    // Now we start an iterative DFS
    DFSorder = vector<blockID>(clock);
    stack<blockID> S, listChildren;

    for (blockID b: lBlocks) {
        if (visited[b]) continue;
        S.push(b);
        visited[b] = true;

        // Put the children to the pending list. The lists of children
        // from different blocks are separated by an invalidDataflowID
        listChildren.push(invalidDataflowID);
        const setPorts& ports = getPorts(b, (forward ? OUTPUT_PORTS : INPUT_PORTS));
        for (portID p: ports) {
            channelID c = getConnectedChannel(p);
            if (onlyMarked and not isChannelMarked(c)) continue; // Skip if the channel is not marked
            blockID other_b = getBlockFromPort(getConnectedPort(p));
            if (not visited[other_b]) listChildren.push(other_b);
        }

        while (not S.empty()) {
            blockID kid = listChildren.top();
            listChildren.pop();
            if (kid == invalidDataflowID) {
                blockID v = S.top();
                S.pop();
                setDFSorder(v, --clock);
            } else {
                if (visited[kid]) continue;
                S.push(kid);
                visited[kid] = true;
                listChildren.push(invalidDataflowID);
                const setPorts& ports = getPorts(kid, (forward ? OUTPUT_PORTS : INPUT_PORTS));
                for (portID p: ports) {
                    channelID c = getConnectedChannel(p);
                    if (onlyMarked and not isChannelMarked(c)) continue; // Skip if the channel is not marked
                    blockID other_b = getBlockFromPort(getConnectedPort(p));
                    if (not visited[other_b]) listChildren.push(other_b);
                }
            }
        }
    }

    assert(clock == 0);

    ForAllBlocks(b) {
        int i = getDFSorder(b);
        assert((onlyMarked and not isBlockMarked(b)) or (i < DFSorder.size()));
        if (i >= 0) DFSorder[i] = b;
    }

}

void DFnetlist_Impl::computeSCC(bool onlyMarked)
{
    DFS(false, onlyMarked); // DFS with the reverse graph

    SCC.clear();

    vector<bool> visited(blocks.size());
    ForAllBlocks(b) {
        visited[b] = onlyMarked and not isBlockMarked(b);
        setDFSorder(b, -1);
    }

    stack<blockID> s;

    // Visit the nodes in DFS post-visit order
    // We add channels to the SCCs (the blocks are implicit)
    // Notice that SCCs require at least one channel.

    for (blockID b: DFSorder) {

        if (visited[b]) continue;

        // A new component
        SCC.push_back(subNetlist());
        s.push(b);
        visited[b] = true;
        while (not s.empty()) {
            blockID v = s.top();
            s.pop();
            ForAllOutputPorts(v, p) {
                channelID c = getConnectedChannel(p);
                if (onlyMarked and not isChannelMarked(c)) continue; // Skip if the channel is not marked
                blockID other_b = getBlockFromPort(getConnectedPort(p));
                if (not visited[other_b]) {
                    visited[other_b] = true;
                    s.push(other_b);
                    SCC.back().insertChannel(*this, c);   // We add the channel to the SCC
                }
            }
        }

        // If no channels, remove the SCC
        if (SCC.back().empty()) SCC.pop_back();
    }

    // Sort the SCCs according to their size (largest first)
    sort(SCC.begin(), SCC.end(),
    [](const subNetlist& s1, const subNetlist& s2) {
        return s1.numBlocks() > s2.numBlocks();
    });

    // Assign SCC numbers to all blocks
    ForAllBlocks(b) blocks[b].scc_number = -1;
    for (int n = 0; n < SCC.size(); ++n) {
        for (blockID b: SCC[n].getBlocks()) blocks[b].scc_number = n;
    }

    // Revisit all channels and add all those that may also be inside the SCCs and have not been visited
    ForAllChannels(c) {
        if (onlyMarked and not isChannelMarked(c)) continue; // Skip if the channel is not marked
        blockID src = getSrcBlock(c);
        int scc = blocks[src].scc_number;
        if (scc < 0) continue; // Not in an SCC
        blockID dst = getDstBlock(c);
        if (scc == blocks[dst].scc_number) SCC[scc].insertChannel(*this, c);
    }
}

void DFnetlist_Impl::eraseNonSCC()
{
    if (SCC.empty()) computeSCC();

    vecBlocks b_to_remove;
    ForAllBlocks(b) {
        if (blocks[b].scc_number < 0) b_to_remove.push_back(b);
    }

    for (auto b: b_to_remove) removeBlock(b);

    vecChannels c_to_remove;
    ForAllChannels(c) {
        blockID src = getSrcBlock(c);
        blockID dst = getDstBlock(c);
        if (blocks[src].scc_number != blocks[dst].scc_number) c_to_remove.push_back(c);
    }

    for (auto c: c_to_remove) removeChannel(c);

    invalidateBasicBlocks();
}

void DFnetlist_Impl::calculateBackEdges()
{
    ForAllChannels(c) {

        blockID bb = getSrcBlock(c);
        bbID src = getBasicBlock(getSrcBlock(c));
        bbID dst = getBasicBlock(getDstBlock(c));
        // BB ids are given in DFS order from entry
        // If branch goes to its own BB or a BB with a smaller id
        // it is a back edge
        // TODO: determine back edges at CFG level and then apply to CDFG
        if ((getBlockType(bb) == BRANCH) && src >= dst)
            setBackEdge(c, true);
        else
            setBackEdge(c, false); 
    }

    // Forward traversal starting from entry nodes.
    // DFS();

    // Check for back edges
    //ForAllChannels(c) {
    //    int src = getDFSorder(getSrcBlock(c));
    //    int dst = getDFSorder(getDstBlock(c));
    //    if (src < 0 or dst < 0) setBackEdge(c, false);
    //    else setBackEdge(c, src > dst);
    //}
}

void DFnetlist_Impl::printBlockSCCs() {
    for (blockID i = 0; i < numBlocks(); i++) {
        cout << getBlockName(i) << " belongs to SCC " << getBlockSCCno(i) << endl;
    }
}