cmake_minimum_required(VERSION 3.16)

project(Overseer LANGUAGES CXX)

# Find the Qt package
find_package(Qt6 COMPONENTS Widgets REQUIRED)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}/src/UIFiles
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set(SOURCES
    src/main.cpp
    src/LoginWindow/login_window.cpp
    src/MainWindow/main_window.cpp
    src/Overseer/overseer.cpp
    src/Informer/informer.cpp
    )

set(UI_FILES
    src/UIFiles/login.ui
    src/UIFiles/main.ui
    )

# Add the executable
add_executable(Overseer ${SOURCES} ${UI_FILES})

target_include_directories(Overseer PRIVATE
    ${CMAKE_BINARY_DIR}
)

# Set the C++ standard
set_target_properties(Overseer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Link Qt libraries
target_link_libraries(Overseer PRIVATE Qt6::Widgets)
