option(ENABLE_TESTING "Enable testing" ON)

if (ENABLE_TESTING)
    enable_testing()
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

    file(GLOB_RECURSE TEST_SOURCES "*.cpp")
    foreach (TEST_SOURCE ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

        add_executable(${TEST_NAME} ${TEST_SOURCE})

        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

        target_link_libraries(${TEST_NAME} PRIVATE cppkit)
    endforeach ()
endif ()
