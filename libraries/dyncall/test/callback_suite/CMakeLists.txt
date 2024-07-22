add_executable(callback_suite globals.c cases.c main.c)
target_link_libraries(callback_suite dyncallback_s dyncall_s)

