import toml
from os import PathLike
from pathlib import Path
from shutil import copyfile, rmtree
from textwrap import dedent
from argparse import ArgumentParser
import github_action_utils as gha_utils
from subprocess import check_call, check_output

here = Path(__file__).parent
base = here / 'extensions'

parser = ArgumentParser()
parser.add_argument('--source_folder', required=False, default=here / '../../build/debug/extension')
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


def group(func):
    def wrapper(*args, **kwargs):
        with gha_utils.group(args[0]):
            return func(*args, **kwargs)

    return wrapper


@group
def process_extension(extension_name: str) -> None:
    source = (Path(args.source_folder) / extension_name / f'{extension_name}.duckdb_extension').resolve()
    if not source.exists():
        gha_utils.warning(f'{source} is missing')
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
        check_call(['pyproject-build', target, '--wheel'])
        wheel = list((target / 'dist').glob('*.whl'))[0]
        gha_utils.warning(f'Okay, built. Now lets repair {wheel}')
        check_call(['auditwheel', 'repair', wheel])

    return True


def main():
    print(check_output(['pyproject-build', '--version'], text=True))
    print(check_output(['auditwheel', '--version'], text=True))

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
