#include <cassert>
#include "DFnetlist.h"

using namespace Dataflow;

double BasicBlockGraph::extractBasicBlockCycles(double coverage)
{
    bool extractOnlyOne = coverage < 0;
    double execTime = 0.0;

    ForAllBasicBlocks(bb) {
        BBs[bb].residual_freq = getFrequency(bb);
        execTime += getFrequency(bb)*getExecTime(bb);
    }

    double coveredTime = 0;

    while (coveredTime < coverage*execTime) {
        BasicBlockCycle C = extractBasicBlockCycle();
        if (C.exec == 0.0) break;
        cycles.push_back(C);
        coveredTime += C.exec;

        // Update the residual graph
        for (int j = 0; j < C.cycle.size(); ++j) {
            if (C.executed[j]) BBs[C.cycle[j]].residual_freq -= C.freq;
        }

        cout << "Cycle: ";
        for (unsigned int i = 0; i < C.cycle.size(); ++i) {
            cout << " " << char('a'+C.cycle[i]);
            if (C.executed[i]) cout << '*';
        }
        cout << endl;
        cout << "  Freq = " << C.freq << endl;
        cout << "  Exec = " << C.exec << endl;
        cout << "  Coverage = " << coveredTime/execTime << endl;

        if (extractOnlyOne) break;
    }

    return coveredTime/execTime;
}

BasicBlockGraph::BasicBlockCycle BasicBlockGraph::extractBasicBlockCycle()
{

    int nBBs = numBasicBlocks();

    // Let us calculate the max execution frequency of all blocks
    double maxFreq = 0;
    ForAllBasicBlocks(bb) {
        maxFreq = max(maxFreq, BBs[bb].residual_freq);
    }

    // Boolean variables for the presence and execution of blocks
    vector<int> presenceBB(nBBs);   // Presence of each block
    vector<int> execBB(nBBs);       // Execution of each block

    // Boolean variables representing arcs of the BBs
    vector<vector<int>> arcBB(nBBs, vector<int>(nBBs, -1));

    // Variables to represent the product Freq*execBB
    vector<int> FreqExec(nBBs);

    Milp_Model M; // The MILP model

    // Execution frequency of the cycle
    int Freq = M.newRealVar("", 0, maxFreq);

    // Total execution time of the cycle
    int Exec = M.newRealVar();

    // Variable that accumulates the total number of BBs in the cycle
    int totalBBs = M.newRealVar();

    // Let us create the variables for the cycle
    ForAllBasicBlocks(bb) {
        FreqExec[bb] = M.newRealVar();
        presenceBB[bb] = M.newBooleanVar();
        execBB[bb] = M.newBooleanVar();
        for (bbArcID arc: successors(bb)) arcBB[bb][getDstBB(arc)] = M.newBooleanVar();
    }

    // Constraints to model cycles
    vector<int> sum;

    ForAllBasicBlocks(bb) {
        // execBB => presenceBB
        int varBB = presenceBB[bb];
        M.implies(execBB[bb], {varBB});

        // Sum of predecessors = presence
        sum.clear();
        for (bbArcID arc: predecessors(bb)) sum.push_back(arcBB[getSrcBB(arc)][bb]);
        M.equalSum( {varBB}, sum);

        // Sum of successors = presence
        sum.clear();
        for (bbArcID arc: successors(bb)) sum.push_back(arcBB[bb][getDstBB(arc)]);
        M.equalSum( {varBB}, sum);
    }

    // Constraints to model the equality FreqExec[bb] = Freq*execBB[bb]
    ForAllBasicBlocks(bb) {
        M.newRow( {{1,FreqExec[bb]},{-1,Freq}}, '<', 0);
        M.newRow( {{1,FreqExec[bb]},{-maxFreq,execBB[bb]}}, '<', 0);
        M.newRow( {{1,FreqExec[bb]},{-1,Freq},{-maxFreq,execBB[bb]}}, '>', -maxFreq);
    }

    // Constraints to bound the execution frequency
    ForAllBasicBlocks(bb) M.newRow( {{1,FreqExec[bb]}}, '<', BBs[bb].residual_freq);

    // Exec = SUM FreqExec*Exectime
    int row_exec = M.newRow('=', 0);
    M.newTerm(row_exec, -1, Exec);
    ForAllBasicBlocks(bb) M.newTerm(row_exec, getExecTime(bb), FreqExec[bb]);

    // totalBBs = SUM presenceBB
    M.equalSum(presenceBB, {totalBBs});

    M.newCostTerm(1, Exec);
    M.newCostTerm(-0.01, totalBBs); // To minimize the number of inactive blocks
    M.setMaximize();

    M.solve();

    // There can be multiple disjoint cycles. Let us take the best one.
    // This vector is to keep track of the BBs already extracted
    vector<bool> used(nBBs, false);

    // Here we accumulate the cycles.
    // A cycle is a sequence of BBs
    vector<vecBBs> cycles;

    ForAllBasicBlocks(bb) {
        if (used[bb] or M.isFalse(presenceBB[bb])) continue;
        // We start a new cycle
        cycles.push_back(vecBBs());
        bbID src = bb;
        while (not used[src]) {
            cycles.back().push_back(src);
            used[src] = true;
            // Find the next one
            for (bbArcID arc: successors(src)) {
                bbID dst = getDstBB(arc);
                if (M.isTrue(arcBB[src][dst])) {
                    src = dst;
                    break;
                }
            }

        }
    }

    // We now calculate the frequency and execution time
    vector<double> CycFreq(nBBs);
    vector<double> CycExec(nBBs);

    for (int c = 0; c < cycles.size(); ++c) {
        double freq = maxFreq;
        for (bbID bb: cycles[c]) {
            if (M.isTrue(execBB[bb])) freq = min(freq, BBs[bb].residual_freq);
        }
        CycFreq[c] = freq;
        double execTime = 0;
        for (bbID bb: cycles[c]) {
            if (M.isTrue(execBB[bb])) execTime += freq*getExecTime(bb);
        }
        CycExec[c] = execTime;
    }

    if (cycles.size() == 0) return BasicBlockCycle {0,0};

    int maxExec = 0;
    for (int c = 1; c < CycExec.size(); ++c) {
        if (CycExec[c] > CycExec[maxExec]) maxExec = c;
    }

    BasicBlockCycle C;
    C.freq = CycFreq[maxExec];
    C.exec = CycExec[maxExec];
    for (bbID bb: cycles[maxExec]) {
        C.cycle.push_back(bb);
        C.executed.push_back(M.isTrue(execBB[bb]));
    }

    return C;
}
