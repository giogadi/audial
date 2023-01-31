if __name__ == "__main__":
    import enum_gen
    import properties_gen
    import sys
    import glob

    enumFiles = glob.glob("codegen/enums/*.json")
    for f in enumFiles:
        enum_gen.generateEnumFromJson(f)

    propsFiles = glob.glob("codegen/properties/*.json")
    for f in propsFiles:
        properties_gen.generatePropsFromJson(f)
