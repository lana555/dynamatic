#include "DFnetlist.h"

using namespace Dataflow;
using namespace std;

void DFlib_Impl::init()
{
    libName.clear();
    error.clear();
    nFuncs = 0;
    freeDF = -1;

    funcs.clear();
    name2func.clear();
    allFuncs.clear();
}

DFlib_Impl::DFlib_Impl(const string& filename)
{
    init();
    add(filename);
}

bool DFlib_Impl::add(const string& filename)
{
    FILE* f = fopen(filename.c_str(), "r");
    if (f == nullptr) {
        setError("File " + filename + " could not be opened.");
        return false;
    }

    string lastName;
    bool status = true;
    while (true) {
        funcs.push_back(DFnetlist(f));
        funcID id = funcs.size() - 1;
        const DFnetlist& func = funcs[id];
        string newName = func.getName();
        if (funcs[id].hasError()) {
            status = false;
            string err;
            if (newName.empty()) {
                if (not lastName.empty()) err = "Error after function " + lastName + ": ";
            } else {
                err = "Error in function " + newName + ": ";
            }
            setError(err + funcs[id].getError());
            break;
        }

        // let us check if it is empty
        if (newName.empty()) {
            funcs.pop_back();
            break;
        }

        // Now we have a new function
        lastName = newName;

        // Let us check for duplicate names
        if (name2func.count(newName) > 0) {
            setError("Error: function " + newName + " multiply defined.");
            funcs.pop_back();
            status = false;
            break;
        }

        name2func[newName] = id;
    }

    fclose(f);
    return status;
}
