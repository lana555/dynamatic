#ifndef ERRORMANAGER_H
#define ERRORMANAGER_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

/**
 * @class ErrorMgr
 * @author Jordi Cortadella
 * @date 05/01/19
 * @file ErrorManager.h
 * @brief Error manager
 */
class ErrorMgr
{
public:
    /**
     * @brief Main constructor (with no error).
     */
    ErrorMgr() : errMsg("") {}

    /**
     * @brief Indicates whether there is no error.
     * @return True if there is no error, and false otherwise.
     */
    bool empty() const {
        return errMsg.empty();
    }
    /**
     * @brief Indicates whether there is an error message.
     * @return True if there is an error, and false otherwise.
     */
    bool exists() const {
        return not empty();
    }

    /**
     * @brief Cleans the error message (no error).
     */
    void clear() {
        errMsg.clear();
    }

    /**
     * @brief Sets the error message.
     * @param err String that stores the error message.
     */
    void set(const string& err) {
        errMsg = err;
    }

    /**
     * @brief Gets the error message.
     * @return The error message.
     */
    const string& get() const {
        return errMsg;
    }

    /**
     * @brief A reference to be used by other methods to deposit an error.
     * @return A reference to the error message.
     */
    string& get() {
        return errMsg;
    }

private:
    std::string errMsg; // Stores the error message
};

//class Cats_Exception;

/// Macro to check error
#define assertMsg(cond, msg)  if (not (cond)) { throw Cats_Exception(msg); }

/// Macro to assert that the LTS represents an asynchronous circuit
#define assertAsync()    assertMsg(isCircuit(), "The LTS does not specify a circuit.")

class Cats_Exception
{
public:
    Cats_Exception(const string &what) : eMsg (what) {}
    const char * what() const throw() {
        return eMsg.c_str();
    }
private:
    string eMsg;
};

#endif // ERRORMANAGER_H
