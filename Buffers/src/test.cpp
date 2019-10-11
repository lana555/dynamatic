#include <iostream>
//#include "SatSolver.h"
//#include "Cats.h"
#include "ErrorManager.h"
//#include "Circuit.h"
#include "Dataflow.h"
#include "MILP_Model.h"
//#include "Dataflow.h"
#include "DFnetlist.h"
#include <sstream>


using namespace std;

using namespace Dataflow;

using vecParams = vector<string>;

string exec;
string command;

#if 0
int main_circuit(const vecParams& params)
{
    if (params.size() < 1 or params.size() > 3) {
        cerr << "Usage: " + exec + ' ' + command + " <input file> [<liberty file>] [info]" << endl;
        return 1;
    }

    CellLibrary Lib;
    Circuit C;
    if (params.size() > 1) {
        bool status = Lib.readLibrary(params[1]);
        if (not status) {
            cerr << "Error when reading library file." << endl;
            cerr << getLibError() << endl;
            return 1;
        }
    }

    //string infile(params[0]);
    C.readVerilog(params[0], &Lib);

    if (C.hasError()) {
        cerr << C.getError() << endl;
        return 1;
    }

    //C.breakLoops();
    string info = params.size() == 3 ? params[2] : "algh";
    cout << C.writeVerilogString(info) << endl;
    return 0;
}

int main_dataflow(const vecParams& params)
{
    if (params.size() != 2) {
        cerr << "Usage: " + exec + ' ' + command + " infile outfile_prefix" << endl;
        return 1;
    }

    DFnetlist DF(params[0]);

    if (DF.hasError()) {
        cerr << DF.getError() << endl;
        return 1;
    }

    DFnetlist DFopt(DF);
    //DFopt.removeControl();
    DFopt.optimize();
    //DFopt.calculateBasicBlocks();
    //DFopt.writeDot(params[1] + "_opt_nobuf.dot");
    DFopt.addElasticBuffers(50);
    DFopt.instantiateElasticBuffers();
    DFopt.calculateBasicBlocks();
    DFopt.writeDot(params[1] + "_opt.dot");
    DFopt.writeBasicBlockDot(params[1] + "_opt_bb.dot");

    DF.calculateBasicBlocks();
    DF.writeDot(params[1] + ".dot");
    DF.writeBasicBlockDot(params[1] + "_bb.dot");

    DFnetlist SCC(DF);
    SCC.eraseNonSCC();
    SCC.calculateBasicBlocks();
    SCC.writeDot(params[1] + "_scc.dot");
    SCC.writeBasicBlockDot(params[1] + "_scc_bb.dot");

    //DF.extractMarkedGraphs(1.0);

    //N.getPetrinet().writeDot(argv[3], true, false, ':');
    return 0;
}

int main_bbg()
{
    char names[] = "abcdefghi";

    BasicBlockGraph G;
    bbID a = G.createBasicBlock();
    bbID b = G.createBasicBlock();
    bbID c = G.createBasicBlock();
    bbID d = G.createBasicBlock();
    bbID e = G.createBasicBlock();
    bbID f = G.createBasicBlock();
    bbID g = G.createBasicBlock();
    bbID h = G.createBasicBlock();
    bbID i = G.createBasicBlock();
    G.setEntryBasicBlock(a);

    bbArcID ab = G.findOrAddArc(a, b);
    bbArcID bc = G.findOrAddArc(b, c);
    bbArcID cd = G.findOrAddArc(c, d);
    bbArcID ce = G.findOrAddArc(c, e);
    bbArcID df = G.findOrAddArc(d, f);
    bbArcID ef = G.findOrAddArc(e, f);
    bbArcID fc = G.findOrAddArc(f, c);
    bbArcID fg = G.findOrAddArc(f, g);
    bbArcID gg = G.findOrAddArc(g, g);
    bbArcID gh = G.findOrAddArc(g, h);
    bbArcID hb = G.findOrAddArc(h, b);
    bbArcID hi = G.findOrAddArc(h, i);


    G.setExecTime(a, 5);
    G.setExecTime(b, 3);
    G.setExecTime(c, 4);
    G.setExecTime(d, 5);
    G.setExecTime(e, 1);
    G.setExecTime(f, 2);
    G.setExecTime(g, 2);
    G.setExecTime(h, 6);
    G.setExecTime(i,10);

    G.setProbability(ab, 1);
    G.setProbability(bc, 1);
    G.setProbability(cd, 0.7);
    G.setProbability(ce, 0.3);
    G.setProbability(df, 1);
    G.setProbability(ef, 1);
    G.setProbability(fc, 0.9);
    G.setProbability(fg, 0.1);
    G.setProbability(gg, 0.9);
    G.setProbability(gh, 0.1);
    G.setProbability(hb, 0.9);
    G.setProbability(hi, 0.1);

    G.calculateBasicBlockFrequencies();

    for (bbID bb = 0; bb < G.numBasicBlocks(); ++bb) {
        cout << "freq(" << names[bb] << ") = " << G.getFrequency(bb) << endl;
    }

    G.extractBasicBlockCycles(0.95);
}

