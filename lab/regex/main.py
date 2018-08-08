import re

BLACK_TILE = chr(ord('z')+1)

def is_assigned(x):
    return x != '?'

def substr2regex(s, start, stop):
    return "".join(i if is_assigned(i) else '.' for i in s[start:stop])

def process_regex_first(r):
    return re.sub(r"\.+$", ".*", r)

def regex_first(line):
    black_tiles = list(i+2 for i, j in enumerate(line[2:]) if j == BLACK_TILE)

    if len(black_tiles):
        stop = black_tiles[0]
    else:
        stop = len(line)

    unassigned = list(i+2 for i, j in enumerate(line[2:]) if i+2 < stop and not is_assigned(j))
    stops = unassigned + [stop]

    for start in [0, 2]:
        if is_assigned(line[1]):
            if line[1] == BLACK_TILE and start != 2:
                continue
            elif start != 0:
                continue

        for stop in stops:
            r = substr2regex(line, start, stop)
            yield process_regex_first(r)

def regex_second(line):
    black_tiles = list(i+2 for i, j in enumerate(line[2:]) if j == BLACK_TILE)
    if len(black_tiles):
        start = black_tiles[0]+1
        if len(black_tiles) == 1:
            stop = len(line)
        else:
            stop = black_tiles[-1]

        r = substr2regex(line, start, stop)
        yield r
    else:
        # TODO?
        return []


# Testing / Examples            
samples = [
    "?ABC?DEF{",
    "C?ABC{DEF{Z",
    "C?ABC???",
    "??ABC?DE??",
    "C?ABC{DEF?Z",
    "C?????????Z",
]

for s in samples:
    print(">", s)
    print("F:", list(regex_first(s)))
    print("S:", list(regex_second(s)))
    print("")
