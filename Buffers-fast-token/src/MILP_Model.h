#ifndef MILP_MODEL_H
#define MILP_MODEL_H

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <string.h>
#include <unistd.h>
#include <regex>

using namespace std;

/**
 * @class Milp_Model
 * @author Jordi Cortadella
 * @date 16/04/18
 * @file MILP_Interface.h
 * @brief Class to represent an MILP model. Variables and rows
 * are represented by indices from 0.
 */
class Milp_Model
{
public:

    enum VarType {REAL, INTEGER, BOOLEAN};  /// Types of variables
    enum RowType {LEQ, GEQ, EQ};            /// Type of constraints
    enum Status {OPTIMAL, NONOPTIMAL, UNFEASIBLE, UNBOUNDED, UNKNOWN, ERROR};   /// Type of solution

    //SHAB_q: I don't understand what 'Term' means.
    //SHAB_ans: the pair literally mean: coefficient * vecVars[var index]. like 2a, 3x, etc.
    using Term = pair<double, int>;         /// Type to represent a term (coefficient, var index)
    using vecTerms = vector<Term>;          /// Vector of terms
    using vecVars = vector<int>;            /// Vector of variables

    /**
     * @brief Default constructor.
     */
    Milp_Model(const string& solver="") {
        init(solver);
    }

    /**
     * @return The error message associated to the object.
     */
    const string& getError() const {
        return errorMsg;
    }

    /**
     * @brief Defines the error message of the object (no error by default).
     * @param err Error message.
     */
    void setError(const string& err = "") {
        errorMsg = err;
    }

    /**
     * @brief Defines a model to minimize the cost function.
     */
    void setMinimize() {
        MinMax = true;
    }

    /**
     * @brief Defines a model to maximize the cost function.
     */
    void setMaximize() {
        MinMax = false;
    }

    /**
     * @brief Defines the min epsilon value to distinguish from zero.
     * @param eps Epsilon value.
     */
    void setEpsilon(double eps) {
        epsilon = eps;
    }

    /**
     * @return The number of variables of the model.
     */
    int numVariables() const {
        return Vars.size();
    }

    int numConstraints() const {
        return Matrix.size();
    }

    /**
     * @brief Creates a new real variable.
     * @param name Name of the variable.
     * @param lower_bound Lower bound
     * @param upper_bound Upper bound (unbounded if upper_bound < lower_bound).
     * @return The index of the variable.
     */
    int newRealVar(string name="",
                   double lower_bound = 0.0,
                   double upper_bound = -1.0) {
        return newVar(name, REAL, lower_bound, upper_bound);
    }

    /**
    * @brief Creates a new integer variable.
    * @param name Name of the variable.
    * @param lower_bound Lower bound.
    * @param upper_bound Upper bound (unbounded if upper_bound < lower_bound).
    * @return The index of the variable.
    */
    int newIntegerVar(string name="",
                      double lower_bound = 0,
                      double upper_bound = -1) {
        return newVar(name, INTEGER, lower_bound, upper_bound);
    }

    /**
    * @brief Creates a new Boolean variable.
    * @param name Name of the variable.
    * @return The index of the variable.
    */
    int newBooleanVar(string name="") {
        return newVar(name, BOOLEAN, 0, 1);
    }

    /**
     * @return The status of the solution.
     */
    Status getStatus() const {
        return stat;
    }

    /**
     * @return The value of the objective function.
     */
    double getObj() const {
        return obj;
    }

    /**
      * @brief Returns the value of a variable.
      * @param i Index of the variable.
      * @return The value of variable i.
      */
    double operator[](int i) const {
        assert (i >= 0 and i < Vars.size());
        return Vars[i].value;
    }

    /**
     * @brief Returns the value of a variable.
     * @param name Name of the variable.
     * @return The value of variable.
     */
    double operator[](const string& name) const {
        assert (Name2Var.count(name) > 0);
        return Vars[Name2Var.at(name)].value;
    }

    /**
     * @brief Checks whether the value of a boolean variable is true.
     * @param i Index of the variable.
     * @return True if the variable is asserted, and false otherwise.
     */
    bool isTrue(int i) const {
        assert (i >= 0 and i < Vars.size() and Vars[i].type == BOOLEAN);
        return Vars[i].value > 0.5;
    }

