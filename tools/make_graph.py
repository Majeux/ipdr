#!/usr/bin/env python3

from collections import defaultdict
import sys
import os
import re as regex
import argparse
import pprint
import string
from typing import KeysView, List, Mapping

PEBBLING_TYPES = {"pebbling", "z3pdr", "bmc"}
TYPES = {"pebbling", "z3pdr", "bmc", "peter"}
IPDRS = {"constrain", "relax", "binary_search"}
tools_folder = os.path.dirname(os.path.realpath(__file__))
with open(tools_folder + "/model_order.txt", 'r') as model_file:
    MODELS = [m.rstrip() for m in model_file.readlines()]

parser = argparse.ArgumentParser(
    description="Parse the contents of an output folder and turn them into a pgfplot bar graph."
)

parser.add_argument("--debug", action="store_true")
parser.add_argument(
    "--dir", type=str, required=True,
    help="the \"output\" folder in which to search for the experiment results."
)
parser.add_argument(
    "--custom_path", type=str,
    help="overrides the default folder structure to look for run-folders directly here, relative to --dir."
)
# parser.add_argument(
#     "--format", type=str, required=True,
#     help="the format for the folder and file name of the run: \"{model}-{run-format}\"."
# )
parser.add_argument(
    "--types", type=str, nargs="+", choices=TYPES, required=True,
    help=f"the types of algorithm to search results for in the \"output\" folder\". T = {TYPES}",
    metavar="T"
)
modelgroup = parser.add_mutually_exclusive_group()
modelgroup.add_argument(
    "--allmodels", action="store_const", const=MODELS, dest="models",
    help=f"create a graph for all pebbling models {MODELS}"
)
modelgroup.add_argument(
    "--models", nargs="+", type=str, choices=MODELS,
    help=f"a list of model names (from model_order.txt) for which to gather results. M = {MODELS}",
    metavar="M"
)
parser.add_argument(
    "--procs", type=int,
    help=f"range of Peterson models to gather results for: [2..P]. P = integer",
)
parser.add_argument(
    "--switches", nargs="+", type=int,
    help=f"the amount of switches for each iteration in order (2..P)."
)
parser.add_argument(
    "--reps", type=int, nargs="+", required=True,
    help=f"no. of experiment repetitions. if 1 value: for all. if multiple: separate in order."
)
parser.add_argument(
    "--ipdr", action="store", choices=IPDRS, required=True,
    help=f"which ipdr algorithm to gather for. A = {IPDRS}."
)
parser.add_argument(
    "--control", action="store_const", const="-C",
    help=f"if the experiment was a control run (for peter format)."
)

args = parser.parse_args()
# sort args.models according order in model_order
if len(PEBBLING_TYPES.intersection(args.types)) != 0:
    if not args.models:
        raise argparse.ArgumentError(None, f"{PEBBLING_TYPES} requires either --allmodels or --models")

if "peter" in args.types:
    if not (args.procs and args.switches and args.reps):
        raise argparse.ArgumentError(None, "peter requires --procs and --switches")

if args.models:
    args.models = sorted(args.models, key=MODELS.index)

args.dir = os.path.expanduser(args.dir)

def main():
    print(f"folder:\t\t\t {args.dir}")
    print(f"content:\t\t {os.listdir(args.dir)}")
    print(f"available models:\t {MODELS}")
    print(f"selected models:\t {args.models}")
    print(f"selected procs:\t\t [2..{args.procs}]")
    print(f"selected procs:\t\t {args.switches}")
    print(f"selected algorithm:\t {args.types}")
    print(f"control run:\t\t {args.control}")
    print(f"no. experiment reps:\t {args.reps}")
    print()

    model_data = {}
    control = False
    for run in args.types:
        if run == "peter":
            models = [f"{p}procs" for p in range(2, args.procs+1)]
        else:
            models = args.models

        for i, model in enumerate(models):
            if args.debug:
                print(f"model: {model}")

            reps = args.reps[0] if len(args.reps) == 1 else args.reps[i]

            if run == "peter":
                model_folder = f"{model}-peter_{args.switches[i]}switches-ipdr_relax-exp_{reps}{args.control}"
            elif run == "z3pdr":
                model_folder = f"{model}-pebbling-ipdr_{args.ipdr}-exp_{reps}{args.control}"
            else:
                model_folder = f"{model}-{run}-ipdr_{args.ipdr}-exp_{reps}{args.control}"

            if run == "z3pdr":
                control = True

            if control:
                model_folder = add_C_tag(model_folder)

            file_content = get_data(run, model, model_folder, args.custom_path)
            table_contents = split_tables(file_content)

            if args.debug:
                print(f"table (size {len(table_contents)})")
                pprint.pprint(table_contents)
                print()

            table = [split_rows(t) for t in table_contents]

            if args.debug:
                print("rows")
                pprint.pprint(table)
                print()

            if control:
                data = parse_rows_single(run, table)
            else:
                data = parse_rows_combined(run, table)

            if args.debug:
                print("data")
                pprint.pprint(data)
                print()

            model_data[model] = data

        plot = bar_data(model_data)

        if args.debug:
            print("plot")
        print(plot)
    return 0


