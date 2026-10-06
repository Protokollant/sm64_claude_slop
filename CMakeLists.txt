cmake_minimum_required(VERSION 3.15)
project(gmcl_sm64 CXX)
set(CMAKE_CXX_STANDARD 17)

# cmake -S . -B build -DGMOD_MODULE_BASE=/path/to/gmod-module-base -DLIBSM64=/path/to/libsm64
set(GMOD_MODULE_BASE "" CACHE PATH "gmod-module-base checkout (contains include/GarrysMod)")
set(LIBSM64 "" CACHE PATH "libsm64 checkout, already built with its Makefile (dist/ present)")

add_library(gmcl_sm64 SHARED module/sm64_module.cpp)
target_include_directories(gmcl_sm64 PRIVATE ${GMOD_MODULE_BASE}/include ${LIBSM64}/src)
set_target_properties(gmcl_sm64 PROPERTIES PREFIX "")

# GMod binary modules always use the .dll extension, even on Linux. Use *32 names for the 32-bit branch.
if(WIN32)
  if(MSVC)
    target_link_libraries(gmcl_sm64 PRIVATE ${LIBSM64}/dist/sm64.lib)
  else()
    # MinGW-w64 (same toolchain libsm64 uses on Windows): link the DLL directly, bundle the C++ runtime
    target_link_libraries(gmcl_sm64 PRIVATE ${LIBSM64}/dist/sm64.dll -static-libgcc -static-libstdc++)
  endif()
  set_target_properties(gmcl_sm64 PROPERTIES OUTPUT_NAME gmcl_sm64_win64 SUFFIX ".dll")
else()
  target_link_libraries(gmcl_sm64 PRIVATE ${LIBSM64}/dist/libsm64.so)
  set_target_properties(gmcl_sm64 PROPERTIES OUTPUT_NAME gmcl_sm64_linux64 SUFFIX ".dll")
endif()
