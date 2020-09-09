cmake_minimum_required(VERSION 3.10)

project(VVERSimulator)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(CMAKE_TOOLCHAIN_FILE C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(imgui_DIR C:/src/vcpkg/packages/imgui_x64-windows/share/imgui)

#incluir diretórios com os headers
include_directories("Eigen")

# GLOB permite usar globbing-expressions como "*.cpp" ou "/*/*.h"
# GLOB_RECURSE é como GLOB mas varre os subdiretórios 
file(GLOB SOURCES *.cpp)

#Precisa adicionar WIN32 para não exibir o terminal no fundo
add_executable(main WIN32 ${SOURCES})
find_package(imgui CONFIG REQUIRED)
target_link_libraries(main PRIVATE imgui::imgui)

find_path(STB_INCLUDE_DIRS "stb.h")
target_include_directories(main PRIVATE ${STB_INCLUDE_DIRS})
