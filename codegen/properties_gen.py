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

def isPrimitiveType(typeName):
    return typeName == 'int' or typeName == 'float' or typeName == 'double' or typeName == 'bool' or typeName == 'string'

def getCppTypeName(typeName):
    if typeName == 'string':
        return 'std::string'
    else:
        return typeName    

# each prop should have name, type, required headers, load code, save code, and
# imgui code
def getProp(propName, propType, isArray):
    outProp = {}
    outProp['name'] = propName
    outProp['isArray'] = isArray
    outProp['header_includes'] = []
    outProp['src_includes'] = []
    if isArray:
        cppTypeName = getCppTypeName(propType)
        outProp['type'] = 'std::vector<{cppTypeName}>'.format(cppTypeName = cppTypeName)        
        outProp['load'] = 'serial::LoadVectorFromChildNode<{cppTypeName}>(pt, \"{name}\", _{name});'.format(cppTypeName = cppTypeName, name = propName)
        outProp['save'] = 'serial::SaveVectorInChildNode<{cppTypeName}>(pt, \"{name}\", \"array_item\", _{name});'.format(cppTypeName = cppTypeName, name = propName)
        outProp['imgui'] = 'imgui_util::InputVector(_{name});'.format(name = propName)
        outProp['header_includes'].append('<vector>')
        outProp['src_includes'].append('serial_vector_util.h')
        outProp['src_includes'].append('imgui_vector_util.h')
        if not isPrimitiveType(propType):
            outProp['header_includes'].append('\"properties/{propType}.h\"'.format(propType = propType))
    else:
    
        if propType == 'int':
            outProp['type'] = 'int'
            outProp['load'] = 'pt.TryGetInt(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['save'] = 'pt.PutInt(\"{name}\", _{name});'.format(name = outProp['name'])
            outProp['imgui'] = 'ImGui::InputInt(\"{name}\", &_{name});'.format(name = outProp['name'])
        elif propType == 'float':
            outProp['type'] = 'float'
            outProp['load'] = 'pt.TryGetFloat(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['save'] = 'pt.PutFloat(\"{name}\", _{name});'.format(name = outProp['name'])
            outProp['imgui'] = 'ImGui::InputFloat(\"{name}\", &_{name});'.format(name = outProp['name'])
        elif propType == 'double':
            outProp['type'] = 'double'
            outProp['load'] = 'pt.TryGetDouble(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['save'] = 'pt.PutDouble(\"{name}\", _{name});'.format(name = outProp['name'])
            outProp['imgui'] = 'ImGui::InputDouble(\"{name}\", &_{name});'.format(name = outProp['name'])
        elif propType == 'bool':
            outProp['type'] = 'bool'
            outProp['load'] = 'pt.TryGetBool(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['save'] = 'pt.PutBool(\"{name}\", _{name});'.format(name = outProp['name'])
            outProp['imgui'] = 'ImGui::Checkbox(\"{name}\", &_{name});'.format(name = outProp['name'])
        elif propType == 'string':
            outProp['type'] = 'std::string'
            outProp['load'] = 'pt.TryGetString(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['save'] = 'pt.PutString(\"{name}\", _{name}.c_str());'.format(name = outProp['name'])
            outProp['imgui'] = 'imgui_util::InputText<128>(\"{name}\", &_{name});'.format(name = outProp['name'])
        else:
            outProp['type'] = propType
            outProp['load'] = 'serial::LoadFromChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            outProp['save'] = 'serial::SaveInNewChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            outProp['imgui'] = '// TODO for user-types'
            outProp['header_includes'].append('\"properties/{name}.h\"'.format(name = propType))
            
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
    props = [getProp(rawProp['name'], rawProp['type'], rawProp.get('isArray', False)) for rawProp in dataDict['props']]
    # print(props)
    dataDict['props'] = props;

    dataDict['header_includes'] = set()
    dataDict['src_includes'] = set()
    for prop in props:
        print(prop['header_includes'])
        dataDict['header_includes'].update(prop['header_includes'])
        dataDict['src_includes'].update(prop['src_includes'])

    # namespacesPrefix = ""
    # if len(dataDict["namespaces"]) > 0:
    #     namespacesPrefix = "_".join(dataDict["namespaces"]) + "_"
    # outputFilenamePrefix = "src/enums/" + namespacesPrefix + dataDict["enumName"]
    outputFilenamePrefix = "src/properties/" + dataDict["propertiesName"]

    headerTemplateFilename = "codegen/templates/properties_header.h.jinja"
    headerOutputFilename = outputFilenamePrefix + ".h"
    renderFromTemplateFile(headerTemplateFilename, dataDict, headerOutputFilename)

    dataDict["headerFilename"] = headerOutputFilename

    sourceTemplateFilename = "codegen/templates/properties_source.cpp.jinja"
    sourceOutputFilename = outputFilenamePrefix + ".cpp"
    renderFromTemplateFile(sourceTemplateFilename, dataDict, sourceOutputFilename)

if __name__ == "__main__":
    import sys
    generatePropsFromJson(sys.argv[1])