    /**
     * @brief Checks whether the value of a boolean variable is false.
     * @param i Index of the variable.
     * @return True if the variable is not asserted, and false otherwise.
     */
    bool isFalse(int i) const {
        return not isTrue(i);
    }

    /**
     * @brief Creates a new row in the constraint matrix.
     * @param type Type of row ('<', '>', '=').
     * @param rhs RHS of the constraint.
     * @param name Name of the row.
     * @return The index of the row.
     */
    int newRow(char type = '<', double rhs = 0.0, const string& name = "") {
        RowType t;
        if (type == '<') t = LEQ;
        else if (type == '>') t = GEQ;
        else if (type == '=') t = EQ;
        else assert(false);
        Matrix.push_back(Row {name, t, rhs});
        return Matrix.size() - 1;
    }

    /**
     * @brief Creates a new row in the constraint matrix from a vector of terms.
     * @param terms Vector of terms (pairs of {coeff, var index}).
     * @param type Type of row ('<', '>', '=').
     * @param rhs RHS of the constraint.
     * @param name Name of the constraint.
     * @return The index of the row.
     */
    int newRow(const vecTerms& terms, char type = '<', double rhs = 0.0, const string& name = "") {
        int r = newRow(type, rhs, name);
        for (auto t: terms) newTerm(r, t.first, t.second);
        return r;
    }

    /**
     * @brief Adds a new term to a constraint.
     * @param rowIndex Index of the constraint.
     * @param coeff Coefficient of the term.
     * @param varIndex Index of the variable.
     */
    void newTerm(int rowIndex, double coeff, int varIndex) {
        assert (rowIndex >= 0);
        assert (rowIndex < Matrix.size());
        assert (varIndex >= 0);
        assert (varIndex < Vars.size());
        assert (rowIndex >= 0 and rowIndex < Matrix.size() and
                varIndex >= 0 and varIndex < Vars.size());
        Matrix[rowIndex].vecRow.push_back( {coeff, varIndex});
    }

    /**
     * @brief Defines the RHS of a constraint.
     * @param rowIndex Index of the constraint.
     * @param rhs RHS value.
     */
    void setRHS(int rowIndex, double rhs) {
        assert (rowIndex >= 0 and rowIndex < Matrix.size());
        Matrix[rowIndex].rhs = rhs;
    }

    /**
     * @brief Creates an at-most-one constraint (v1+...+vn <= 1).
     * @param vars The variables of the constraint.
     * @param name The name of the constraint.
     * @return The index of the row.
     */
    int atMostOne(const vecVars& vars, const string& name="") {
        int r = newRow('<', 1.0, name);
        for (int v: vars) newTerm(r, 1.0, v);
        return r;
    }

    /**
      * @brief Creates an at-least-one constraint (v1+...+vn >= 1).
      * @param vars The variables of the constraint.
      * @param name The name of the constraint.
      * @return The index of the row.
      */
    int atLeastOne(const vecVars& vars, const string& name="") {
        int r = newRow('>', 1.0, name);
        for (int v: vars) newTerm(r, 1.0, v);
        return r;
    }

    /**
    * @brief Creates an exactly-one constraint (v1+...+vn = 1).
    * @param vars The variables of the constraint.
    * @param name The name of the constraint.
    * @return The index of the row.
    */
    int exactlyOne(const vecVars& vars, const string& name="") {
        int r = newRow('=', 1.0, name);
        for (int v: vars) newTerm(r, 1.0, v);
        return r;
    }

    /**
    * @brief Creates a constraint indicating that the sum of a set
    * of variables must be equal to the sum of another set, i.e.,
    * x1 + ... + xn = y1 + ... + ym.
    * @param x The first set of variables.
    * @param y The second set of variables.
    * @param name The name of the constraint.
    * @return The index of the row.
    */
    int equalSum(const vecVars& x, const vecVars& y, const string& name="") {
        int r = newRow('=', 0.0, name);
        for (int v: x) newTerm(r, 1.0, v);
        for (int v: y) newTerm(r, -1.0, v);
        return r;
    }

