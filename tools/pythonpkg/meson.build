project('duckdb', 'cpp', version: '1.0.0')

py = import('python').find_installation('python3')
pybind11 = dependency('pybind11')

root = meson.current_source_dir() + '/../..'
build_dir = root + '/build/debug'
third_party_include = '../../third_party'
cc = meson.get_compiler('cpp')

duckdb_static = declare_dependency(
    dependencies: cc.find_library('duckdb_static', dirs: build_dir + '/src'),
    include_directories: '../../src/include',
)

run_command(
  find_program('make'),
  '-C', root,
  'debug',
  env: {
    'GEN': 'ninja',
    'BUILD_ICU': '1',
    'BUILD_JEMALLOC': '1',
    'BUILD_VISUALIZER': '1',
    'BUILD_FTS': '1',
    'BUILD_TPCDS': '1',
    'BUILD_TPCH': '1',
    'BUILD_PARQUET': '1',
    'BUILD_JSON': '1',
  },
  check: true,
)

jemalloc = declare_dependency(
    dependencies: cc.find_library(
        'jemalloc_extension',
        dirs: build_dir + '/extension/jemalloc',
    ),
)
visualizer = declare_dependency(
    dependencies: cc.find_library(
        'visualizer_extension',
        dirs: build_dir + '/extension/visualizer',
    ),
)
fts = declare_dependency(
    dependencies: cc.find_library(
        'fts_extension',
        dirs: build_dir + '/extension/fts',
    ),
)
tpch = declare_dependency(
    dependencies: cc.find_library(
        'tpch_extension',
        dirs: build_dir + '/extension/tpch',
    ),
)
tpcds = declare_dependency(
    dependencies: cc.find_library(
        'tpcds_extension',
        dirs: build_dir + '/extension/tpcds',
    ),
)
icu = declare_dependency(
    dependencies: cc.find_library(
        'icu_extension',
        dirs: build_dir + '/extension/icu',
    ),
)
parquet = declare_dependency(
    dependencies: cc.find_library(
        'parquet_extension',
        dirs: build_dir + '/extension/parquet',
    ),
)
json = declare_dependency(
    dependencies: cc.find_library(
        'json_extension',
        dirs: build_dir + '/extension/json',
    ),
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
mbedtls = declare_dependency(
    dependencies: cc.find_library(
        'duckdb_mbedtls',
        dirs: build_dir + '/third_party/mbedtls',
    ),
    # include_directories: [third_party_include + '/mbedtls/include', third_party_include + '/mbedtls/include/mbedtls']
)
fsst = declare_dependency(
    dependencies: cc.find_library(
        'duckdb_fsst',
        dirs: build_dir + '/third_party/fsst',
    ),
)
fastpforlib = declare_dependency(
    dependencies: cc.find_library(
        'duckdb_fastpforlib',
        dirs: build_dir + '/third_party/fastpforlib',
    ),
)
miniz = declare_dependency(
    dependencies: cc.find_library(
        'duckdb_miniz',
        dirs: build_dir + '/third_party/miniz',
    ),
)
pg = declare_dependency(
    dependencies: cc.find_library(
        'duckdb_pg_query',
        dirs: build_dir + '/third_party/libpg_query',
    ),
)

pyduckdb = py.extension_module(
    'duckdb',
    include_directories: ['src/include'],
    dependencies: [
        dependency('python3'),
        pybind11,
        duckdb_static,
        fmt,
        re2,
        utf8proc,
        jemalloc,
        visualizer,
        fts,
        tpch,
        tpcds,
        icu,
        parquet,
        json,
        mbedtls,
        fsst,
        fastpforlib,
        miniz,
        pg,
    ],
    cpp_args: ['-Wno-unused-variable'],
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
        'src/typing/typing.cpp',
    ],
)


test(
    'loading',
    find_program('python3.10'),
    args: ['-c', '__import__("duckdb")'],
)
