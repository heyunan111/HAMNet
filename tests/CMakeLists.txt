add_executable(addr_test addr_test.cc)
target_link_libraries(addr_test HAMNet GTest::GTest GTest::Main)