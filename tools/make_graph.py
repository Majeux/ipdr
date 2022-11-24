import sys
import os
import re as regex
import pprint
from typing import List, Mapping

# DEBUG = True
DEBUG = False

test = r'''\begin{tabular}
{rr}
 & ipdr \\
avg time & 7.010 s \\
std dev time & 0.217 s \\
max inv constraint & 8 \\
min inv level & 15 \\
min strat marked & 9 \\
min strat length & 21 \\
\end{tabular}
\begin{tabular}
{rrrr}
 & ipdr & control & improvement \\
avg time & 7.010 s & 28.395 s & 75.31 \% \\
std dev time & 0.217 s & 2.289 s &  \\
max inv constraint & 8 & 8 &  \\
min inv level & 15 & 8 & 0.00 \% \\
min strat marked & 9 & 9 &  \\
min strat length & 21 & 22 & 4.55 \% \\
\end{tabular}'''


# read table
def split_tables(tex: str) -> List[str]:
    pattern = r"\\begin\{tabular\}(?:\n)*\{(?:.)*?\}((?:.|\n)*?)\\end\{tabular\}"
    return [x.replace('\n', '') for x in regex.findall(pattern, tex)]


def split_rows(table: str) -> List[List[str]]:
    rv = [x for x in table.split("\\\\") if x != '']
    return rv


Data_t = Mapping[str, Mapping[str, str]]


def parse_rows(rows: List[str]) -> Data_t:
    rv = {"ipdr": {}, "control": {}}

    for row in rows[1:]:
        cells = row.split('&')
        assert len(cells) == 4

        name = cells[0].strip()
        rv["ipdr"][name] = cells[1].strip()
        rv["control"][name] = cells[2].strip()

    return rv

# write plot


def tabbed(s: str) -> str:
    assert s is not None
    return regex.sub("^", "    ", s, flags=regex.MULTILINE)


def escape(s: str) -> str:
    assert s is not None
    return regex.sub("\^", "\\^{}", regex.sub("_", "\\_", s))


def get_time_val(s: str) -> str:
    assert s is not None
    return regex.search("([0-9]+\\.[0-9]+) s", s).group(1)

def name_list(names: List[str]) -> str:
    rv = ""
    for name in names:
        rv += f"    \\texttt{{{name}}},\n"

    return rv


def add_plot(version: str, data: Mapping[str, Data_t]):
    assert version == "ipdr" or version == "control"

    str = ("\\addplot+[\n"
           "    error bars/.cd,\n"
           "    y dir=both,\n"
           "    y explicit\n"
           "] coordinates {\n"
           )

    for name in data:
        time = get_time_val(data[name][version]["avg time"])
        dev = get_time_val(data[name][version]["std dev time"])
        str += f"\t(\\texttt{{{name}}}, {time}) +- (0, {dev})\n"

    str += "};"

    return str


def write_plot(data: Mapping[str, Data_t]) -> str:
    # escape names of data
    data = {escape(key): data[key] for key in data}

    str = "\\begin{figure}\n"
    str += "\\centering\n"
    str += "\\begin{vsplot}{\\cipdr, naive \\cipdr}{\n"
    str += name_list(data.keys())
    str += "}\n"

    str += tabbed(add_plot("ipdr", data)) + "\n"
    str += "\n"
    str += tabbed(add_plot("control", data)) + "\n"

    str += "\\end{vsplot}\n"
    str += "\\end{figure}"

    return str

# main matter


def get_data(dir: str, sub: str, run: str) -> Data_t:
    file_path = os.path.join(dir, sub, f"{sub}{run}", f"{sub}{run}.tex")
    if DEBUG:
        print(file_path)
    assert os.path.isfile(file_path)
    with open(file_path, 'r') as file:
        raw = file.read()
    return raw

if __name__ == "__main__":
    assert len(sys.argv) >= 3

    folder = sys.argv[1]
    run = sys.argv[2]
    print(f"folder: {folder}")
    print(f"content: {os.listdir(folder)}")
    print()

    groups = [g.split(",") for g in sys.argv[3:]]
    print(f"groups: {groups}")
    print()

    for G in groups:
        if DEBUG:
            print(f"group: {G}")
        group_data = {}

        for x in G:
            file_content = get_data(folder, x, run)
            table_contents = split_tables(file_content)

            if DEBUG:
                print("tables")
                pprint.pprint(table_contents)
                print()

            table = [split_rows(t) for t in table_contents]

            if DEBUG:
                print("rows")
                pprint.pprint(table)
                print()

            data = parse_rows(table[1])

            if DEBUG:
                print("data")
                pprint.pprint(data)
                print()

            group_data[x] = data

        plot = write_plot(group_data)

        if DEBUG:
            print("plot")
        print(plot)
        print()