int main_milp(const vecParams& params)
{

    if (params.size() != 2) {
        cerr << "Usage: " + exec + ' ' + command + " solver outfile" << endl;
        return 1;
    }

    Milp_Model milp;
    milp.init(params[0]);
    int r1 = milp.newRealVar();
    int b1 = milp.newBooleanVar("b1");
    int b2 = milp.newBooleanVar("b2");
    int i1 = milp.newIntegerVar("i1", 1, 5);
    milp.newCostTerm(2, r1);
    milp.newCostTerm(1, b1);
    milp.newCostTerm(3, i1);

    int row1 = milp.newRow( {{1, b1}, {1, b2}}, '<' , 1);

    int row2 = milp.newRow('>', -2, "row2");
    milp.newTerm(row2, 4, r1);
    milp.newTerm(row2, -2, i1);
    milp.newTerm(row2, -1.36, b2);

    if (not milp.writeLP(params[1])) {
        cerr << "Could not open output file " << params[1] << endl;
        return 1;
    }

    bool status = milp.solve(30);

    if (not status) {
        cerr << milp.getError() << endl;
        return 1;
    }

    cout << "Obj= " << milp.getObj() << endl;
    cout << "Variable b1 = " << milp[b1] << endl;
    cout << "Variable b2 = " << milp[b2] << endl;
    cout << "Variable i1 = " << milp[i1] << endl;
    cout << "Variable r1 = " << milp[r1] << endl;

    return 0;
}


int main_dflib(const vecParams& params)
{
    if (params.size() < 2) {
        cerr << "Usage: " + exec + ' ' + command + " infile1 infile2 ... outfile" << endl;
        return 1;
    }

    DFlib L;

    for (unsigned int i = 0; i < params.size() - 1; ++i) {
        L.add(params[i]);
        if (L.hasError()) break;
    }

    if (L.hasError()) {
        cerr << L.getError() << endl;
        return 1;
    }

    L.writeDot(params.back());
    return 0;
}

#endif

int main_buffers(const vecParams& params)
{
    if (params.size() != 5) {
        cerr << "Usage: " + exec + ' ' + command + " period buffer_delay solver infile outfile" << endl;
        return 1;
    }

    DFnetlist DF(params[3]);

    if (DF.hasError()) {
        cerr << DF.getError() << endl;
        return 1;
    }

    // Lana 07.03.19. Removed graph optimizations (temporary)
    //DF.optimize();
    double period = atof(params[0].c_str());
    double delay = atof(params[1].c_str());

    cout << "Adding elastic buffers with period=" << period << " and buffer_delay=" << delay << endl;
    DF.setMilpSolver(params[2]);

    bool stat = DF.addElasticBuffers(period, delay, true);

    // Lana 05/07/19 Instantiate if previous step successful, otherwise just print and exit
    if (stat) {
        DF.instantiateElasticBuffers();
    }
    //DF.instantiateElasticBuffers();
    DF.writeDot(params[4]);
    return 0;
}

