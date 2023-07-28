import toml
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


def main():
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
        source = (Path(args.source_folder) / extension_name / f'{extension_name}.duckdb_extension').resolve()
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
            p = ArgumentParser()
            repair.configure_parser(p.add_subparsers())
            repair.execute(p.parse_args(['repair', str(list((target / 'dist').glob('*.whl'))[0])]), p)


if __name__ == '__main__':
    main()
