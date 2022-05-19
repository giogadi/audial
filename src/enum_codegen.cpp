#include <iostream>
#include <fstream>
#include <cstdio>

#include <vector>
#include <string>
#include <optional>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Need an input file and an output file." << std::endl;
        return 1;
    }

    std::optional<std::string> namespaceName;
    if (argc >= 4) {
        namespaceName = argv[3];
    }


    std::string enumName;
    std::vector<std::string> enumItems;
    {
        std::ifstream inFile(argv[1]);
        if (!inFile.is_open()) {
            std::cerr << "Error opening input file \"" << argv[1] << "\"" << std::endl;
            return 1;
        }

        inFile >> enumName;

        std::string line;        
        while (inFile >> line) {
            enumItems.push_back(line);
        }
    }

    std::string headerFilename = std::string(argv[2]) + ".h";

    {        
        std::ofstream outFile(headerFilename);
        if (!outFile.is_open()) {
            std::cerr << "Error opening output file \"" << headerFilename << "\"" << std::endl;
            return 1;
        }

        outFile << "#pragma once\n\n";

        if (namespaceName.has_value()) {
            outFile << "namespace " << *namespaceName << " {" << std::endl << std::endl;
        }
        outFile << "enum class " << enumName << " : int {" << std::endl;
        for (std::string const& item : enumItems) {
            outFile << "\t" << item << "," << std::endl;
        }
        outFile << "\tCount" << std::endl << "};" << std::endl;

        outFile << "char const* " << enumName << "ToString(" << enumName << " e);" << std::endl;
        outFile << enumName << " StringTo" << enumName << "(char const* s);" << std::endl;

        if (namespaceName.has_value()) {
            outFile << std::endl << "} // namespace " << *namespaceName << std::endl;
        }
    }

    {
        std::string cppFilename = std::string(argv[2]) + ".cpp";
        std::ofstream outFile(cppFilename);
        if (!outFile.is_open()) {
            std::cerr << "Error opening output file \"" << cppFilename << "\"" << std::endl;
            return 1;
        }

        outFile << "#include \"" << headerFilename << "\"" << std::endl << std::endl;
        outFile << "#include <unordered_map>\n#include <string>" << std::endl << std::endl;

        if (namespaceName.has_value()) {
            outFile << "namespace " << *namespaceName << " {" << std::endl << std::endl;
        }

        outFile << "namespace {" << std::endl;
        outFile << "char const* g" << enumName << "Strings[] = {" << std::endl;
        for (int i = 0; i < enumItems.size() - 1; ++i) {
            outFile << "\t\"" << enumItems[i] << "\"," << std::endl;
        }
        outFile << "\t\"" << enumItems.back() << "\"" << std::endl << "};" << std::endl << std::endl;
        
        outFile << "std::unordered_map<std::string, " << enumName << "> const gStringTo" << enumName << " = {" << std::endl;
        for (int i = 0; i < enumItems.size() - 1; ++i) {
            outFile << "\t{ \"" << enumItems[i] << "\", " << enumName << "::" << enumItems[i] << " }," << std::endl;
        }
        outFile << "\t{ \"" << enumItems.back() << "\", " << enumName << "::" << enumItems.back() << " }" << std::endl << "};" << std::endl << std::endl;

        outFile << "} // end namespace" << std::endl << std::endl;

        outFile << "char const* " << enumName << "ToString(" << enumName << " e) {" << std::endl;
        outFile << "\treturn g" << enumName << "Strings[static_cast<int>(e)];" << std::endl;
        outFile << "}" << std::endl << std::endl;

        outFile << enumName << " StringTo" << enumName << "(char const* s) {" << std::endl;
        outFile << "\treturn gStringTo" << enumName << ".at(s);" << std::endl;
        outFile << "}" << std::endl;

        if (namespaceName.has_value()) {
            outFile << std::endl << "} // namespace " << *namespaceName << std::endl;
        }
    }

    {
        std::string cerealHeader = std::string(argv[2]) + "_cereal.h";
        FILE* pFile = fopen(cerealHeader.c_str(), "w");
        if (pFile == nullptr) {
            std::cerr << "Error opening output file \"" << cerealHeader << "\"" << std::endl;
            return 1;
        }

        fprintf(pFile, "#pragma once\n\n");

        fprintf(pFile, "#include \"cereal/cereal.hpp\"\n");
        fprintf(pFile, "#include \"cereal/types/string.hpp\"\n\n");
        
        fprintf(pFile, "#include \"%s\"\n\n", headerFilename.c_str());

        fprintf(pFile, "namespace %s {\n\n", namespaceName.has_value() ? namespaceName->c_str() : "cereal");
        fprintf(pFile, "template<typename Archive>\n");
        fprintf(pFile, "std::string save_minimal(Archive const& ar, %s const& e) {\n", enumName.c_str());
        fprintf(pFile, "\treturn std::string(%sToString(e));\n", enumName.c_str());
        fprintf(pFile, "}\n\n");

        fprintf(pFile, "template<typename Archive>\n");
        fprintf(pFile, "void load_minimal(Archive const& ar, %s& e, std::string const& v) {\n", enumName.c_str());
        fprintf(pFile, "\te = StringTo%s(v.c_str());\n", enumName.c_str());
        fprintf(pFile, "}\n\n");

        fprintf(pFile, "}  // namespace %s\n", namespaceName.has_value() ? namespaceName->c_str() : "cereal");
        fclose(pFile);
    }
    return 0;
}