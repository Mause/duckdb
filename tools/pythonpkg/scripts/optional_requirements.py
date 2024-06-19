import os
import sys
from subprocess import check_call, CalledProcessError
import argparse

free_threaded = 'free-threading' in sys.version

check_call([sys.executable, '-m', 'pip', 'install', '--upgrade', 'packaging'])

from packaging.requirements import Requirement


def install_package(package, is_optional):
    cmd = [sys.executable, '-m', 'pip', 'install', str(package), '--prefer-binary']
    if package.name in ['pyarrow', 'torch', 'polars', 'numpy'] and free_threaded:
        cmd += ['-i', 'https://pypi.anaconda.org/scientific-python-nightly-wheels/simple', '--pre']
    try:
        check_call(cmd)
    except CalledProcessError:
        if is_optional:
            print(f'WARNING: Failed to install (optional) "{package_name}", might require manual review')
            return
        raise


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Import test dependencies')
    parser.add_argument('--exclude', action='append', help='Exclude a package from installation', default=[])

    args = parser.parse_args()

    # Failing to install this package does not constitute a build failure
    OPTIONAL_PACKAGES = ["pyarrow", "torch", "polars", "adbc_driver_manager", "tensorflow"]

    for package in args.exclude:
        if package not in OPTIONAL_PACKAGES:
            print(f"Unrecognized exclude list item '{package}', has to be one of {', '.join(OPTIONAL_PACKAGES)}")
            exit(1)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    requirements_path = os.path.join(script_dir, '..', 'requirements-dev.txt')

    content = open(requirements_path).read()
    packages = [Requirement(x) for x in content.splitlines() if x]

    result = []
    for package in packages:
        if package.name in args.exclude:
            print(f"Skipping {package_name}, as set by the --exclude option")
            continue
        is_optional = package.name in OPTIONAL_PACKAGES
        install_package(package, is_optional)
