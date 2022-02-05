from nomalizer import normalize_key_value
import os
from multiprocessing import Process, Manager


read_ball_pos = True
read_players_pos = True
read_with_normalize = True
read_out_unum = True
number_of_threads = 7


def read_a_file(file_name: str):
    lines = open(file_name, 'r').read().split("\n")

    headers = lines[0].split(',')[:-1]

    data_x, data_y = [], []

    for line in lines[1:]:
        values = line.split(',')[:-1]
        x = []
        y = []

        for i, v in enumerate(values):
            key = headers[i]
            value = (normalize_key_value(key, float(v))
                     if read_with_normalize
                     else float(v))

            if key.startswith("ball"):  # it is ball
                key_sp = key.split('_')
                # position(maybe relative) or vel
                if key_sp[-1] == 'x' or key_sp[-1] == 'y':
                    if key_sp[-2] == 'pos':  # absolute position
                        if read_ball_pos:
                            x.append(value)
                    elif key_sp[-2] == 'vel':  # vel
                        pass
                    elif key_sp[-2] == 'kicker':  # relative pos
                        pass
                    else:
                        raise Exception("ball position no match found")

            elif key[0] == 'p':  # it is player
                key_sp = key.split('_')
                # position(maybe relative) or vel
                if key_sp[-1] == 'x' or key_sp[-1] == 'y':
                    if key_sp[-2] == 'pos':  # absolute position
                        if read_players_pos:
                            x.append(value)
                    elif key_sp[-2] == 'vel':  # vel
                        pass
                    elif key_sp[-2] == 'kicker':  # relative pos
                        pass
                    else:
                        raise Exception("player position no match found")
                else:  # other stoff
                    pass
            elif key.startswith("out"):
                key_sp = key.split("_")
                if key_sp[-1] == 'unum':  # out unum
                    if read_out_unum:
                        y.append(value)
                else:
                    pass
            else:
                pass
            
        data_x.append(x)
        data_y.append(y)

    return data_x, data_y


def read_a_file_multi_process(file_name: str, id, ret_dic):
    lines = open(file_name, 'r').read().split("\n")

    headers = lines[0].split(',')[:-1]

    data_x, data_y = [], []

    for line in lines[1:]:
        values = line.split(',')[:-1]
        x = []
        y = []

        for i, v in enumerate(values):
            key = headers[i]
            value = (normalize_key_value(key, float(v))
                     if read_with_normalize
                     else float(v))

            if key.startswith("ball"):  # it is ball
                key_sp = key.split('_')
                # position(maybe relative) or vel
                if key_sp[-1] == 'x' or key_sp[-1] == 'y':
                    if key_sp[-2] == 'pos':  # absolute position
                        if read_ball_pos:
                            x.append(value)
                    elif key_sp[-2] == 'vel':  # vel
                        pass
                    elif key_sp[-2] == 'kicker':  # relative pos
                        pass
                    else:
                        raise Exception("ball position no match found")

            elif key[0] == 'p':  # it is player
                key_sp = key.split('_')
                # position(maybe relative) or vel
                if key_sp[-1] == 'x' or key_sp[-1] == 'y':
                    if key_sp[-2] == 'pos':  # absolute position
                        if read_players_pos:
                            x.append(value)
                    elif key_sp[-2] == 'vel':  # vel
                        pass
                    elif key_sp[-2] == 'kicker':  # relative pos
                        pass
                    else:
                        raise Exception("player position no match found")
                else:  # other stoff
                    pass
            elif key.startswith("out"):
                key_sp = key.split("_")
                if key_sp[-1] == 'unum':  # out unum
                    if read_out_unum:
                        y.append(value)
                else:
                    pass
            else:
                pass
            
        data_x.append(x)
        data_y.append(y)

    print(len(data_x), len(data_y))
    ret_dic[id] = (data_x, data_y)


def read_folder(path: str):
    os.chdir(path)
    files = [f for f in os.listdir() if not f[0] == '.' and f.split('.')[-1] == 'csv']
    
    X, Y = [], []
    test_data = []
    for file in files:
        print(file)
        x,y = read_a_file(file)
        X += x
        Y += y
        test_data.append((file, x, y))
    print(len(X), len(Y))
    return test_data

def read_folder_multi_thread(path: str):
    os.chdir(path)
    files = [f for f in os.listdir() if not f[0] == '.' and f.split('.')[-1] == 'csv']
    X, Y = [], []
    test_data = []

    batch = len(files)//number_of_threads
    for b in range(batch+1):
        ret_dic = Manager().dict()
        threads = []

        for t in range(number_of_threads):
            index = b*number_of_threads + t
            if index >= len(files):
                break
            file = files[index]
            thread = Process(target=read_a_file_multi_process, args=(file, index,ret_dic))
            thread.start()
            threads.append(thread)
        
        for t in threads:
            t.join()
        
        for id, value in ret_dic.items():
            X += value[0]
            Y += value[1]
            test_data.append((files[id], value[0], value[1]))

    print(len(X), len(Y))
    return test_data


if __name__ == "__main__":
    # x, y = read_a_file("/mnt/f/xxx/2022-02-03-23-56-55_10_KN2C_E6000.csv")
    # print(x)
    # print(y)
    b = read_folder("/mnt/f/xxx/")
    a = read_folder_multi_thread("/mnt/f/xxx/")

    file_name = "2022-02-03-11-51-56_3_KN2C_E6000.csv"
    resa = []
    resb = []
    for i in a:
        if i[0] == file_name:
            resa = (i[1], i[2])
            break
    for i in b:
        if i[0] == file_name:
            resb = (i[1], i[2])
            break
    
    for i in range(len(resa[0])):
        print(resa[0][i])
        print(resb[0][i])