    /**
     * @brief Creates a set of constraints that enforces a set of variables
     * to have the same value.
     * @param v The vector of variables.
     * @return The index of the first row of the constraints.
     * @note The vector is assumed to contain two variables at least.
     */
    int allEqual(const vecVars& v) {
        assert (v.size() > 1);
        int row = newRow( {{1, v[0]}, {-1, v[1]}}, '=');
        for (unsigned int i = 2; i < v.size(); ++i) newRow( {{1, v[0]}, {-1, v[i]}}, '=');
        return row;
    }

    /**
     * @brief Creates a constraint representing the Boolean implication
     * x => y1 + ... + ym.
     * @param x The variable in the antecedent.
     * @param y The set of variables in the consequent.
     * @return The index of the row.
     */
    int implies(int x, const vecVars& y) {
        int r = newRow('>', 0.0);
        newTerm(r, -1.0, x);
        for (int v: y) newTerm(r, 1.0, v);
        return r;
    }

    /**
     * @brief Creates a set of constraints representing the Boolean implication
     * x1 + ... + xn => y.
     * @param x The set of variables in the antecedent.
     * @param y The variable in the consequent.
     * @return The index of the first row.
     */
    int implies(const vecVars& x, int y) {
        int first = -1;
        for (int v: x) {
            int r = newRow('>', 0.0);
            if (first < 0) first = r;
            newTerm(r, -1.0, v);
            newTerm(r, 1.0, y);
        }
        return first;
    }


    /**
     * @brief Adds a new term to the cost function.
     * @param coeff Coefficient of the term.
     * @param varIndex Index of the variable.
     */
    void newCostTerm(double coeff, int varIndex) {
        assert (varIndex >= 0 and varIndex < Vars.size());
        Cost.push_back( {coeff, varIndex});
    }

    /**
     * @brief Write the LP model into a file in CPLEX LP format.
     * @param filename Name of the file.
     * @return True if successful, and false otherwise.
     */
    bool writeLP(const string& filename) {
        ofstream f;
        f.open(filename);
        if (not f.is_open()) {
            setError("Could not open file " + filename + ".");
            return false;
        }
        writeLP(f);
        f.close();
        return true;
    }

    /**
     * @brief Calls the MILP solver.
     * @param timelimit Time limit in seconds. No limit if <= 0.
     * @return True if successful, and false otherwise.
     */
    bool solve(int timelimit = -1) {

        if (solver != "cbc" and solver != "glpsol" and solver != "gurobi_cl") { //Carmine 25.02.22 gurobi_cl included as MILP solver
            setError("Unkonwn solver " + solver + ".");
            return false;
        }

        string exec = "which " + solver + " >/dev/null 2>&1";
        if (system(exec.c_str())) {
            setError("Solver " + solver + " is not available.");
            return false;
        }

        // The solver seems to be working. Let us solve the MILP model.
        string lpfile = createTempFilename("MILP_lpmodel", ".lp");
        string outfile;
        if(solver == "gurobi_cl")  //Carmine 25.02.22 gurobi_cl accepts solution in sol format
            outfile = createTempFilename("MILP_solution", ".sol");
        else
            outfile = createTempFilename("MILP_solution", ".gsol");
        string command = writeCommand(solver, lpfile, outfile, timelimit) + " >/dev/null 2>&1";
        writeLP(lpfile);

        int status = system(command.c_str());

        if (status != 0) {
            deleteTempFilename(outfile);
            setError("Error when executing " + solver + ".");
            return false;
        }
        bool opt_status; //Carmine 25.02.22 Important to check if the status optimization is OPTIMAL or SUBOPTIMAL
        if (solver == "cbc") opt_status = readCbcSolution(outfile);
        else if (solver == "glpsol") opt_status = readGlpsolSolution(outfile);
        else if (solver == "gurobi_cl") opt_status = readGurobiSolution(outfile); //Carmine 25.02.22 gurobi_cl included as MILP solver
        else assert(false);

        if(!opt_status)
            cout << "*ERROR* MILP solution is UNFEASIBLE or UNBOUNDED" << endl;

        //deleteTempFilename(lpfile);
        //deleteTempFilename(outfile);
        return true;
    }

