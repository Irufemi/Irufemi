import xml.etree.ElementTree as ET
import os
import uuid

# Namespace used in vcxproj files
NS = "http://schemas.microsoft.com/developer/msbuild/2003"
ET.register_namespace("", NS)

def get_filter_for_path(path_str, is_shader=False):
    path_str = path_str.replace('/', '\\')
    
    if is_shader:
        return "Shaders"
        
    # Exclude filename
    directory = os.path.dirname(path_str)
    
    # If the file is in a higher directory (e.g. ..\) we group it neatly
    if directory.startswith('..\\'):
        # For example, ..\Application_solo\resources\shaders\ParticleGPU.PS.hlsl -> Shaders
        return "External\\" + directory.replace('..\\', '').replace('..', '')
        
    if directory == "":
        return None # Root directory files
        
    return directory

def get_all_parent_filters(filters):
    result = set()
    for f in filters:
        if not f: continue
        parts = f.split('\\')
        for i in range(1, len(parts) + 1):
            result.add('\\'.join(parts[:i]))
    return result

def fix_filters(proj_path):
    print(f"Processing {proj_path}...")
    if not os.path.exists(proj_path):
        print(f"File not found: {proj_path}")
        return
        
    tree = ET.parse(proj_path)
    root = tree.getroot()
    
    file_types = ['ClInclude', 'ClCompile', 'FxCompile', 'None', 'Image', 'Text', 'Natvis']
    
    file_nodes = []
    filters_set = set()
    
    for item_group in root.findall(f"{{{NS}}}ItemGroup"):
        for ft in file_types:
            for node in item_group.findall(f"{{{NS}}}{ft}"):
                inc = node.get('Include')
                if inc:
                    # check if shader
                    is_shader = (ft == 'FxCompile' or inc.endswith('.hlsli') or inc.endswith('.hlsl') or inc.endswith('.hlsli'))
                    f = get_filter_for_path(inc, is_shader)
                    if f:
                        # clean up trailing slashes
                        f = f.strip('\\')
                        if f:
                            filters_set.add(f)
                    file_nodes.append((ft, inc, f))
                    
    all_filters = get_all_parent_filters(filters_set)
    
    # Build new tree
    new_root = ET.Element("Project", ToolsVersion="4.0", xmlns=NS)
    
    filters_item_group = ET.SubElement(new_root, "ItemGroup")
    for f in sorted(list(all_filters)):
        if not f: continue
        filter_node = ET.SubElement(filters_item_group, "Filter", Include=f)
        uid = ET.SubElement(filter_node, "UniqueIdentifier")
        uid.text = "{" + str(uuid.uuid5(uuid.NAMESPACE_URL, proj_path + f)).upper() + "}"
        
    files_item_group = ET.SubElement(new_root, "ItemGroup")
    for ft, inc, f in file_nodes:
        file_node = ET.SubElement(files_item_group, ft, Include=inc)
        if f:
            filter_child = ET.SubElement(file_node, "Filter")
            filter_child.text = f
            
    filters_path = proj_path + ".filters"
    
    # pretty print hack
    def indent(elem, level=0):
        i = "\n" + level*"  "
        if len(elem):
            if not elem.text or not elem.text.strip():
                elem.text = i + "  "
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
            for elem in elem:
                indent(elem, level+1)
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
        else:
            if level and (not elem.tail or not elem.tail.strip()):
                elem.tail = i

    indent(new_root)
    tree = ET.ElementTree(new_root)
    tree.write(filters_path, encoding="utf-8", xml_declaration=True)
    print(f"Generated {filters_path}")

fix_filters("Application_solo/Application_solo.vcxproj")
fix_filters("IrufemiEditor/IrufemiEditor.vcxproj")
fix_filters("IrufemiEngine/IrufemiEngine.vcxproj")
