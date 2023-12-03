find_package(Catch2 3 REQUIRED)

add_executable(test_midi test_midi.cpp)
target_link_libraries(test_midi PRIVATE chrds Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(test_midi)
