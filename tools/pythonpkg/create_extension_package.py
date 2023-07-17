import toml
from pathlib import Path
from shutil import copyfile, rmtree
from textwrap import dedent


def pyproject(extension_name: str) -> dict:
    module_name = f'duckdb-extension-{extension_name}'
    return {
        'project': {
            'name': module_name,
            'dynamic': ['version'],
            'license': {'text': 'MIT'},
            'dependencies': ['duckdb'],
            'entry-points': {
                'duckdb_extension': {
                    extension_name: f'duckdb_extension_{extension_name}:extension'
                }
            },
        },
        'tool': {
            'setuptools': {
                'include-package-data': True
            },
            'setuptools_scm': {
                "root": "../../../..",
            }
        },
        'build-system': {
            'requires': ["setuptools>=61.0.0", "setuptools_scm[toml]>=6.2", "wheel"],
            'build-backend': "setuptools.build_meta:__legacy__"
        }
    }


def main():
    cwd = Path(__file__).parent
    base = cwd / 'extensions'
    rmtree(base, ignore_errors=True)
    for extension_name in ['httpfs']:
        source = (cwd / '../../build/debug/extension' / extension_name/ f'{extension_name}.duckdb_extension').resolve()
        assert source.exists(), source

        target = base / extension_name
        module_name = f'duckdb_extension_{extension_name}'
        module = target / module_name
        module.mkdir(parents=True, exist_ok=True)

        with (target / 'pyproject.toml').open('w') as fh:
            toml.dump(pyproject(extension_name), fh)

        with (target / 'setup.py').open('w') as fh:
            fh.write(dedent('''
            from setuptools import setup
            setup()
            '''
            ))

        copyfile(source, module / source.name)
        with (module / '__init__.py').open('w') as fh:
            fh.write(dedent('''
            from glob import iglob
            from os.path import dirname, join

            __all__ = ['extension']

            def extension():
                return next(iglob(join(dirname(__file__), '*.duckdb_extension')))
            '''
            ))


if __name__ == '__main__':
    main()
