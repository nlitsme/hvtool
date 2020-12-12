add_library(regutils STATIC registryutils/regfileparser.cpp registryutils/regvalue.cpp)
target_include_directories(regutils PUBLIC registryutils)
target_link_libraries(regutils PRIVATE itslib)
