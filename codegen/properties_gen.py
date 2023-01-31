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

# def isUserPropType(propType):
#     return propType != 'int' and propType != 'string'

# each prop should have name, type, required headers, load code, save code, and
# imgui code
def getPropFromRawProp(rawProp):
    outProp = {}
    outProp['name'] = rawProp['name']
    rawPropType = rawProp['type']
    if rawPropType == 'int':
        outProp['type'] = 'int'
        outProp['load'] = 'pt.TryGetInt(\"{name}\", &_{name});'.format(name = outProp['name'])
        outProp['save'] = 'pt.PutInt(\"{name}\", _{name});'.format(name = outProp['name'])
        outProp['imgui'] = 'ImGui::InputInt(\"{name}\", &_{name});'.format(name = outProp['name'])        
    elif rawPropType == 'string':
        outProp['type'] = 'std::string'
        outProp['load'] = 'pt.TryGetString(\"{name}\", &_{name});'.format(name = outProp['name'])
        outProp['save'] = 'pt.PutString(\"{name}\", _{name});'.format(name = outProp['name'])
        outProp['imgui'] = 'imgui_util::InputText<128>(\"{name}\", &_{name});'.format(name = outProp['name'])
    else:
        outProp['type'] = rawPropType
        outProp['load'] = 'serial::LoadFromChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
        outProp['save'] = 'serial::SaveInNewChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
        outProp['header'] = 'properties/{name}.h'.format(name = outProp['type'])
    return outProp

def generatePropsFromJson(dataFilename):
    dataFile = open(dataFilename, "r")
    dataFileContents = dataFile.read()
    dataFile.close()
    dataDict = json.loads(dataFileContents)

    # Go through the properties and look for any unrecognized properties. Assume
    # those are user properties to be included.
    # userProps = [prop['type'] for prop in dataDict['props'] if isUserPropType(prop['type'])]
    # print(userProps)
    props = [getPropFromRawProp(rawProp) for rawProp in dataDict['props']]
    print(props)

    # namespacesPrefix = ""
    # if len(dataDict["namespaces"]) > 0:
    #     namespacesPrefix = "_".join(dataDict["namespaces"]) + "_"
    # outputFilenamePrefix = "src/enums/" + namespacesPrefix + dataDict["enumName"]
    # outputFilenamePrefix = "src/properties/" + dataDict["propertiesName"]

    # headerTemplateFilename = "codegen/templates/properties_header.h.jinja"
    # headerOutputFilename = outputFilenamePrefix + ".h"
    # renderFromTemplateFile(headerTemplateFilename, dataDict, headerOutputFilename)

    # dataDict["headerFilename"] = headerOutputFilename

    # sourceTemplateFilename = "codegen/templates/properties_source.cpp.jinja"
    # sourceOutputFilename = outputFilenamePrefix + ".cpp"
    # renderFromTemplateFile(sourceTemplateFilename, dataDict, sourceOutputFilename)

if __name__ == "__main__":
    import sys
    generatePropsFromJson(sys.argv[1])
