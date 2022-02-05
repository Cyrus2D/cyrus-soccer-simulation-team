import math
import os


class Player:
    def __init__(self, unum, side, data_dic):
        self.side = side
        start_key = f'p_l_{unum}_' if side == -1 else f'p_r_{unum}_'
        self.data = [data_dic[k] for k in data_dic.keys() if k.startswith(start_key)]
        self.unum = round(float(data_dic[start_key + 'unum']) * 11)
        if side == -1:
            self.is_kicker = round(float(data_dic[start_key + 'is_kicker']))
        self.x = float(data_dic[start_key + 'pos_x'])
        self.y = float(data_dic[start_key + 'pos_y'])

    def sorter(self, func, kicker_first=False):
        if kicker_first:
            if self.is_kicker == 1:
                return -1000
        if func == 'x':
            return self.x
        if func == 'unum':
            return self.unum
        if func == 'fe':
            x = self.x * 105 - 52.5
            y = self.y * 68 - 34
            diff = math.sqrt(math.pow(52.5 - x, 2) + math.pow(y, 2))
            return x + max(0.0, 40.0 - diff)


class Out:
    def __init__(self, data_dic):
        self.unum = int(data_dic['out_unum'])
        self.index = int(data_dic['out_unum_index'])
        self.data = {k: data_dic[k] for k in data_dic.keys() if k.startswith('out_')}

    def list(self):
        line = [str(v) for v in self.data.values()]
        return line


class State:
    def __init__(self, data_dic):
        self.our = []
        self.their = []
        self.ball = [data_dic[k] for k in data_dic.keys() if k.startswith('ball') or k.startswith('cycle') or k.startswith('offside_count')]
        for side in [-1, 1]:
            for i in range(1, 12):
                if side == -1:
                    self.our.append(Player(i, side, data_dic))
                else:
                    self.their.append(Player(i, side, data_dic))
        self.out = Out(data_dic)

    def __str__(self):
        line = ','.join(self.ball) + ','
        for p in self.our:
            line += ','.join(p.data) + ','
        for p in self.their:
            line += ','.join(p.data) + ','
        line += ','.join(self.out.list())
        return line


def sorter(state: State, our_sort_func, their_sort_func, first_kicker):
    state.our.sort(key=lambda p: p.sorter(our_sort_func, first_kicker))
    state.their.sort(key=lambda p: p.sorter(their_sort_func))
    target_unum = state.out.unum
    target_index = 0
    for i in range(len(state.our)):
        if state.our[i].unum == target_unum:
            target_index = i + 1
    state.out.out_unum_index = target_index
    state.out.data['out_unum_index'] = target_index
    return state


our_sorter = 'fe'
their_sorter = 'fe'
kicker_first = True


def sort_file(in_path, file_name, out_path):
    try:
        lines = open(in_path + file_name, 'r').readlines()
        print(file_name)
        headers = lines[0].split(',')
        if len(lines) == 1:
            return
        new_lines = []
        for line in lines[1:]:

                values = line.split(',')
                data = {}
                for h, v in zip(headers, values):
                    data[h] = v
                state = State(data)
                state = sorter(state, our_sorter, their_sorter, kicker_first)
                new_lines.append(str(state))
    except:
        return

    out_file = open(out_path + file_name, 'w')
    out_file.write(','.join(headers))
    for l in new_lines:
        out_file.write(l + '\n')

in_path = '/home/nader/data/robo_data/all_data/all_csv/'
out_path = '/home/nader/data/robo_data/f_fe/'
# file = '2021-01-25-13-11-45_4__E6000.csv'
files = os.listdir(in_path)
i = 0.0
i_all = float(len(files))
for f in files:
    if f.endswith('.csv'):
        sort_file(in_path, f, out_path)
    i += 1
    print(i / i_all * 100.0)