    /**
     * @brief Initializes the MILP model.
     * @param solver Name of the MILP solver.
     * @return True if the initialization was correct, and false otherwise.
     */
    bool init(const string& solver = "") {
        MinMax=true;
        epsilon=10e-10;
        numRealVars=numIntegerVars=numBooleanVars=0;
        numEmptyRows = 0;
        numUsedVars = 0;
        stat=UNKNOWN;
        errorMsg = "";
        Cost.clear();
        Vars.clear();
        Matrix.clear();
        Name2Var.clear();
        Name2delays.clear();
        return find_solver(solver);
    }

    /**
     * @brief Writes a output file to save output pin timing to retrieve critical path
     * @param file_name name of the file where to save the output
     */
    void writeOutDelays(const string& file_name) {
        ofstream file;
        file.open(file_name);
        for (auto const& values: Name2delays){
            file << values.second << "  " <<  Vars[values.first].value << endl;
        }
        file.close();

        file.open("tmp_delays.txt");
        int i, ord = Vars.size();
        for (i = 0; i< ord; i++){
            file <<  Vars[i].name << "  " <<  Vars[i].value << endl;
        }
        file.close();
    }

    /**
     * @brief Get variables names
     * @param id variable id
     */
    string getVarName(int id) {
        return Vars[id].name;
    }

private:

    struct Var {
        string name;
        VarType type;
        double lower_bound;
        double upper_bound; // unbounded if upper_bound < lower_bound
        double value;
    };

    struct Row {
        string name;
        RowType type;           /// Type of constraint
        double rhs;             /// Constant at the rhs
        vecTerms vecRow;    /// LHS of the row (sorted terms by variables)
    };

    string solver;      /// Solver to be used
    bool MinMax;        /// Minimization (true) or maximization (false)
    vecTerms Cost;      /// Cost function
    vector<Var> Vars;   /// List of variables (columns)
    vector<Row> Matrix; /// Matrix of constraints
    int numEmptyRows;   /// Number of empty rows in the matrix
    vector<int>appearanceOrder; /// Vector to store the indices in order of appearance
    vector<bool>appeared;       /// Variables already appeared
    int numUsedVars;            /// Number of used variables
    double epsilon;     /// Epsilon to be considered as zero
    int numRealVars;
    int numIntegerVars;
    int numBooleanVars;
    map<string, int> Name2Var;  /// Mapping from var names to var indices
    Status stat;        /// Status of the solution
    double obj;        /// Value of the cost function
    string errorMsg;    /// Error message in case an error is produced.

    map<int, string> Name2delays; //Carmine 07.02.2022 map containing the output delay of the blocks

    bool find_solver(const string& s) {

        if (not s.empty() and s != "cbc" and s != "glpsol" and s != "gurobi_cl") { //Carmine 25.02.22 gurobi_cl included as MILP solver
            setError("Unkonwn solver " + solver + ".");
            return false;
        }
        
        // First try cbc
        if (s.empty() or s == "cbc") {
            string exec = "which cbc >/dev/null 2>&1";
            if (system(exec.c_str()) == 0) {
                solver = "cbc";
                return true;
            }

            if (s == "cbc") {
                setError("MILP solver cbc not found.");
                return false;
            }
        }

        // Next try glpsol
        if (s.empty() or s == "glpsol") {
            string exec = "which glpsol >/dev/null 2>&1";
            if (system(exec.c_str()) == 0) {
                solver = "glpsol";
                return true;
            }

            if (s == "glpsol") {
                setError("MILP solver glpsol not found.");
                return false;
            }
        }

        // Next try gurobi_cl
        if (s.empty() or s == "gurobi_cl") { //Carmine 25.02.22 gurobi_cl included as MILP solver
            string exec = "which gurobi_cl >/dev/null 2>&1";
            if (system(exec.c_str()) == 0) {
                solver = "gurobi_cl";
                return true;
            }

            if (s == "gurobi_cl") {
                setError("MILP solver gurobi_cl not found.");
                return false;
            }
        }

        // No more solvers for the moment
        setError("No MILP solver found.");
        return false;
    }

