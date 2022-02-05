def convertor_x(x):
    return min(max((x + 52.5) / 105.0, 0.0), 1.0)


def convertor_y(y):
    return min(max((y + 34) / 68.0, 0.0), 1.0)


def convertor_dist(dist):
    return dist / 123.0


def convertor_dist_x(dist):
    return dist / 105.0


def convertor_dist_y(dist):
    return dist / 68.0


def convertor_angle(angle):
    return (angle + 180.0) / 360.0


def convertor_type(type):
    return type / 18.0


def convertor_cycle(cycle):
    return cycle / 6000.0


def convertor_bv(bv):
    return bv / 3.0


def convertor_bvx(bvx):
    return (bvx + 3.0) / 6.0


def convertor_bvy(bvy):
    return (bvy + 3.0) / 6.0


def convertor_pv(pv):
    return pv / 1.5


def convertor_pvx(pvx):
    return (pvx + 1.5) / 3.0


def convertor_pvy(pvy):
    return (pvy + 1.5) / 3.0


def convertor_unum(unum):
    return unum / 11.0


def convertor_card(card):
    return card / 2.0


def convertor_stamina(stamina):
    return stamina / 8000.0


def convertor_counts(count):
    return count


def normalize_key_value(k: str, v):
    if k[-1] == 'x':
        if k[-2] == 'pos':  # absolute position
            return convertor_x(v)
    elif k[-1] == 'y':
        if k[-2] == 'pos':  # absolute position
            return convertor_y(v)
    elif k.endswith("unum") or k.endswith("unum_index"):
        return convertor_unum(v)
    else:
        raise Exception("not handled normalizer")
