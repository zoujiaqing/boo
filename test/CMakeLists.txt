add_executable(booTest WIN32 main.cpp)
target_link_libraries(booTest boo logvisor ${BOO_SYS_LIBS})
