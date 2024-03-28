project(x-stitch-editor)

add_executable(${PROJECT_NAME} src/main.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

add_subdirectory(third_party/nanogui)
target_link_libraries(${PROJECT_NAME} PRIVATE nanogui)