#if 0
int main_throughput(const vecParams& params)
{
    if (params.size() != 4) {
        cerr << "Usage: " + exec + ' ' + command + " period buffer_delay infile outfile" << endl;
        return 1;
    }

    DFnetlist DF(params[2]);

    if (DF.hasError()) {
        cerr << DF.getError() << endl;
        return 1;
    }

    double period = atof(params[0].c_str());
    double delay = atof(params[1].c_str());
    DF.setMilpSolver("cbc");

    DF.optimize();
    DF.calculateBasicBlocks();
}

int main_csv(const vecParams& params)
{
    if (params.size() < 3) {
        cerr << "Not enough arguments" << endl;
        return 1;
    }

    string infile(params[0]);
    string vfile(params[1]);
    string csvfile(params[2]);

    Cats A(infile);

    string err = A.getError();
    if (not err.empty()) {
        cerr << "Error: " << err << endl;
        return 1;
    }

    bool status = A.stateEncoding();
    if (not status) {
        cerr << A.getAsyncStatusMessage() << endl;
        return 1;
    }

    status = A.setMinimizationMethod();
    status |= A.setSupportReductionMethod();
    if (not status) {
        cerr << "Wrong configuration" << endl;
        return 1;
    }

    A.calculateEquations();
    A.writeCircuit(vfile);
    A.writeCircuitStats(csvfile);
    return 0;
}

int main_syncprod(const vecParams& params)
{
    if (params.size() != 3) {
        cerr << "Usage: " + exec + ' ' + command + " <Lts1> <Lts2> <LtsOut>" << endl;
        return 1;
    }
    string str_lts1(params[0]);
    string str_lts2(params[1]);
    string outfile(params[2]);

    if (outfile == "-") outfile = "";

    Cats L1(str_lts1);
    Cats L2(str_lts2);

    string err = L1.getError();
    if (not err.empty()) {
        cerr << "Error (" << str_lts1 << "): " << err << endl;
        return 1;
    }

    err = L2.getError();
    if (not err.empty()) {
        cerr << "Error (" << str_lts2 << "): " << err << endl;
        return 1;
    }

    Cats L12(L1, L2);
    L12.write(outfile);
    return 0;
}

int main_bisimilar(const vecParams& params)
{
    if (params.size() != 2) {
        cerr << "Usage: " + exec + ' ' + command + " <Lts1> <Lts2>" << endl;
        return 1;
    }
    string str_lts1(params[0]);
    string str_lts2(params[1]);

    Cats L1(str_lts1);
    Cats L2(str_lts2);

    string err = L1.getError();
    if (not err.empty()) {
        cerr << "Error (" << str_lts1 << "): " << err << endl;
        return 1;
    }

    err = L2.getError();
    if (not err.empty()) {
        cerr << "Error (" << str_lts2 << "): " << err << endl;
        return 1;
    }

    bool equiv = L1.compare(&L2);
    err = L1.getError();

    if (not err.empty()) {
        cerr << "Error: " << err << endl;
        return 1;
    }

    if (equiv) cout << "Equivalent" << endl;
    else cout << "Not equivalent" << endl;

    return 0;
}

int main_async_synth(const vecParams& params)
{
    if (params.size() < 2 or params.size() > 3) {
        cerr << "Usage: " + exec + ' ' + command + " <SG file> <Verilog file> <Lib file>" << endl;
        return 1;
    }

    string infile(params[0]);
    string vfile(params[1]);
    string libfile = "";
    if (params.size() == 3) libfile = params[2];
    //string sup_method(argv[3]);
    //string min_method(argv[4]);

    Cats A(infile);

    string err = A.getError();
    if (not err.empty()) {
        cerr << "Error: " << err << endl;
        return 1;
    }

    A.determinize();
    //A.minimize("wt", {".dummy"});
    bool status = A.stateEncoding();
    if (not status) {
        cerr << A.getAsyncStatusMessage() << endl;
        return 1;
    }


    /* bool status = A.setMinimizationMethod(min_method);
    status |= A.setSupportReductionMethod(sup_method);
    if (not status) {
        cerr << "Wrong configuration" << endl;
        return 1;
    }*/

    status = A.calculateEquations();
    if (not status) {
        cerr << "Error: no equations have been calculated." << endl;
        return 1;
    }

    if (not libfile.empty()) {

    }

    status = A.writeCircuit(vfile);
    if (not status) {
        cerr << "Error: The Verilog file has not been generated." << endl;
        return 1;
    }

    if (not libfile.empty()) {
        CellLibrary Lib;
        Circuit C;
        bool status = Lib.readLibrary(libfile);
        if (not status) {
            cerr << "Error when reading library file." << endl;
            cerr << getLibError() << endl;
            return 1;
        }

        C.readVerilog(vfile);
        C.assignLibrary(Lib);
        C.techMapping();
        C.writeVerilogFile(vfile);
    }

    return 0;
}

