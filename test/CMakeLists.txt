cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# ---- Project ----

project(RiftenDequeTests 
        LANGUAGES CXX
)

# ---- Options ----

option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add dependencies ----

find_package( Threads )

include(../cmake/CPM.cmake)

CPMAddPackage("gh:onqtam/doctest#2.4.5")

if(TEST_INSTALLED_VERSION)
  find_package(RiftenDeque REQUIRED)
else()
  CPMAddPackage(NAME RiftenDeque SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()


# ---- Create executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(RiftenDequeTests "${sources}")

# Link dependencies
target_link_libraries(RiftenDequeTests PUBLIC  doctest::doctest RiftenDeque::RiftenDeque ${CMAKE_THREAD_LIBS_INIT})

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(RiftenDeque INTERFACE -Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(RiftenDeque INTERFACE /W4 /WX)
    target_compile_definitions(RiftenDequeTests PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()

# ---- Add RiftenDequeTests ----

enable_testing()


include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

doctest_discover_tests(RiftenDequeTests)

