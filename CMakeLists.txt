cmake_minimum_required(VERSION 3.1)
# - 3.1 for C_STANDARD

project(Alias-ECS
  VERSION 0.1.0
  DESCRIPTION "Archetype style ECS in c11"
  HOMEPAGE_URL "https://github.com/MysticalUnicat/alias-ecs"
  LANGUAGES C
)

option(ALIAS_ECS_BUILD_DOCS "Build documents if Doxygen is found" ON)
option(ALIAS_ECS_BUILD_TESTS "Build tests for Alias-ECS" ON)
option(ALIAS_ECS_COVERAGE "Build tests and generate reports for coverage" ON)

add_library(coverage_config INTERFACE)
if(ALIAS_ECS_COVERAGE AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    target_link_options(coverage_config INTERFACE --coverage)
  else()
    target_link_libraries(coverage_config INTERFACE --coverage)
  endif()
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_library(alias-ecs STATIC
  src/component.c
  src/entity.c
  src/instance.c
  src/layer.c
  src/memory.c
)

set_property(TARGET alias-ecs PROPERTY C_STANDARD 11)
target_include_directories(alias-ecs
  PUBLIC SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(alias-ecs PUBLIC coverage_config)

if(ALIAS_ECS_BUILD_TESTS)
  set(TEST_SOURCES test/main.c)

  enable_testing()
  macro(alias_ecs_test NAME)
    add_test(NAME ${NAME} COMMAND alias-ecs-test --test "${NAME}")
    list(APPEND TEST_SOURCES "test/${NAME}.c")
  endmacro()

  alias_ecs_test(create_instance)
  alias_ecs_test(create_layer)
  alias_ecs_test(create_component)

  add_executable(alias-ecs-test ${TEST_SOURCES})
  target_link_libraries(alias-ecs-test alias-ecs)
endif()

if(ALIAS_ECS_BUILD_DOCS)
  find_package(Doxygen)

  if(DOXYGEN_FOUND)
    set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
    set(DOXYGEN_GENERATE_MAN YES)
    set(DOXYGEN_QUIET YES)

    doxygen_add_docs(alias-ecs-docs
      include/alias/ecs.h
      ALL
    )
  endif()
endif()