# READ LATEX TABLE
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


def split_tables(tex: str) -> List[str]:
    pattern = r"\\begin\{tabular\}(?:\n)*\{(?:.)*?\}((?:.|\n)*?)\\end\{tabular\}"
    return [x.replace('\n', '') for x in regex.findall(pattern, tex)]


def split_rows(table: str) -> List[str]:
    rv = [x for x in table.split("\\\\") if x != '']
    return rv


Data_t = Mapping[str, Mapping[str, str]]


def parse_rows_single(type: str, table: List[List[str]]) -> Data_t | None:
    if len(table) == 0:
        return None

    rows = table[0]
    rv = {type: {}}

    for row in rows[1:]:
        cells = row.split('&')
        assert len(cells) == 2

        name = cells[0].strip()
        rv[type][name] = cells[1].strip()

    return rv


def parse_rows_combined(type: str, table: List[List[str]]) -> Data_t | None:
    if len(table) == 0:
        return None

    rows = table[1]
    rv = {type: {}, "control": {}}

    for row in rows[1:]:
        cells = row.split('&')
        assert len(cells) == 4

        name = cells[0].strip()
        rv[type][name] = cells[1].strip()
        rv["control"][name] = cells[2].strip()

    return rv

# write plot


def tabbed(s: str) -> str:
    assert s is not None
    return regex.sub("^", "    ", s, flags=regex.MULTILINE)


def escape(s: str) -> str:
    assert s is not None
    return regex.sub("\\^", "\\^{}", regex.sub("_", "\\_", s))


def add_C_tag(s: str):
    assert s is not None
    return regex.sub("(_[0-9]+)$", r"_C\1", s)


def get_time_val(s: str) -> str:
    assert s is not None
    result = regex.search("([0-9]+\\.[0-9]+) s", s)
    if not result == None:
        return result.group(1)
    return "???"


def name_list(names: KeysView[str]) -> str:
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


def bar_data(data: Mapping[str, Data_t]) -> str:
    data = {escape(key): data[key] for key in data}
    output = defaultdict(lambda: defaultdict(tuple))

    str = "PGFPLOT DATA\n"
    for model in data:
        if data[model] is None:
            continue
        for type in data[model]:
            time = get_time_val(data[model][type]["avg time"])
            dev = get_time_val(data[model][type]["std dev time"])
            output[type][model] = (time, dev)

    for type in output:
        str += type + "\n"
        for i, model in zip(string.ascii_lowercase, data):
            if model in output[type]:
                time, dev = output[type][model]
                str += f"({i}, {time}) +- (0, {dev})\t% {model}\n"
            else:
                str += "???"
        str += "\n"

    return str

# main matter


def get_data(sub_dir: str, model: str, model_dir: str, cpath: str) -> str:
    if cpath:
        file_path = os.path.join(
            args.dir, cpath, model_dir, f"{model_dir}.tex")
    else:
        file_path = os.path.join(
            args.dir, sub_dir, model, model_dir, f"{model_dir}.tex")

    if args.debug:
        print(file_path)
    with open(file_path, 'r') as file:
        raw = file.read()
    return raw


if __name__ == "__main__":
    main()
