cmake_minimum_required(VERSION 3.28)

project(x-stitch-editor)

set(NANOGUI_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON   OFF CACHE BOOL " " FORCE)
set(NANOGUI_INSTALL        OFF CACHE BOOL " " FORCE)

add_subdirectory(third_party/nanogui)
set_property(TARGET glfw glfw_objects nanogui PROPERTY FOLDER "dependencies")
add_definitions(${NANOGUI_EXTRA_DEFS})
include_directories(${NANOGUI_EXTRA_INCS})

add_subdirectory(third_party/tinyxml2)
add_subdirectory(third_party/fmt)
add_subdirectory(third_party/glm)
add_subdirectory(third_party/PDF-Writer)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_compile_definitions(MACOSX_BUNDLE)
set(ICON_NAME "icon.icns")
set(ICON_PATH ${PROJECT_SOURCE_DIR}/assets/${ICON_NAME})
set_source_files_properties(${ICON_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
file(COPY ${ICON_PATH} DESTINATION ${PROJECT_NAME}.app/Contents/Resources)

set(FONT_HELVETICA_PATH ${PROJECT_SOURCE_DIR}/assets/Helvetica.ttf)
set_source_files_properties(${FONT_HELVETICA_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
file(COPY ${FONT_HELVETICA_PATH} DESTINATION ${PROJECT_NAME}.app/Contents/Resources)

set(FONT_HELVETICA_BOLD_PATH ${PROJECT_SOURCE_DIR}/assets/Helvetica-Bold.ttf)
set_source_files_properties(${FONT_HELVETICA_BOLD_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
file(COPY ${FONT_HELVETICA_BOLD_PATH} DESTINATION ${PROJECT_NAME}.app/Contents/Resources)

set(DMC_THREADS_PATH ${PROJECT_SOURCE_DIR}/assets/DMC.xml)
set_source_files_properties(${DMC_THREADS_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
file(COPY ${DMC_THREADS_PATH} DESTINATION ${PROJECT_NAME}.app/Contents/Resources)

set(SYMBOLS_PATH ${PROJECT_SOURCE_DIR}/assets/symbols/)
set_source_files_properties(${SYMBOLS_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
file(COPY ${SYMBOLS_PATH} DESTINATION ${PROJECT_NAME}.app/Contents/Resources/symbols)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${ICON_PATH} ${SOURCES})
# add_executable(${PROJECT_NAME} ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

set_target_properties(${PROJECT_NAME} PROPERTIES
    BUNDLE True
    MACOSX_BUNDLE_GUI_IDENTIFIER uk.brickgrass.${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
    MACOSX_BUNDLE_ICONFILE ${ICON_NAME}
)

target_link_libraries(${PROJECT_NAME} PRIVATE nanogui ${NANOGUI_EXTRA_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE tinyxml2)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm-header-only)
target_link_libraries(${PROJECT_NAME} PRIVATE PDFWriter)
target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreFoundation")
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")