int main_windows(const vecParams& params)
{
    if (params.size() < 1) {
        cerr << "Not enough arguments" << endl;
        return 1;
    }

    string infile(params[0]);

    Cats PN(infile);
    if (not PN.isValid()) {
        return 1;
    }

    PN.writePetriNet("");
    PN.write();

    PN.extractWindow();
    PN.extractWindow();

    PN.getWindow(0)->write();
    PN.getWindow(1)->write();

    //int n = RG.calculateNodalStates();
    //cout << "It has " << n << " nodal states" << endl;

    return 0;
}

int main_nodal(const vecParams& params)
{
    string infile(params[0]);
    Cats A(infile);

    string err = A.getError();
    if (not err.empty()) {
        cerr << "Error: " << err << endl;
        return 1;
    }

    int n;
    try {
        n = A.calculateNodalStates();
    } catch (Cats_Exception e) {
        cout << "Msg: " << e.what() << endl;
    }
    cout << "It has " << n << " nodal states." << endl;
}

int main_solveCSC(const vecParams& params)
{
    if (params.size() < 3 or params.size() > 4) {
        cerr << "Usage: " + exec + ' ' + command + " infile SG_outfile Verilog_outfile [coeff_string]" << endl;
        return 1;
    }

    string infile(params[0]);

    Cats A(infile);

    string err = A.getError();
    if (not err.empty()) {
        cerr << "Error: " << err << endl;
        return 1;
    }

    A.determinize();
    bool status = A.stateEncoding();
    if (status) {
        cerr << "The circuits has no CSC conflicts" << endl;
        //cerr << A.getAsyncStatusMessage() << endl;
        return 1;
    }

    if (params.size() == 4) status = A.solveCSC(params[3]);
    else status = A.solveCSC();

    if (not status) {
        cerr << A.getAsyncStatusMessage() << endl;
        return 1;
    }

    status = A.write(params[1]);
    if (not status) {
        cerr << A.getAsyncStatusMessage() << endl;
        return 1;
    }

    status = A.calculateEquations();
    if (not status) {
        cerr << A.getAsyncStatusMessage() << endl;
        return 1;
    }

    status = A.writeCircuit(params[2]);
    if (not status) {
        cerr << A.getAsyncStatusMessage() << endl;
        return 1;
    }

    return 0;
}

int main_persistence(const vecParams& params)
{
    if (params.size() != 1) {
        cerr << "Wrong number of arguments" << endl;
        return 1;
    }

    string infile(params[0]);

    Cats A(infile);

    string err = A.getError();
    if (not err.empty()) {
        cerr << "Error: " << err << endl;
        return 1;
    }

    bool status = A.stateEncoding();
    if (not status) {
        cerr << A.getAsyncStatusMessage() << endl;
    }

    cout << "Going to solve persistence" << endl;
    status = A.solvePersistence();
    //A.calculateEquations();
    return 0;
}

int main_hideSignals(const vecParams& params)
{
    if (params.size() != 3) {
        cerr << "Usage: " + exec + ' ' + command + " <LTSin> \"<signals>\" <LTSout>" << endl;
        return 1;
    }

    string outfile(params[2]);

    if (outfile == "-") outfile = "";

    Cats A(params[0]);

    string err = A.getError();
    if (not err.empty()) {
        cerr << "Error: " << err << endl;
        return 1;
    }

    bool status = A.hideSignals(params[1]);
    if (not status) {
        cerr << A.getError() << endl;
    }

    if (A.hasTau()) {
        cerr << "Tau could not be hidden to preserve weak bisimilarity." << endl;
    }

    A.write(outfile);
    return 0;
}

