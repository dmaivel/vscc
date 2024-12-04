cmake_minimum_required(VERSION 3.5.0)
project(vscc VERSION 0.1.0)
# project(frontend VERSION 0.1.0)

include_directories(${vscc_SOURCE_DIR}/include)

file(GLOB VSCC_GLOB_SRC CONFIGURE_DEPENDS "src/util/*.c" "src/ir/*.c" "src/asm/*.c" "src/asm/codegen/*.c" "src/opt/*.c")

add_library(vscc SHARED ${VSCC_GLOB_SRC})
set_target_properties(vscc PROPERTIES PUBLIC_HEADER include/vscc.h)
set_target_properties(vscc PROPERTIES C_STANDARD 99)

add_executable(frontend examples/codecheck/main.c)
target_link_libraries(frontend vscc)
