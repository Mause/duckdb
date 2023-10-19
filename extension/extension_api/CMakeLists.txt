cmake_minimum_required(VERSION 2.8.12)

project(ExtensionApi)

set(EXTENSION_API_SOURCES extension-api.cpp)

include_directories(include)

set(PARAMETERS "-warnings")
build_static_extension(extension_api STATIC ${EXTENSION_API_SOURCES})
target_compile_options(extension_api_extension PRIVATE -fpermissive)
# build_loadable_extension(extension_api ${PARAMETERS} ${EXTENSION_API_SOURCES})

install(
  TARGETS extension_api_extension
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
