'''

Copyright 2019 Applied Research Center for Computer Networks

'''

from pyparsing import *

__all__ = [
    'regex', 'versionCheck',
    'strVal', 'intVal', 'boolVal',
    'match', 'selector', 'prop', 'propSet', 'entry', 'entries'
]

# Helpers
point = Literal('.')
exponent = CaselessLiteral('E')
plusorminus = Literal('+') | Literal('-')
number = Word(nums) 
integer = Combine( Optional(plusorminus) + number )
floatnumber = Combine( integer +
                       Optional( point + Optional(number) ) +
                       Optional( exponent + integer )
                     )

versionCheck = Forward()

# Modified ECMAScript regular expression grammar from C++ standard.
# Added (?v ... ) blocks for version checking.
reDisjunction = Forward()
reGroup = Literal("(") + reDisjunction + Literal(")")
reAnonGroup = Literal("(?:") + reDisjunction + Literal(")")
reVersionGroup = Group(Literal("(?v") + versionCheck + Literal(")"))
reAtomEscape = ( Literal("x") + Word(hexnums, exact=2) |
                 Literal("u") + Word(hexnums, exact=4) |
                 Literal("c") + Word(alphas, exact=1) |
                 Word(printables, exact=1) )
reEscape = Combine(Literal("\\") + reAtomEscape)
rePatternChar = CharsNotIn("/^$\.*+?()[]{}|", exact=1) # XXX: '/' is not escaped

reClassAtomNoDash = CharsNotIn("\\]-", exact=1) | reEscape
reClassAtom = ( Literal("-") |
                Literal("[:") + CharsNotIn(".=:") + Literal(":]") |
                Literal("[.") + CharsNotIn(".=:") + Literal(".]") |
                Literal("[=") + CharsNotIn(".=:") + Literal("=]") |
                reClassAtomNoDash )
reClassRanges = Forward()
reNonemptyClassRangesNoDash = Forward()
reNonemptyClassRangesNoDash << ( reClassAtomNoDash + Literal("-") + reClassAtom + reClassRanges |
                                 Or([reClassAtom, reClassAtomNoDash + reNonemptyClassRangesNoDash]) )
reNonemptyClassRanges = (reClassAtom + ( Literal("-") + reClassAtom + reClassRanges |
                                         reNonemptyClassRangesNoDash |
                                         Empty() ))
reClassRanges << ( reNonemptyClassRanges | Empty() )
reChClass = Combine(Literal("[") + Optional(Literal("^")) + reClassRanges + Literal("]"))

reAtom = ( rePatternChar | reEscape | reChClass | Literal(".") |
           reAnonGroup | reVersionGroup | reGroup )
reAssertion = ( Literal("^") | Literal("$") | Literal("\\b") | Literal("\\B") |
                Literal("(?=") + reDisjunction + Literal(")") |
                Literal("(?!") + reDisjunction + Literal(")") )
reQuantifierPrefix = ( Literal("*") | Literal("+") | Literal("?") |
                       Literal("{") +
                       number + Optional(Literal(",") + Optional(number)) +
                       Literal("}") )
reQuantifier = Combine(reQuantifierPrefix + Optional(Literal("?")))
reTerm = reAssertion | reAtom + Optional(reQuantifier)
reAlternative = ZeroOrMore(reTerm).leaveWhitespace()
reDisjunction << (reAlternative + Optional('|' + reDisjunction))
regex = Group(reDisjunction).leaveWhitespace().parseWithTabs()

# Possible selector fields
property = Word( alphas, alphanums )

# Version checker
greaterEq = Literal(">=")
greater = Literal(">")
lessEq = Literal("<=")
less = Literal("<")
equal = Literal("=")
versionCmp = greaterEq | greater | lessEq | less | equal
versionNum = delimitedList(Word(nums), delim=".", combine=True)
versionAssert = Group(versionCmp + versionNum)
versionCheck << Group( OneOrMore(versionAssert) )

regexLiteral = Suppress(Literal("/")) + regex + Suppress(Literal("/"))
applyExact = Literal("=") + QuotedString('"', '\\')
applyFuzzy = Literal("~") + regexLiteral

match = Group(property + (applyExact | applyFuzzy))
selector = Group(delimitedList(match, combine=False) | empty)

def tokenConvert(elem, f):
    return elem.setParseAction(lambda tok: f(tok[0]))
strVal = QuotedString('"', '\\')
intVal = tokenConvert(integer, int)
boolVal = tokenConvert(Literal("true") | Literal("false"), lambda x: x == "true")

propName = Word( alphas, alphanums + '_.' )
propValue = strVal | intVal | boolVal
prop = Group(propName + Suppress(Literal(":")) + propValue)
propSet = delimitedList(prop, combine=False)

entry = Group(selector + nestedExpr("{", "}", propSet))
entries = ZeroOrMore(entry)
