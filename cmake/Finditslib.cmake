add_library(itslib STATIC itslib/src/vectorutils.cpp itslib/src/stringutils.cpp itslib/src/debug.cpp)
target_include_directories(itslib PUBLIC itslib/include)
target_compile_definitions(itslib PUBLIC _NO_RAPI _NO_WINDOWS)
target_include_directories(itslib PUBLIC  ${Boost_INCLUDE_DIRS})
