import shlex
import sys
from argparse import ArgumentParser
from importlib import import_module
from pathlib import Path
from shutil import copyfile, rmtree
from subprocess import check_call, check_output
from textwrap import dedent
from typing import cast

import github_action_utils as gha_utils
import setuptools_scm
import toml
from cibuildwheel.options import Options, CommandLineArguments
from cibuildwheel.typing import PlatformName
from cibuildwheel.util import prepare_command

here = Path(__file__).parent
base = here / 'extensions'

parser = ArgumentParser()
parser.add_argument('--source_folder', required=False, default=here / '../../build/debug/extension')
parser.add_argument('--build', action='store_true')
parser.add_argument('--test', action='store_true')
args = parser.parse_args()

version = setuptools_scm.get_version(str(here / '../..'))


def pyproject(extension_name: str) -> dict:
    module_name = f'duckdb-extension-{extension_name}'
    folder_name = f'duckdb_extension_{extension_name}'
    return {
        'project': {
            'name': module_name,
            'version': version,
            'license': {'text': 'MIT'},
            'dependencies': [f'duckdb=={version}'],
            'entry-points': {'duckdb_extension': {extension_name: f'{folder_name}:extension'}},
        },
        'tool': {
            'setuptools': {'package-data': {folder_name: ['*.duckdb_extension']}},
        },
        'build-system': {
            'requires': ['setuptools>=61.0.0', 'wheel'],
            'build-backend': 'setuptools.build_meta',
        },
    }


def process_extension(source: Path) -> None:
    extension_name = source.stem
    target = base / extension_name
    module_name = f'duckdb_extension_{extension_name}'
    module = target / module_name
    module.mkdir(parents=True, exist_ok=True)

    with (target / 'pyproject.toml').open('w') as fh:
        toml.dump(pyproject(extension_name), fh)

    (target / 'setup.py').write_text(
        dedent(
            f'''\
        from setuptools import setup

        setup()
        '''
        )
    )

    copyfile(source, module / source.name)

    (module / '__init__.py').write_text(
        dedent(
            f'''\
        import importlib.resources as pkg_resources

        __all__ = ['extension']

        def extension():
            files = list(pkg_resources.files(__name__).iterdir())
            result = next((f for f in files if f.name.endswith('.duckdb_extension')), None)
            assert result, files
            return pkg_resources.as_file(result)

        '''
        )
    )

    (module / '__main__.py').write_text(
        dedent(
            '''
        from . import extension
        print(extension())
        '''
        )
    )

    print('templated', extension_name)

    if not args.build:
        return

    check_call(['pyproject-build', target, '--wheel'])
    wheel = first((target / 'dist').glob('*.whl'))
    print(f'Okay, built. Now lets repair {wheel}')

    repair_command = get_repair_command(target)
    if repair_command:
        match sys.platform:
            case 'linux':
                tool = 'auditwheel'
            case 'darwin':
                tool = 'delocate'
            case _:
                tool = None

        if tool:
            check_call(['pip', 'install', tool])
        check_call(
            shlex.split(
                prepare_command(repair_command, dest_dir='wheelhouse', wheel=wheel, delocate_archs='x86_64,arm64')
            )
        )
        wheel = first((Path.cwd() / 'wheelhouse').glob(f'duckdb_extension_{extension_name}*.whl'))
    else:
        print('no repair required for this system')

    if not args.test:
        return

    check_call(['pip', 'install', wheel])

    import duckdb
    ext = import_module(f'duckdb_extension_{extension_name}')

    duckdb.load_extension(ext.extension())


def get_repair_command(target):
    cli_args = CommandLineArguments.defaults()
    cli_args.config_file = str(target / 'pyproject.toml')
    platform_name = cast(PlatformName, {'linux': 'linux', 'darwin': 'macos', 'win32': 'windows'}[sys.platform])
    cli_args.package_dir = target
    options = Options(platform=platform_name, command_line_arguments=cli_args, env={})
    build_options = options.build_options('')
    return build_options.repair_command


def first(iterable):
    return next(iter(iterable))


def main():
    print(check_output(['pyproject-build', '--version'], text=True))
    print(check_output(['auditwheel', '--version'], text=True))

    rmtree(base, ignore_errors=True)


    extensions = list(Path(args.source_folder).glob('**/*.duckdb_extension'))
    if not extensions:
        parser.error("Couldn't find any extensions to process")

    for extension in extensions:
        with gha_utils.group(str(extension)):
            process_extension(extension)


if __name__ == '__main__':
    main()
