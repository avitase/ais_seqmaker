add_library(catch_main STATIC catch_main.cpp)
target_include_directories(catch_main BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(catch_main PUBLIC Catch2::Catch2)
target_link_libraries(catch_main PRIVATE project_options)

add_executable(
        tests
        tests.cpp
        ${PROJECT_SOURCE_DIR}/src/ais.cpp
        ${PROJECT_SOURCE_DIR}/src/seq.cpp
        ${PROJECT_SOURCE_DIR}/src/sequencer.cpp
        ${PROJECT_SOURCE_DIR}/src/seq_maker.cpp)
target_link_libraries(tests PRIVATE project_warnings project_options catch_main)

catch_discover_tests(
        tests
        TEST_PREFIX
        "unittests."
        REPORTER
        xml
        OUTPUT_DIR
        .
        OUTPUT_PREFIX
        "unittests."
        OUTPUT_SUFFIX
        .xml)