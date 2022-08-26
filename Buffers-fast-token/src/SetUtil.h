#ifndef _SETUTIL_H__
#define _SETUTIL_H__

#include <algorithm>
#include <iterator>
#include <set>

using namespace std;

/// Operations on sets of integers
using setInt = set<int>;

/**
 * @class setOp
 * @author Jordi Cortadella
 * @date 18/04/18
 * @file SetUtil.h
 * @brief Class to group set operations
 */
class setOp
{
public:

    /// Set Intersection
    static setInt setIntersection(const setInt& S1, const setInt& S2) {
        setInt R;
        set_intersection(S1.begin(),S1.end(),S2.begin(),S2.end(),
                         std::inserter(R,R.begin()));
        return R;
    }

    /// Set Union
    static setInt setUnion(const setInt& S1, const setInt& S2) {
        setInt R = S1;
        R.insert(S2.begin(), S2.end());
        return R;
    }

    /// Set Difference: S \ notS
    static setInt setDifference(const setInt& S, const setInt& notS) {
        setInt R;
        std::set_difference(S.begin(), S.end(), notS.begin(), notS.end(),
                            std::inserter(R, R.end()));

        return R;
    }

    /// Checks whether two sets are equal (the == operator can also be used)
    static bool setEqual(const setInt& S1, const setInt& S2) {
        return S1 == S2;
    }

    /// Checks whether S1 includes S2
    static bool setIncludes(const setInt& S1, const setInt& S2) {
        return std::includes(S1.begin(), S1.end(), S2.begin(), S2.end());
    }
};


#endif // _SETUTIL_H__
