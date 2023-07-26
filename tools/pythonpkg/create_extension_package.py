import toml
from pathlib import Path
from shutil import copyfile, rmtree
from textwrap import dedent
from argparse import ArgumentParser
import cibuildwheel.__main__ as cibuildwheel

parser = ArgumentParser()
parser.add_argument('--build', action='store_true')
args = parser.parse_args()


def pyproject(extension_name: str) -> dict:
    module_name = f'duckdb-extension-{extension_name}'
    return {
        'project': {
            'name': module_name,
            'version': '0.1.0',
            'license': {'text': 'MIT'},
            'dependencies': ['duckdb'],
            'entry-points': {'duckdb_extension': {extension_name: f'duckdb_extension_{extension_name}:extension'}},
        },
        'tool': {'setuptools': {'include-package-data': True}, 'cibuildwheel': {'build': "*cp31*"}},
        'build-system': {
            'requires': ["setuptools>=61.0.0", "wheel"],
            'build-backend': "setuptools.build_meta:__legacy__",
        },
    }


def main():
    cwd = Path(__file__).parent
    base = cwd / 'extensions'
    rmtree(base, ignore_errors=True)
    for extension_name in [
        'autocomplete',
        'excel',
        'fts',
        'httpfs',
        'icu',
        'inet',
        'json',
        'parquet',
        'sqlsmith',
        'tpcds',
        'tpch',
        'visualizer',
    ]:
        source = (cwd / '../../build/debug/extension' / extension_name / f'{extension_name}.duckdb_extension').resolve()
        if not source.exists():
            print(source, 'is missing')
            continue

        target = base / extension_name
        module_name = f'duckdb_extension_{extension_name}'
        module = target / module_name
        module.mkdir(parents=True, exist_ok=True)

        with (target / 'pyproject.toml').open('w') as fh:
            toml.dump(pyproject(extension_name), fh)

        with (target / 'setup.py').open('w') as fh:
            fh.write(
                dedent(
                    f'''\
            from setuptools import setup, Extension
            setup(
                ext_modules = [Extension('duckdb-extension-{extension_name}', [])],
            )
            '''
                )
            )

        copyfile(source, module / source.name)
        with (module / '__init__.py').open('w') as fh:
            fh.write(
                dedent(
                    '''
            from glob import iglob
            from os.path import dirname, join

            __all__ = ['extension']

            def extension():
                return next(iglob(join(dirname(__file__), '*.duckdb_extension')))
            '''
                )
            )

        print('templated', extension_name)

        if args.build:
            cibuildwheel.build_in_directory(
                cibuildwheel.CommandLineArguments(
                    platform='linux',
                    archs=None,
                    allow_empty=None,
                    config_file=None,
                    only=None,
                    output_dir=base / 'wheels',
                    package_dir=target,
                    prerelease_pythons=False,
                    print_build_identifiers=False,
                )
            )


if __name__ == '__main__':
    main()
