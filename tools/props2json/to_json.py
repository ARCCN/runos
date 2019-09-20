
'''
Copyright 2019 Applied Research Center for Computer Networks

'''

from pyparsing import ParseResults

VERSION_PATTERN = "(\d+(?:[.]\d+)*)"

class UnknownGroupToken(Exception):
    pass

def jsonify_ap_fuzzy(toks):
    regex = ''
    group = 1
    smatch = {}
    for tok in toks:
        if isinstance(tok, ParseResults):
            if tok[0] == "(?v":
                regex += VERSION_PATTERN
                smatch[group] = { 'check': 'version' }
                print(toks[1])
                smatch[group].update(dict(tok[1].asList()))
                group += 1
            else:
                raise UnknownGroupToken(tok[0])
        else:
            regex += tok
            if tok == '(':
                group += 1

    return { 'type': 'fuzzy',
            'regex': regex,
           'smatch': smatch }

def jsonify_ap_exact(toks):
    return { 'type': 'exact',
            'value': toks }

def jsonify_match(toks):
    return (toks[0], { '=': jsonify_ap_exact,
                       '~': jsonify_ap_fuzzy,
                     }[toks[1]](toks[2]) )

def jsonify_entry(toks):
    return { 'selector': dict(map(jsonify_match, toks[0])),
             'props': dict(toks[1].asList()) }

def jsonify_entries(toks):
    return list(map(jsonify_entry, toks))
