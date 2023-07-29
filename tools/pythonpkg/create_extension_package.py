import toml
from os import PathLike
from pathlib import Path
from shutil import copyfile, rmtree
from textwrap import dedent
from argparse import ArgumentParser
from build.__main__ import main as build
import auditwheel.main_repair as repair

cwd = Path(__file__).parent
base = cwd / 'extensions'

parser = ArgumentParser()
parser.add_argument('--source_folder', required=False, default=cwd / '../../build/debug/extension')
parser.add_argument('--build', action='store_true')
args = parser.parse_args()


def pyproject(extension_name: str) -> dict:
    module_name = f'duckdb-extension-{extension_name}'
    folder_name = f'duckdb_extension_{extension_name}'
    return {
        'project': {
            'name': module_name,
            'version': '0.1.0',
            'license': {'text': 'MIT'},
            'dependencies': ['duckdb'],
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


def auditwheel_repair(target: PathLike) -> None:
    p = ArgumentParser()
    repair.configure_parser(p.add_subparsers())
    ags = p.parse_args(['repair', str(target)])
    ags.func(ags, p)


def group(func):
    def wrapper(*args, **kwargs):
        print(f'::group::{args[0]}')
        try:
            return func(*args, **kwargs)
        finally:
            print('::endgroup::')
    return wrapper


@group
def process_extension(extension_name: str) -> None:
    source = (Path(args.source_folder) / extension_name / f'{extension_name}.duckdb_extension').resolve()
    if not source.exists():
        print(source, 'is missing')
        return False

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
        from setuptools import setup

        setup()
        '''
            )
        )

    copyfile(source, module / source.name)
    with (module / '__init__.py').open('w') as fh:
        fh.write(
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

    if args.build:
        build([str(target), '--wheel'])
        auditwheel_repair(list((target / 'dist').glob('*.whl'))[0])

    return True


def main():
    rmtree(base, ignore_errors=True)
    extension_names = [
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
    ]

    if not any([process_extension(extension_name) for extension_name in extension_names]):
        parser.error("Couldn't process any extensions")


if __name__ == '__main__':
    main()