#endif
int main_help()
{
    cerr << "Usage: " + exec + " cmd params" << endl;
    cerr << "Available commands:" << endl;
    cerr << "  dataflow:      handling dataflow netlists." << endl;
    cerr << "  buffers:       add elastic buffers to a netlist." << endl;
    cerr << "  async_synth:   synthesize an asynchronous circuit." << endl;
    cerr << "  solveCSC:      solve state encoding in an asynchronous circuit." << endl;
    cerr << "  hideSignals:   hide signals in an asynchronous specification." << endl;
    cerr << "  syncprod:      calculate the synchronous product of two LTSs." << endl;
    cerr << "  bisimilar:     check whether two LTSs are weakly bisimilar." << endl;
    cerr << "  bbg:           unit test for Basic Block graphs." << endl;
    cerr << "  circuit:       unit test to read and write a circuit." << endl;
}

int main_shab(const vecParams& params){


    if (params.size() != 9 && params.size() != 10) {
        cerr << "Usage: " + exec + ' ' + command + " period buffer_delay solver infile outfile" << endl;
        return 1;
    }


    if (params.size() == 9) {
        DFnetlist DF(params[4], params[5]);

        if (DF.hasError()) {
            cerr << DF.getError() << endl;
            return 1;
        }
        double period = atof(params[0].c_str());
        double delay = atof(params[1].c_str());
        cout << "Adding elastic buffers with period=" << period << " and buffer_delay=" << delay << endl;
        cout << endl;
        DF.setMilpSolver(params[2]);

        bool stat;
        if (stoi(params[3]))
            stat = DF.addElasticBuffersBB_sc(period, delay, true, 1, -1, stoi(params[8]));
        else
            stat = DF.addElasticBuffersBB(period, delay, true, 1, -1, stoi(params[8]));

        if (stat) {
            DF.instantiateElasticBuffers();
        }
        DF.writeDot(params[6]);
        DF.writeDotBB(params[7]);

    } else {
        DFnetlist DF(params[5], params[6]);
        if (DF.hasError()) {
            cerr << DF.getError() << endl;
            return 1;
        }
        double period = atof(params[0].c_str());
        double delay = atof(params[1].c_str());
        cout << "Adding elastic buffers with period=" << period << " and buffer_delay=" << delay << endl;
        cout << endl;
        DF.setMilpSolver(params[2]);

        bool stat;
        if (stoi(params[3]))
            stat = DF.addElasticBuffersBB_sc(period, delay, true, 1, stoi(params[4]), stoi(params[9]));
        else
            stat = DF.addElasticBuffersBB(period, delay, true, 1, stoi(params[4]), stoi(params[9]));

        if (stat) {
            DF.instantiateElasticBuffers();
        }
        DF.writeDot(params[7]);
        DF.writeDotBB(params[8]);
    }

    return 0;
}

int main_test(const vecParams& params){
    DFnetlist DF(params[0], params[1]);
//    DF.computeSCC(false);
  //  DF.printBlockSCCs();
}

int main(int argc, char *argv[])
{
    //return main_persistence(argc, argv);
    //return main_csv(argc, argv);
    //return main_nodal(argc, argv);
    //return main_windows(argc, argv);

    exec = basename(argv[0]);
    if (argc == 1) return main_help();

    command = argv[1];
    if (command == "help") return main_help();

    vecParams params;
    for (int i = 2; i < argc; ++i) params.push_back(argv[i]);

    if (command == "buffers") return main_buffers(params);
    if (command == "shab") return main_shab(params);
    if (command == "test") return main_test(params);

#if 0    
    if (command == "dataflow") return main_dataflow(params);
    if (command == "dflib") return main_dflib(params);
    if (command == "syncprod") return main_syncprod(params);
    if (command == "bisimilar") return main_bisimilar(params);
    if (command == "hideSignals") return main_hideSignals(params);
    if (command == "solveCSC") return main_solveCSC(params);
    if (command == "async_synth") return main_async_synth(params);
    if (command == "milp") return main_milp(params);
    if (command == "bbg") return main_bbg();
    if (command == "circuit") return main_circuit(params);
#endif
    cerr << command << ": Unknown command." << endl;
    return main_help();
}
