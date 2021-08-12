import re
import fnmatch
import os
import glob
import sys


def clean_list(l):
    l = [x.strip() for x in l]
    l = [x for x in l if x != ""]
    return l

def enum_line(line):
    l = line.strip()
    elems = clean_list(l.split(','))
    if len(elems) <= 1 or elems[1].strip().startswith("//"):
        return l
    else:
        for idx in range(len(elems)):
            if idx != len(elems) - 1:
                elems[idx] = elems[idx] + ","
        return elems

# Extract enum values from header file
def get_enum_from(header, enum_name):
    matches = glob.glob(header)    
    if len(matches) == 0 or len(matches) > 1:
        print "No single match for header: {0} {1}".format(header, enum_name)
        print "Please specify right path for header"
        exit(1)
    header_file = matches[0]
    content = ""
    with open(header_file) as f:
        content = f.read()
    keyword = "enum class " + enum_name    
    enum_start = content.index(keyword)
    left = content.index("{", enum_start) + 1
    right = content.index("}", left) - 1
    enum_content = content[left:right]
    enum_lines = enum_content.split("\n")
    enum_lines = [enum_line(l) for l in enum_lines]
    
    result = []
    for x in enum_lines:
        if type(x) is list:
            result.extend(x)
        else:
            result.append(x)
    return clean_list(result)


## Compare with header files enum value according marks in wasmedge.h.in
## Generate new wasmedge.h.in if any difference found
def sync_enums(force_update):
    lines = []
    with open('include/api/wasmedge.h.in') as f:
        lines = f.readlines()
    result = []
    cur = 0
    while cur < len(lines):
        line = lines[cur]
        if 'Gen Based:' in line:
            result.append(line)
            header = line.split(':')[-1].strip()
            enum_line = lines[cur+1]
            result.append(enum_line)
            prefix = "WasmEdge_"
            prev = lines[cur-1].strip()
            if "Gen Prev:" in prev:
                prefix = prev.split(':')[-1].strip()
            enum_name = enum_line.replace("enum " + prefix, "").split()[0]
            print "check .... ", enum_name
            num = cur + 2
            values = []
            while num < len(lines):
                l = lines[num]
                if '};' in l:
                    break
                values.append(l.strip())
                num += 1
            header_values = get_enum_from(header, enum_name)
            if len(values) != len(header_values):                
                if not force_update:
                    print "wasmedge.h.in is not synced"
                    print "Please run conf-header.py with option --update and commit changes"
                    exit(1)
                print "update ... ", enum_name
            for v in header_values:
                if v.startswith("//"):
                    result.append("  {0}\n".format(v))
                else:
                    t = "  {0}{1}_{2}\n".format(prefix, enum_name, v)
                    result.append(t)
            cur = num
        else:
            result.append(line)
            cur += 1
    if force_update:
        with open('include/api/wasmedge.h.in', 'w') as f:
            f.writelines(result)

if __name__ == "__main__":
    force_update = False
    if len(sys.argv) >= 2 and sys.argv[1] == '--update':
        force_update = True
    sync_enums(force_update)