    /**
     * @brief Creates a new variables and returns its index.
     * @param name Name of the variable.
     * @param type Type of the variable (real, Integer or Boolean).
     * @param lower_bound Lower bound for the variable.
     * @param upper_bound Upper bound for the variable.
     * @return The index of the variable (-1 if error).
     */
    int newVar(const string& name, VarType type, double lower_bound, double upper_bound) {
        string n = name.empty() ? "x" + to_string(Vars.size()) : name;
        if (Name2Var.count(n) > 0) {
            setError("Variable " + n + " multiply defined.");
            return -1;
        }
        Name2Var[n] = Vars.size();
        Vars.push_back(Var {n, type, lower_bound, upper_bound, 0});
        if (type == REAL) numRealVars++;
        else if (type == INTEGER) numIntegerVars++;
        else numBooleanVars++;
        return Vars.size() - 1;
    }

    /**
     * @brief Creates a new block output value associated to an index
     * @param output Name of the output pin
     * @param ind Index at which this value is appeared
     * @return 1 if new value/ 0 if value already present
     */
    int newOutDelay(int ind, string output) {
        if (Name2delays.count(ind) > 0)
            return 0;
        Name2delays[ind] = output;
        return 1;
    }

    /**
     * @brief Normalize the model. It accumulates terms with
     * the same variable and remove terms with zero coefficient.
     */
    void normalize() {
        normalizeRow(Cost);
        for (Row& r: Matrix) normalizeRow(r.vecRow);
    }

    /**
     * @brief Accumulates terms with the same variable and eliminates
     * the terms with zero coefficient.
     * @param r A reference to the row.
     */
    void normalizeRow(vecTerms& r) {
        // Sorts the terms by variable index
        sort(r.begin(), r.end(), [](Term& t1, Term& t2) {
            return t1.second < t2.second;
        });

        // Now check consecutive terms and accumulate coefficients
        int i = 0;
        int last = r.size();
        while (i < last - 1) {
            int var = r[i].second;
            unsigned int k = i + 1;
            while (k < last and r[k].second == var) {
                r[i].first += r[k].first;
                r[k].first = 0;
                ++k;
            }
            i = k;
        }

        // Now remove the terms with zero coefficient
        unsigned int k = 0;  // to point at the first available slot
        for (i = 0; i < last; ++i) {
            if (abs(r[i].first) >= epsilon) r[k++] = r[i];
        }
        r.resize(k);
    }

    /**
     * @brief Writes the terms of a linear constraint to f.
     * @param f The output stream.
     * @param terms The vector of terms.
     */
    void writeTerms(ofstream& f, const vecTerms& terms) {
        assert (terms.size() > 0);
        double coeff = terms[0].first;
        if (abs(coeff) != 1.0) f << coeff << ' ';
        else if (coeff < 0) f << '-';
        int idx = terms[0].second;
        f << Vars[idx].name;
        if (not appeared[idx]) {
            appearanceOrder.push_back(idx);
            appeared[idx] = true;
            regex regexp ("timePath_(.)+_out[0-9]+");  //Carmine 07.02.2022 Extracting timing output pins indexes
            if (regex_search(Vars[idx].name, regexp)){
                newOutDelay(idx, Vars[idx].name);
            }
        }

        for (unsigned int i = 1; i < terms.size(); ++i) {
            coeff = terms[i].first;
            f << ' ' << (coeff < 0 ? '-' : '+') << ' ';
            if (abs(coeff) != 1) f << abs(coeff) << ' ';
            idx = terms[i].second;
            f << Vars[idx].name;
            if (not appeared[idx]) {
                appearanceOrder.push_back(idx);
                appeared[idx] = true;
                regex regexp ("timePath_(.)+_out[0-9]+");  //Carmine 07.02.2022 Extracting timing output pins indexes
                if (regex_search(Vars[idx].name, regexp)){
                    newOutDelay(idx, Vars[idx].name);
            }
            }
        }
    }

    /**
     * @brief Writes a row of the constraint matrix into the output stream.
     * @param f The output stream.
     * @param r A reference to the row.
     */
    void writeRow(ofstream& f, const Row& r) {
        if (r.vecRow.empty()) {
            ++numEmptyRows;
            return;
        }
        f << "  ";
        if (not r.name.empty()) f << r.name << ": ";
        writeTerms(f, r.vecRow);
        if (r.type == EQ) f << " = ";
        else if (r.type == GEQ) f << " >= ";
        else f << " <= ";
        f << r.rhs << endl;
    }

