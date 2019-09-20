'''

Copyright 2019 Applied Research Center for Computer Networks

'''

import argparse
import sys
import json
import pyparsing

from grammar import entries
from to_json import jsonify_entries

parser = argparse.ArgumentParser(description = 'Preprocess *.props files')
parser.add_argument('--verbose', '-v', dest='verbose', action='store_const',
                    const=True, default=False, help="Pretty format json")
parser.add_argument('infile', nargs='?',
                    type=argparse.FileType('r'), default=sys.stdin)
parser.add_argument('outfile', nargs='?',
                    type=argparse.FileType('w'), default=sys.stdout)

args = parser.parse_args()

try:
    toks = entries.parseFile(args.infile, parseAll=True)
    model = jsonify_entries(toks)

    format_args = {}
    if args.verbose:
        format_args = {'sort_keys': True,
                       'indent': 4 }
    json.dump(model, args.outfile, **format_args)
except pyparsing.ParseException as e:
    print("Syntax error: " + str(e), file=sys.stderr)
