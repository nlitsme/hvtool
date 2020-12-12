add_executable(hvtool hvtool.cpp)
target_link_libraries(hvtool itslib regutils openssl)
target_link_libraries(hvtool ${Boost_regex_LIBRARIES})
target_link_directories(hvtool PUBLIC ${Boost_LIBRARY_DIRS})
