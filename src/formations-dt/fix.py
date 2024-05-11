basefilename = "F433_defense-formation.conf"
nofilename = "F433_defense-formation_no6.conf"
base = open(basefilename, "r").readlines()
no = open(nofilename, "r").readlines()
pnumber = 6

# count lines in base contains "index"
index_count = 0
for line in base:
    if "index" in line:
        index_count += 1
        
def find_index_line(ar, index):
    for i in range(len(ar)):
        if "index" in ar[i] and str(index) in ar[i]:
            return i
    return -1

def update(index):
    base_index_line = find_index_line(base, index)
    if base_index_line == -1:
        print("index not found in base")
        return
    no_index_line = find_index_line(no, index)
    if no_index_line == -1:
        print("index not found in no")
        return
    p_base_line = -1
    for i in range(base_index_line, len(base)):
        if "\"6\" : {" in base[i]:
            p_base_line = i
            break
    if p_base_line == -1:
        print("p not found in base")
        return
    p_no_line = -1
    for i in range(no_index_line, len(no)):
        if "\"6\" : {" in no[i]:
            p_no_line = i
            break
    if p_no_line == -1:
        print("p not found in no")
        return
    no[p_no_line] = base[p_base_line]
    
for i in range(index_count + 1):
    update(i)

with open("F433_defense-formation_no6_new.conf", "w") as f:
    f.write("\n".join(no))
    