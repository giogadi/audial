#include "boost/property_tree/ptree.hpp"

using boost::property_tree::ptree;

// namespace boost {
//     namespace property_tree {
//         class ptree;
//     }
// }

class MyPT {
    MyPT AddChild(char const* name) {
        ptree& child = _pt->add_child(name, ptree());
        MyPT mpt;
        mpt._pt = &child;
        return mpt;
    }
    MyPT GetChild(char const* name) {
        ptree& pt = _pt->get_child(name);
        MyPT mpt;
        mpt._pt = &pt;
        return mpt;
    }
private:
    ptree* _pt;
};

int main() {
    return 0;
}