import sys
import json
from os.path import exists
from glob import glob
from dataclasses import dataclass

path = "/home/me/dotfiles/local/pipx/venvs/meson/lib/python3.10/site-packages"
assert exists(path)
sys.path.insert(0, path)

import mesonbuild.rewriter
import mesonbuild.mesonmain
from mesonbuild.mparser import StringNode, Token

files = glob("src/**/*.cpp", recursive=True)


sys.argv.append("rewrite")
sys.argv.append("command")
commands = json.dumps(
    [{"type": "tweak", "id": "pyduckdb", "function": "dependency", "operation": "set"}]
)
sys.argv.append(commands)


@dataclass
class AddThing:
    target: tuple
    filename: str

    def accept(self, visitor):
        visitor.visit_StringNode(
            StringNode(
                Token(
                    value=self.filename,
                    filename="",
                    tid=0,
                    line_start=0,
                    lineno=0,
                    colno=0,
                    bytespan=0,
                )
            )
        )


def tweak(rewriter, cmd):
    breakpoint()
    rewriter.process_kwargs(
        {"id": "pybind11", "operation": "set", "function": "dependency"}
    )
    rewriter.print_info()
    target = rewriter.interpreter.assignments[cmd["id"]]
    for key, value in target.args.kwargs.items():
        if key.value == "sources":
            break

    for filename in files:
        node = StringNode(
            Token(
                value=filename,
                filename="meson.build",
                tid=0,
                line_start=0,
                lineno=0,
                colno=0,
                bytespan=0,
            )
        )
        value.args.append(node)

    rewriter.modified_nodes.append(value)


# mesonbuild.mesonmain.main()
mesonbuild.rewriter.Rewriter.functions = property(
    lambda self: {"tweak": lambda cmd: tweak(self, cmd)}
)
mesonbuild.rewriter.Rewriter.functions = mesonbuild.rewriter.Rewriter.functions.setter(
    lambda self, value: print(value)
)
mesonbuild.rewriter.run(
    type(
        "args",
        (),
        {
            "type": "command",
            "verbose": True,
            "json": commands,
            "sourcedir": ".",
            "skip": False,
        },
    )()
)
