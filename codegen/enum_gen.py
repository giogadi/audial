from jinja2 import Template
import json

def renderFromTemplateFile(templateFilename, dataDict, outputFilename):
    templateFile = open(templateFilename, "r")

    templateFileContents = templateFile.read()
    templateFile.close()

    t = Template(templateFileContents)

    outputFile = open(outputFilename, "w")
    outputFile.write(t.render(dataDict))
    outputFile.close()

def generateEnumFromJson(dataFilename):
    dataFile = open(dataFilename, "r")
    dataFileContents = dataFile.read()
    dataFile.close()
    dataDict = json.loads(dataFileContents)

    namespacesPrefix = ""
    if len(dataDict["namespaces"]) > 0:
        namespacesPrefix = "_".join(dataDict["namespaces"]) + "_"
    outputFilenamePrefix = "src/enums/" + namespacesPrefix + dataDict["enumName"]

    headerTemplateFilename = "codegen/templates/enum_header.h.jinja"
    headerOutputFilename = outputFilenamePrefix + ".h"
    renderFromTemplateFile(headerTemplateFilename, dataDict, headerOutputFilename)

    dataDict["headerFilename"] = headerOutputFilename

    sourceTemplateFilename = "codegen/templates/enum_source.cpp.jinja"
    sourceOutputFilename = outputFilenamePrefix + ".cpp"
    renderFromTemplateFile(sourceTemplateFilename, dataDict, sourceOutputFilename)

    cerealTemplateFilename = "codegen/templates/enum_cereal.h.jinja"
    cerealOutputFilename = outputFilenamePrefix + "_cereal.h"
    renderFromTemplateFile(cerealTemplateFilename, dataDict, cerealOutputFilename)

if __name__ == "__main__":
    import sys
    generateEnumFromJson(sys.argv[1])