    /**
     * @brief Writes the model into an output stream in CPLEX LP format.
     * @param f The output stream.
     */
    void writeLP(ofstream& f) {
        normalize();
        appearanceOrder.clear();
        appeared = vector<bool>(Vars.size(), false);

        // Cost function: if no cost, create a fake cost function
        if (Cost.empty()) newCostTerm(0, 0);

        // Cost function
        f << (MinMax ? "Minimize" : "Maximize") << endl;
        f << "  ";
        writeTerms(f, Cost);
        f << endl;

        f << "Subject to" << endl;
        for (const Row& r: Matrix) writeRow(f, r);

        numUsedVars = 0;
        for (bool b: appeared) if (b) ++numUsedVars;

        bool need_bounds = false;
        for (const Var& v: Vars) {
            if (v.type != BOOLEAN and v.lower_bound <= v.upper_bound) {
                need_bounds = true;
                break;
            }
        }

        if (need_bounds) {
            f << "Bounds" << endl;
            for (const Var& v: Vars) {
                if (v.type != BOOLEAN and v.lower_bound <= v.upper_bound) {
                    f << "  " << v.lower_bound << " <= " << v.name << " <= " << v.upper_bound << endl;
                }
            }
        }

        if (numIntegerVars > 0) {
            f << "General" << endl << ' ';
            for (const Var& v: Vars) {
                if (v.type == INTEGER) f << ' ' << v.name;
            }
            f << endl;
        }

        if (numBooleanVars > 0) {
            f << "Binary" << endl << ' ';
            for (const Var& v: Vars) {
                if (v.type == BOOLEAN) f << ' ' << v.name;
            }
            f << endl;
        }

        f << "End" << endl;
    }

    /**
     * @brief Reads a line from an ifstream and puts all strings in a vector.
     * @param f The input stream.
     * @param values The vector of strings.
     * @return The number of items read from the stream.
     */
    static int readLine(ifstream& f, vector<string>& items) {
        string line;
        getline(f, line);
        istringstream myStream(line);
        items.clear();
        string s;
        while (myStream >> s) items.push_back(s);
        return items.size();
    }

    /**
     * @brief Reads a solution from the command gsolu of Cbc.
     * @param filename Name of the file.
     * @return True if the file has be successfully read, and false otherwise.
     */
    bool readCbcSolution(const string& filename) {
        ifstream f;
        f.open(filename);
        if (not f.is_open()) return false;

        vector<string> values;
        int nv = readLine(f, values);
        assert(nv == 2);

        int nrows = stoi(values[0]);
        int ncols = stoi(values[1]);

        assert (nrows == Matrix.size() - numEmptyRows + 1);
        assert (ncols == numUsedVars);

        // This is a tricky function that has to deal with the
        // fact that cbc sometimes puts three values per line
        // and sometimes only one (possibly when MILP model).
        nv = readLine(f, values);

        int st = stoi(values[0]);
        switch (st) {
        case 1:
            stat = UNBOUNDED;
            break;
        case 2:
            stat = NONOPTIMAL;
            break;
        case 4:
            stat = UNFEASIBLE;
            break;
        case 5:
            stat = OPTIMAL;
            break;
        default:
            assert(false);
        }

        if (stat != OPTIMAL and stat != NONOPTIMAL) {
            f.close();
            return false; //Carmine 25.02.22 if it is unfeasible or unbounded it should stop dynamatic execution
        }

        obj = stod(values.back());
        if (not MinMax) obj = -obj;

        // Skip the value of the rows
        for (unsigned int i = 0; i < nrows; ++i) readLine(f, values);

        // Read the variables
        for (unsigned int i = 0; i < ncols; ++i) {
            nv = readLine(f, values);
            int loc = nv > 1 ? 1 : 0;
            Vars[appearanceOrder[i]].value = stod(values[loc]);
        }

        f.close();
        return true;
    }

