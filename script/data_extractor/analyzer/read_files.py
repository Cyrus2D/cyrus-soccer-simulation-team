from msilib.schema import Error
from cv2 import normalize
from nomalizer import normalize_key_value


read_ball_pos = True
read_players_pos = True
read_with_normalize = True
read_out_unum = True


def read_a_file(file_name: str):
    lines = open(file_name, 'r').read().split("\n")

    headers = lines[0].split('\n')
    print(headers)

    data_x, data_y = [], []

    for line in lines:
        values = line.split(',')
        print(values)

        for i, v in enumerate(values):
            key = headers[i]
            value = normalize_key_value(key, float(v)) if read_with_normalize else float(v)

            if key.startswith("ball"):  # it is ball
                key_sp = key.split('_')
                # position(maybe relative) or vel
                if key_sp[-1] == 'x' or key_sp == 'y':
                    if key_sp[-2] == 'pos':  # absolute position
                        if read_ball_pos:
                            data_x.append(value)
                    elif key_sp[-2] == 'vel':  # vel
                        pass
                    elif key_sp[-2] == 'kicker':  # relative pos
                        pass
                    else:
                        raise Exception("ball position no match found")

            elif key[0] == 'p':  # it is player
                key_sp = key.split('_')
                # position(maybe relative) or vel
                if key_sp[-1] == 'x' or key_sp == 'y':
                    if key_sp[-2] == 'pos':  # absolute position
                        if read_players_pos:
                            data_x.append(value)
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
                if key_sp[-1] == 'unum': # out unum
                    if read_out_unum:
                        data_y.append(v)
                else:
                    pass
            else:
                pass
