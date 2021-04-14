add_executable(test-alpha-lib test-overlay_alpha.cpp)
target_link_libraries(test-alpha-lib
    PUBLIC
        ARM-NEON-Compositor::alpha-lib
        GTest::gtest_main
)

if (TARGET NEON_2_SSE::NEON_2_SSE)
    target_link_libraries(test-alpha-lib PRIVATE NEON_2_SSE::NEON_2_SSE)
    target_compile_options(test-alpha-lib PRIVATE -Wno-deprecated-declarations)
endif()
if (TARGET SSE4)
    target_link_libraries(test-alpha-lib PRIVATE SSE4)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    gtest_discover_tests(test-alpha-lib)
endif()