    /**
     * @brief Reads a solution from glpsol.
     * @param filename Name of the file.
     * @return True if the file has be successfully read, and false otherwise.
     */
    bool readGlpsolSolution(const string& filename) {
        ifstream f;
        f.open(filename);
        if (not f.is_open()) return false;

        bool cost_read = false;
        vector<string> line;

        while (readLine(f, line) > 0) {
            char c = line[0][0];
            if (c != 's' and c != 'j') {
                // Skip line
                continue;
            } else if (c == 'j') {
                // Now we can read the value of a variable
                int nvar = stoi(line[1]);
                int loc = line.size() == 3 ? 2 : 3;
                Vars[appearanceOrder[nvar-1]].value = stod(line[loc]);
            } else { // c == 's'
                assert (not cost_read);
                cost_read = true;
                // Read the cost function
                int nrows = stoi(line[2]);
                int ncols = stoi(line[3]);
                char st = line[4][0];
                obj = stod(line.back());

                assert (nrows == Matrix.size() - numEmptyRows);
                assert (ncols == numUsedVars);

                switch (st) {
                case 'u':
                    stat = UNBOUNDED;
                    break;
                case 'f':
                    stat = NONOPTIMAL;
                    break;
                case 'n':
                    stat = UNFEASIBLE;
                    break;
                case 'o':
                    stat = OPTIMAL;
                    break;
                default:
                    assert(false);
                }
            }
        }

        f.close();
        return true;
    }

    /**
     * @brief Reads a solution from gurobi.
     * @param filename Name of the file.
     * @return True if the file has be successfully read, and false otherwise.
     */
    bool readGurobiSolution(const string& filename) {
        ifstream f;
        f.open(filename);
        if (not f.is_open()) return false;

        vector<string> values;
        readLine(f, values); // first line only tells the optimization function cost of the solution

        string line;
        int numVars = 0;
        while(getline(f, line)){
            istringstream myStream(line);
            values.clear();
            string s;
            while (myStream >> s) values.push_back(s);
            Vars[Name2Var[values[0]]].value = stod(values[1]);
            numVars++;
        }
        
        //assert (numVars == numUsedVars);

        //TODO: check the optimization status of gurobi
        stat = OPTIMAL;
        
        f.close();
        return true;
    }

    /**
     * @brief Generates the command to invoke the MILP solver.
     * @param solver Name of the MILP solver.
     * @param lpfile name of the input file (CPLEX LP format).
     * @param solfile name of the solution file.
     * @param timeout Max amount of time to solve the problem (in seconds).
     * @return A string with the command to be executed.
     */
    string writeCommand(const string& solver, const string& lpfile, const string& solfile,
                        int timeout = -1) {
        ostringstream command;
        command << solver << ' ';
        if (solver == "cbc") {
            command << lpfile;
            if (timeout > 0) command << " sec " << timeout;
            command << " solve gsolution " << solfile;
        } else if (solver == "glpsol") {
            command << " --lp " << lpfile;
            if (timeout > 0) command << " --tmlim " << timeout;
            command << " -w " << solfile;
        } else if (solver == "gurobi_cl") { //Carmine 25.02.22 gurobi_cl included as MILP solver
            if (timeout > 0) command << " TimeLimit=" << timeout;
            command << " ResultFile=" << solfile;
            command << " " << lpfile;
        } else {
            assert(false);
        }
        return command.str();
    }

    /**
     * @brief Creates a temporary file name with the format "prefix".XXXXXXsuffix.
     * This format is used with mkstemp to create a temporary file.
     * @param prefix Prefix of the file.
     * @param suffix Suffix of the file.
     * @return The file name.
     */
    static std::string createTempFilename(const std::string& prefix, const std::string& suffix = "") {
        char fname[1000];
        strcat(strcpy(fname, prefix.c_str()), ".XXXXXX");
        if (not suffix.empty()) strcat(fname, suffix.c_str());
        int file = mkstemps(fname, suffix.size());
        if (file == -1) return "";
        return fname;
    }

    /**
     * @brief Deletes a temporary file name.
     * @param filename The name of the file.
     */
    static void deleteTempFilename(const std::string& filename) {
        unlink(filename.c_str());
    }
};
#endif // MILP_INTERFACE_H
