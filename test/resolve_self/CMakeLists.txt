add_executable(resolve_self main.c)
target_link_libraries(resolve_self dynload_s)
set_target_properties(resolve_self PROPERTIES ENABLE_EXPORTS on)

