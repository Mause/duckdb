import os
from typing import Union

import libcst as cst
from libcst import FlattenSentinel
from libcst.codemod import VisitorBasedCodemodCommand, CodemodTest

os.environ['LIBCST_PARSER_TYPE'] = 'pure'

import_from = cst.ImportFrom(cst.Name('pytest'), names=[cst.ImportAlias(cst.Name('importorskip'))])


def should_transform(node):
    alias = node.names[0]
    if not isinstance(alias, cst.ImportAlias): return False

    name = alias.name.value

    if not isinstance(name, cst.Name):
        return False

    return name.value == 'pandas'


class FixPandasVisitor(VisitorBasedCodemodCommand):
    def leave_Import(self, old_node: cst.Import, node: cst.Import) -> Union[cst.CSTNode, FlattenSentinel]:
        if any(alias.name.value == "pandas" for alias in node.names):
            # Remove the import
            nodes = [
                import_from,
                cst.Assign(
                    [
                        cst.AssignTarget(cst.Name('pd'))
                    ],
                    cst.Call(
                        func=cst.Name('importorskip'),
                        args=[cst.Arg(cst.SimpleString("'pandas'"))]
                    )
                )]
            return cst.FlattenSentinel(nodes)
        elif should_transform(node):
            target_name = node.names[0].asname.name
            name = node.names[0].name
            nodes = [
                import_from,
                cst.Assign(
                    [cst.AssignTarget(target_name)],
                    cst.Call(
                        func=cst.Name('importorskip'),
                        args=[cst.Arg(cst.SimpleString(
                            '"{}"'.format('.'.join(
                                [
                                    name.value.value,
                                    name.attr.value
                                ]
                            ))
                        ))]
                    )
                )
            ]

            return cst.FlattenSentinel(nodes)
        else:
            # Keep the import
            return node


class Testy(CodemodTest):
    TRANSFORM = FixPandasVisitor

    def test_noop(self):
        before = '''
        import os
        from pytest import importorskip
        pd = importorskip('pandas')
        '''

        self.assertCodemod(before, before)

    def test_replace(self):
        before = '''
        import pandas as pd
        '''
        after = '''
        from pytest import importorskip; pd = importorskip('pandas')
        '''

        self.assertCodemod(before, after)

    def test_complex(self):
        before = '''
        import pandas._testing as pt
        '''
        after = '''
        from pytest import importorskip; pt = importorskip("pandas._testing")
        '''

        self.assertCodemod(before, after)


if __name__ == '__main__':
    # import unittest
    #
    # unittest.main()

    import libcst.tool

    libcst.tool.main('', ['codemod', '-x', 'fix_pandas.FixPandasVisitor', '.'])
