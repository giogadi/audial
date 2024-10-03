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
    return typeName == 'int' or typeName == 'float' or typeName == 'double' or typeName == 'bool' or typeName == 'string' or typeName == 'editor_id'

def getCppTypeName(typeName):
    if typeName == 'string':
        return 'std::string'
    elif typeName == 'editor_id':
        return 'EditorId'
    else:
        return typeName    

# each prop should have name, type, required headers, load code, save code, and
# imgui code
def getProp(propName, propType, isArray, enumType, maybeNamespace, maybeDefault, rawProp):
    outProp = {}
    genImGui = rawProp.get('gen_imgui', True)
    preImGuiStrs = []
    imGuiStr = ''
    postImGuiStrs = []
    outProp['name'] = propName
    outProp['isArray'] = isArray
    outProp['header_includes'] = []
    outProp['src_includes'] = []
    if (maybeDefault is not None):
        outProp['default'] = maybeDefault
    if isArray:
        cppTypeName = getCppTypeName(propType)
        outProp['type'] = 'std::vector<{cppTypeName}>'.format(cppTypeName = cppTypeName)        
        outProp['load'] = 'serial::LoadVectorFromChildNode<{cppTypeName}>(pt, \"{name}\", _{name});'.format(cppTypeName = cppTypeName, name = propName)
        outProp['save'] = 'serial::SaveVectorInChildNode<{cppTypeName}>(pt, \"{name}\", \"array_item\", _{name});'.format(cppTypeName = cppTypeName, name = propName)
        
        preImGuiStrs.append('imgui_util::InputVectorOptions options;')
        
        if propType == 'MidiNoteAndName':
            preImGuiStrs.append('options.removeOnSameLine = true;')
        preImGuiStrs.append('if (ImGui::TreeNode(\"{name}\")) {{'.format(name = propName))
        imGuiStr = 'imgui_util::InputVector(_{name}, options);'.format(name = propName)
        postImGuiStrs.append('ImGui::TreePop();')
        postImGuiStrs.append('}')
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
            imGuiStr = 'ImGui::InputInt(\"{name}\", &_{name});'.format(name = outProp['name'])
        elif propType == 'float':
            outProp['type'] = 'float'
            outProp['load'] = 'pt.TryGetFloat(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['save'] = 'pt.PutFloat(\"{name}\", _{name});'.format(name = outProp['name'])
            imGuiStr = 'ImGui::InputFloat(\"{name}\", &_{name});'.format(name = outProp['name'])
        elif propType == 'double':
            outProp['type'] = 'double'
            outProp['load'] = 'pt.TryGetDouble(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['save'] = 'pt.PutDouble(\"{name}\", _{name});'.format(name = outProp['name'])
            imGuiStr = 'ImGui::InputDouble(\"{name}\", &_{name});'.format(name = outProp['name'])
        elif propType == 'bool':
            outProp['type'] = 'bool'
            outProp['load'] = 'pt.TryGetBool(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['save'] = 'pt.PutBool(\"{name}\", _{name});'.format(name = outProp['name'])
            imGuiStr = 'ImGui::Checkbox(\"{name}\", &_{name});'.format(name = outProp['name'])
        elif propType == 'vec3':
            outProp['type'] = 'Vec3'
            outProp['load'] = 'serial::LoadFromChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            outProp['save'] = 'serial::SaveInNewChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            imGuiStr = 'imgui_util::InputVec3(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['header_includes'].append('\"matrix.h\"')
        elif propType == 'string':
            outProp['type'] = 'std::string'
            outProp['load'] = 'pt.TryGetString(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['save'] = 'pt.PutString(\"{name}\", _{name}.c_str());'.format(name = outProp['name'])
            imGuiStr = 'imgui_util::InputText<128>(\"{name}\", &_{name});'.format(name = outProp['name'])
        elif propType == 'editor_id':
            outProp['type'] = 'EditorId'
            outProp['load'] = 'serial::LoadFromChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            outProp['save'] = 'serial::SaveInNewChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            imGuiStr = 'imgui_util::InputEditorId(\"{name}\", &_{name});'.format(name = outProp['name'])
            outProp['header_includes'].append('\"editor_id.h\"')
        elif propType == 'MidiNoteAndName':
            outProp['type'] = 'MidiNoteAndName'
            outProp['load'] = 'serial::LoadFromChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            outProp['save'] = 'serial::SaveInNewChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            imGuiStr = '_{name}.ImGui()'.format(name = outProp['name'])
            outProp['header_includes'].append('\"midi_note_and_name.h\"')
        elif propType == 'enum':
            if maybeNamespace is None:
                outProp['type'] = enumType
            else:
                outProp['type'] = f'{maybeNamespace}::{enumType}'
            outProp['load'] = 'serial::TryGetEnum(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            outProp['save'] = 'serial::PutEnum(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            imGuiStr = '{enumType}ImGui(\"{labelName}\", &_{name});'.format(enumType = outProp['type'], labelName = enumType, name = outProp['name'])
            if maybeNamespace is None:
                outProp['header_includes'].append('\"enums/{enumType}.h\"'.format(enumType = enumType))
            else:
                outProp['header_includes'].append(f'\"enums/{maybeNamespace}_{enumType}.h\"')
        else:
            outProp['type'] = propType
            outProp['load'] = 'serial::LoadFromChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            outProp['save'] = 'serial::SaveInNewChildOf(pt, \"{name}\", _{name});'.format(name = outProp['name'])
            imGuiStr = '// TODO for user-types'
            outProp['header_includes'].append('\"properties/{name}.h\"'.format(name = propType))
    
    if genImGui:
        outProp['preImgui'] = preImGuiStrs
        outProp['imgui'] = imGuiStr
        outProp['postImgui'] = postImGuiStrs
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
    props = [getProp(rawProp['name'], rawProp['type'], rawProp.get('isArray', False), rawProp.get('enumType', ''), rawProp.get('namespace', None), rawProp.get('default', None), rawProp) for rawProp in dataDict['props']]
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
