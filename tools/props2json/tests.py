
'''

Copyright 2019 Applied Research Center for Computer Networks

'''

import unittest

import grammar as g
import to_json as j

class TestRegexGrammar(unittest.TestCase):

    def parse(self, expr):
        return g.regex.parseString(expr, parseAll=True)

    # Tests good expression without version checks
    def check_common_expr(self, expr):
        toks = self.parse(expr)
        self.assertEqual(expr, "".join(toks[0]))
        return toks[0]

    def test_cppref_examples(self):
        self.check_common_expr('abc|def')
        self.check_common_expr('ab|abc')
        self.check_common_expr('((a)|(ab))((c)|(bc))')
        self.check_common_expr('') # Empty regex
        self.check_common_expr('abc|')
        self.check_common_expr('|abc')
        self.check_common_expr('a[a-z]{2,4}')
        self.check_common_expr('a[a-z]{2,4}?')
        self.check_common_expr('(aa|aabaac|ba|b|c)*')
        self.check_common_expr('^(a+)\\1*,\\1+$')
        self.check_common_expr('(z)((a+)?(b+)?(c))*')
        self.check_common_expr('a$')
        self.check_common_expr('o\\b')
        self.check_common_expr('(?=(a+))')
        self.check_common_expr('(?=(a+))a*b\\1')
        self.check_common_expr('(?=.*[[:lower:]])(?=.*[[:upper:]])(?=.*[[:punct:]]).{6,}')

    def test_whitespace(self):
        self.check_common_expr('   ')
        self.check_common_expr(' |  |   ')
        self.check_common_expr(' ((  )+ | (  )* | ( )){2,3} ')
        self.check_common_expr('\t\\t| ')
        self.check_common_expr('\\uaabb \\xbb')
        self.check_common_expr('( ( ) ( (|  (()| ( | | | |))) ))')
        self.check_common_expr(' {2,} + * ? (?= | ) (?! | ) (?: ( | ) | )')
        self.check_common_expr('^$')

    def test_good(self):
        self.check_common_expr('[^a-z0-9-]')
        self.check_common_expr('[^-a-z@0-9$]')
        self.check_common_expr('a\[\[[^^\[\]]+')
        self.check_common_expr('\cD\cd[\cd]')
        self.check_common_expr('\D[^[:digit:]]\w[_[:alnum:]]\W[^_[:alnum:]]')
        self.check_common_expr('[^[=ru=]-[.cz.]abc]')

    def test_malformed(self):
        def t(expr):
            self.assertRaises(Exception, lambda: self.parse(expr))
        t('+')
        t('{2,3}')
        t('((())')
        t('(?!')
        t('\\ t')
        t('\\ uaabb')
        t('(?!))')

class TestProps(unittest.TestCase):

    def parse(self, expr):
        return g.entries.parseString(expr, parseAll=True)

    def expect(self, expr, result):
        self.assertListEqual(j.jsonify_entries(self.parse(expr)), result)
    
    def test_entry_count(self):
        self.assertEqual(len(self.parse('')), 0)
        self.assertEqual(len(self.parse('{}{}{}')), 3)
        self.assertEqual(len(self.parse('{} manufacturer="{}" {}')), 2)

    def test_propset(self):
        self.expect('{ a: -1, b: "test", true: true, false: false }',
                    [ {'props': { "a": -1, "b": "test", "true": True, "false": False },
                       'selector': {}} ])

    def test_exact(self):
        self.expect('manufacturer = "abc", hwVersion = "cdf" { }',
                    [ {'props': {},
                       'selector': {'manufacturer': { "type": "exact",
                                                       "value": "abc" },
                                    'hwVersion': { "type": "exact",
                                                   "value": "cdf" } } } ])

    def test_fuzzy(self):
        self.expect("swVersion ~ /abc ()(?:())(?v >= 1)/ { }",
                    [ {'props': {},
                       'selector': {'swVersion': { "type": "fuzzy",
                                                  "regex": "abc ()(?:())" + j.VERSION_PATTERN,
                                                  "smatch": { 3: { "check": "version",
                                                                      ">=": "1" }}}}} ] )

if __name__ == '__main__':
    unittest.main()
