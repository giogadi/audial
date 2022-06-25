#include <string>
#include <iostream>

#include "boost/property_tree/ptree.hpp"
#include <boost/property_tree/xml_parser.hpp>

using boost::property_tree::ptree;

class ISerializable {
public:
    virtual void save(boost::property_tree::ptree& pt) const = 0;
    virtual void load(boost::property_tree::ptree const& pt) = 0;
};

struct Foo : public ISerializable {
    virtual void save(boost::property_tree::ptree& pt) const {
        pt.put("x", x);
        pt.put("y", y);
    }
    virtual void load(boost::property_tree::ptree const& pt) {
        x = pt.get<int>("x");
        y = pt.get<std::string>("y");
    }

    int x = 0;
    std::string y;
};

struct Bar : public ISerializable {
    virtual void save(boost::property_tree::ptree& pt) const {
        boost::property_tree::ptree& fTree = pt.add_child("f", boost::property_tree::ptree());
        f.save(fTree);
        pt.put("z", z);
    }
    virtual void load(boost::property_tree::ptree const& pt) {
        boost::property_tree::ptree const& fTree = pt.get_child("f");
        f.load(fTree);
        z = pt.get<float>("z");
    }

    Foo f;
    float z = 3.14f;
};

int main() {
    Bar b;
    b.f.x = 5;
    b.f.y = "howdy";
    b.z = 4.444444f;

    boost::property_tree::ptree pt;
    boost::property_tree::ptree& bTree = pt.add_child("bar", boost::property_tree::ptree());
    b.save(bTree);

    boost::property_tree::xml_parser::xml_writer_settings<std::string> settings(' ', 4);
    boost::property_tree::write_xml("ptree_test.xml", pt, std::locale(), settings);

    {
        boost::property_tree::ptree loadedPt;
        boost::property_tree::read_xml("ptree_test.xml", loadedPt);
        boost::property_tree::ptree const& bTreeLoaded = loadedPt.get_child("bar");
        Bar bLoaded;
        bLoaded.load(bTreeLoaded);
        std::cout << "loaded!" << std::endl;
    }

    {
        ptree listPt;
        ptree& foo = listPt.add_child("list", ptree());
        {
            ptree& foo2 = foo.add_child("item", ptree());
            foo2.put("v", 1.f);

            ptree& foo3 = foo.add_child("item", ptree());
            foo3.put("v", 2.f);

            ptree& foo4 = foo.add_child("item", ptree());
            foo4.put("v", 3.f);

            for (auto const& bar : foo) {
                std::cout << "howdy" << std::endl;
            }
        }
    }

    return 0;
}