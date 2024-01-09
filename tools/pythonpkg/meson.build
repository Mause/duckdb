project('duckdb', 'cpp', version: '0.2.0')

py = import('python').find_installation('python3')
pybind11 = dependency('pybind11')

build_dir = meson.current_source_dir() + '/../../build/debug'
duckdb_include = '../../src/include'
third_party_include = '../../third_party'
cc = meson.get_compiler('cpp')

fmt_include = third_party_include + '/fmt/include'
fmt = declare_dependency(
  dependencies: cc.find_library('duckdb_fmt', required : true, dirs: build_dir + '/third_party/fmt'),
  include_directories: [fmt_include],
)


duckdb_static = declare_dependency(
dependencies:  cc.find_library('duckdb_static', required : true, dirs: build_dir + '/src'),
  include_directories : include_directories(duckdb_include)
)

re2 = declare_dependency(
dependencies:  cc.find_library('duckdb_re2', required : true, dirs: build_dir + '/third_party/re2'),
  include_directories: [third_party_include + '/re2'],
)
utf8proc = declare_dependency(
dependencies:  cc.find_library('duckdb_utf8proc', required : true, dirs: build_dir + '/third_party/utf8proc'),
  include_directories: [third_party_include + '/utf8proc/include'],
)

pyduckdb = py.extension_module(
  'duckdb',
  include_directories: ['src/include'],
  dependencies: [dependency('python3'), pybind11, duckdb_static, fmt, re2, utf8proc],
  #link_with: [duckdb_static],
  link_language: 'cpp',
  sources: [
    'duckdb_python.cpp', 
    'src/path_like.cpp',
    'src/numpy/array_wrapper.cpp',
    'src/numpy/numpy_result_conversion.cpp',
    'src/numpy/numpy_scan.cpp',
    'src/numpy/raw_array_wrapper.cpp',
    'src/numpy/numpy_bind.cpp',
    'src/numpy/type.cpp',
    'src/pyduckdb/connection_wrapper.cpp',
    'src/importer.cpp',
    'src/pyresult.cpp',
    'src/pyexpression.cpp',
    'src/pyfilesystem.cpp',
    'src/dataframe.cpp',
    'src/jupyter/jupyter_progress_bar_display.cpp',
    'src/functional/functional.cpp',
    'src/map.cpp',
    'src/pybind11/pybind_wrapper.cpp',
    'src/pyconnection.cpp',
    'src/native/python_objects.cpp',
    'src/native/python_conversion.cpp',
    'src/pyrelation/initialize.cpp',
    'src/pyrelation.cpp',
    'src/common/exceptions.cpp',
    'src/pyconnection/type_creation.cpp',
    'src/python_udf.cpp',
    'src/pandas/analyzer.cpp',
    'src/pandas/scan.cpp',
    'src/pandas/bind.cpp',
    'src/pyexpression/initialize.cpp',
    'src/arrow/arrow_export_utils.cpp',
    'src/arrow/arrow_array_stream.cpp',
    'src/python_import_cache.cpp',
    'src/typing/pytype.cpp',
    'src/typing/typing.cpp'
  ]
)

test('loading', find_program('python3.10'), args: ['-c', '__import__("duckdb")'])
