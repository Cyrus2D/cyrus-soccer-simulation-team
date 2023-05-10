from os import listdir
from os.path import isfile, join
import os
import sys
import re
import json

path = sys.argv[1]

original_stdout = sys.stdout # Save a reference to the original standard output

if os.path.exists(path):
    print ('Found Folder')
else:
    print ('Wrong Path')
    exit()

confFiles = [f for f in listdir(path) if isfile(join(path, f)) and f.endswith('.conf')]

if not confFiles:
    print("No Conf Files Detected")
    exit()

for filename in confFiles:
    with open(filename) as file:
        cnt = 0
        str_out= ''
        str_out+= '{\n"version" : "0",'
        lines = file.readlines()
        lines = [line for line in lines if line[0] != '#']
        method = lines[0].split()[1]
        str_out+= f'"method" : "{method}",'
        if method == 'Static':
            str_out+= '"role" : [\n{"number" : 1,"name" : "Goalie", "type" : "G", "side" : "C", "pair" : 0},' # CYRUS_LIB
            for i in range(2,11):
                str_out+= f'{{"number" : {i},"name" : "Player", "type" : "MF","side" : "C", "pair" : 0}},' # CYRUS_LIB
            str_out+= '{"number" : 11,"name" : "Player", "type" : "MF","side" : "C", "pair" : 0}],' # CYRUS_LIB
            str_out+= '"data" : [\n{\n"index" : 0,\n"ball" : { "x" :   0.00, "y" :   0.00 },'
            cnt = 0
            for line in lines:
                line = " ".join(line.split())
                words = [w.strip() for w in line.split(' ')]
                try:
                    if float(words[0]) < 12:
                        # print(words)
                        cnt += 1
                        if float(words[0]) < 11:
                            str_out+= f'"{words[0]}" : {{ "x" : {float(words[2])}, "y" :   {float(words[3])} }},'
                        else:
                            str_out+= f'"{words[0]}" : {{ "x" : {float(words[2])}, "y" :   {float(words[3])} }}'
                            if cnt != 11:
                                print(f" ### PROBLEM IN Roles {filename}")
                except:
                    continue
            str_out+='}]\n}'
        elif method == 'DelaunayTriangulation':
            step = 0
            groups = 0
            cnt = 0
            gcnt = 0
            str_out+= '"role" : ['
            for line in lines:
                line = " ".join(line.split())
                words = [w.strip() for w in line.split(' ')]
                if step == 0:
                    if str(words[0]) == 'Begin' and str(words[1]) == 'Roles':
                        step = 1
                elif step == 1:
                    try:
                        if float(words[0]) < 12:
                            # print(words)
                            cnt += 1
                            if words[0] == '1':
                                str_out+= f'{{"number" : 1,"name" : "Goalie", "type" : "G", "side" : "C", "pair" : {words[2]} }},' # CYRUS_LIB
                            elif float(words[0]) < 11:
                                str_out+=f'{{"number" : {words[0]},"name" : "Player", "type" : "MF","side" : "C", "pair" : {words[2]}}},' # CYRUS_LIB
                            else:
                                str_out+=f'{{"number" : {words[0]},"name" : "Player", "type" : "MF","side" : "C", "pair" : {words[2]}}}],' # CYRUS_LIB
                                if cnt != 11:
                                    print(f" ### PROBLEM IN Roles {filename}")
                                cnt = 0
                                step = 2
                    except:
                        if str(words[0]) == 'End' and str(words[1]) == 'Roles':
                            step = 2
                        continue
                elif step == 2:
                    if str(words[0]) == 'Begin' and str(words[1]) == 'Samples':
                        step = 3
                        # print(words)
                        groups = float(words[3])
                        # print("GROUPS" , words[3])
                        str_out+='"data" : ['
                elif step == 3:
                    if str(words[0]) == '-----' and float(words[1]) < groups:
                        # print ('----',gcnt,'----')
                        str_out+=f'{{"index" : {words[1]},'
                        gcnt += 1
                        cnt = 0
                    else:
                        # print(words)
                        # print(f"LOC {gcnt},{cnt}")
                        cnt += 1
                        if cnt == 1:
                            str_out+=f'"ball" : {{ "x" : {words[1]}, "y" :   {words[2]} }},'
                        elif cnt < 12:
                            str_out+=f'"{words[0]}" : {{ "x" : {words[1]}, "y" :   {words[2]} }},'
                        else:
                            if gcnt != groups:
                                str_out+=f'"{words[0]}" : {{ "x" : {words[1]}, "y" :   {words[2]} }} }},'
                            else:
                                str_out+=f'"{words[0]}" : {{ "x" : {words[1]}, "y" :   {words[2]} }} }}]'
                                step = 4
                                cnt = 0
                            if float(words[0]) != 11:
                                print(f"### PROBLEM IN index {gcnt},{cnt}  {filename}")
                elif step == 4:
                    if str(words[0]) == 'End':
                        cnt += 1
                    if cnt == 2:
                        step = 5
                        str_out+="}"
                        # print(f"DONE! {filename}")
        try:
            json_test = json.loads(str_out)
            print(f"{filename} JSON loaded!")
            str_out= json.dumps(json_test, indent=2)
            with open(f'json/{filename}', 'w') as f:
                f.write(str_out)
        except:
            # print(str_out)
            print(f" ### {filename} JSON problem!")

