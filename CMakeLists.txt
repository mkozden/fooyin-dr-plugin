cmake_minimum_required(VERSION 3.24)

project(fooyin_dr_meter VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

find_package(Fooyin 0.12.6 CONFIG REQUIRED)

set(DRMETER_PLUGIN_INSTALL_DIR "${FOOYIN_PLUGIN_INSTALL_DIR}" CACHE PATH "fooyin plugin installation directory")
set(FOOYIN_PLUGIN_INSTALL_DIR "${DRMETER_PLUGIN_INSTALL_DIR}")

create_fooyin_plugin(
    drmeter
    JSON_IN src/drmeter.json.in
    DEPENDS Fooyin::Gui
    SOURCES src/dranalyzer.cpp
            src/dranalyzer.h
            src/drmeterplugin.cpp
            src/drmeterplugin.h
            src/drresultsdialog.cpp
            src/drresultsdialog.h
            src/drscanner.cpp
            src/drscanner.h
            src/drtags.cpp
            src/drtags.h
)

target_compile_features(drmeter PRIVATE cxx_std_23)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    add_executable(dranalyzertest tests/dranalyzertest.cpp src/dranalyzer.cpp)
    target_include_directories(dranalyzertest PRIVATE src)
    target_link_libraries(dranalyzertest PRIVATE Qt6::Core Qt6::Test)
    target_compile_features(dranalyzertest PRIVATE cxx_std_23)
    set_target_properties(dranalyzertest PROPERTIES AUTOMOC ON)
    add_test(NAME dranalyzertest COMMAND dranalyzertest)

    add_executable(drtagstest tests/drtagstest.cpp src/drtags.cpp)
    target_include_directories(drtagstest PRIVATE src)
    target_link_libraries(drtagstest PRIVATE Fooyin::Core Qt6::Test)
    target_compile_features(drtagstest PRIVATE cxx_std_23)
    set_target_properties(drtagstest PROPERTIES AUTOMOC ON)
    add_test(NAME drtagstest COMMAND drtagstest)

    add_executable(pluginloadtest tests/pluginloadtest.cpp)
    target_link_libraries(pluginloadtest PRIVATE Fooyin::Core Qt6::Core Qt6::Test)
    target_compile_features(pluginloadtest PRIVATE cxx_std_23)
    set_target_properties(pluginloadtest PROPERTIES AUTOMOC ON)
    add_test(NAME pluginloadtest
             COMMAND ${CMAKE_COMMAND} -E env DRMETER_PLUGIN_PATH=$<TARGET_FILE:drmeter> $<TARGET_FILE:pluginloadtest>)
endif()
