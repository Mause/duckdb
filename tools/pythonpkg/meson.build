project(
    'duckdb',
    'cpp',
    version: configure_file(
        output: 'version',
        capture: true,
        command: [find_program('git'), 'describe', '--tags'],
    ),
)

py = import('python').find_installation('python3')
pybind11 = dependency('pybind11')

root = meson.current_source_dir() + '/../..'
build_dir = root + '/build/debug'
third_party_include = '../../third_party'
cc = meson.get_compiler('cpp')

duckdb_dynamic = declare_dependency(
    dependencies: cc.find_library('duckdb', dirs: build_dir + '/src'),
    include_directories: '../../src/include',
)

fmt = declare_dependency(
    dependencies: cc.find_library(
        'duckdb_fmt',
        dirs: build_dir + '/third_party/fmt',
    ),
    include_directories: third_party_include + '/fmt/include',
)
re2 = declare_dependency(
    dependencies: cc.find_library(
        'duckdb_re2',
        dirs: build_dir + '/third_party/re2',
    ),
    include_directories: [third_party_include + '/re2'],
)
utf8proc = declare_dependency(
    dependencies: cc.find_library(
        'duckdb_utf8proc',
        dirs: build_dir + '/third_party/utf8proc',
    ),
    include_directories: third_party_include + '/utf8proc/include',
)

pyduckdb = py.extension_module(
    'duckdb',
    include_directories: ['src/include'],
    dependencies: [
        dependency('python3'),
        pybind11,
        duckdb_dynamic,
        fmt,
        re2,
        utf8proc,
    ],
    cpp_args: ['-Wno-unused-variable'],
    link_language: 'cpp',
    sources: [
'src/path_like.cpp',
'src/pystatement.cpp',
'src/importer.cpp',
'src/pyresult.cpp',
'src/pyexpression.cpp',
'src/pyfilesystem.cpp',
'src/dataframe.cpp',
'src/map.cpp',
'src/pyconnection.cpp',
'src/pyrelation.cpp',
'src/python_udf.cpp',
'src/python_import_cache.cpp',
'src/numpy/array_wrapper.cpp',
'src/numpy/numpy_result_conversion.cpp',
'src/numpy/numpy_scan.cpp',
'src/numpy/raw_array_wrapper.cpp',
'src/numpy/numpy_bind.cpp',
'src/numpy/type.cpp',
'src/pyduckdb/connection_wrapper.cpp',
'src/jupyter/jupyter_progress_bar_display.cpp',
'src/functional/functional.cpp',
'src/pybind11/pybind_wrapper.cpp',
'src/native/python_objects.cpp',
'src/native/python_conversion.cpp',
'src/pyrelation/initialize.cpp',
'src/common/exceptions.cpp',
'src/pyconnection/type_creation.cpp',
'src/pandas/analyzer.cpp',
'src/pandas/scan.cpp',
'src/pandas/bind.cpp',
'src/pyexpression/initialize.cpp',
'src/arrow/arrow_export_utils.cpp',
'src/arrow/arrow_array_stream.cpp',
'src/typing/pytype.cpp',
'src/typing/typing.cpp'
  ],
)


test(
    'loading',
    find_program('python3.10'),
    args: ['-c', 'print(__import__("duckdb").__version__)'],
)
