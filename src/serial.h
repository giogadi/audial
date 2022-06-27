#pragma once

#include "boost/property_tree/ptree.hpp"

using boost::property_tree::ptree;

namespace serial {
    template <typename T>
    inline void SaveInNewChildOf(ptree& pt, char const* childName, T const& v) {
        ptree childPt;
        v.Save(childPt);
        pt.add_child(childName, childPt);
    }
}