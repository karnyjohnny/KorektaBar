cmake_minimum_required(VERSION 3.20)
project(KorektaBar VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(KorektaBar WIN32
    src/main.cpp
    src/Config.cpp
    src/TextFormatter.cpp
    src/PolishAnalyzer.cpp
    src/SpellCheckerWin.cpp
    src/BackgroundWorker.cpp
    src/D2DEditor.cpp
    src/SettingsDialog.cpp
    src/MainWindow.cpp
    resources/KorektaBar.rc
)

target_include_directories(KorektaBar PRIVATE src resources)
target_compile_definitions(KorektaBar PRIVATE UNICODE _UNICODE NDEBUG)

# Opcjonalne strojenie pod CPU budującej maszyny, np.:
#   cmake -S native -B native/build-cmake -G Ninja -DKOREKTABAR_MARCH="-march=native"
set(KOREKTABAR_MARCH "" CACHE STRING "Dodatkowe flagi architektury CPU (domyślnie przenośne)")

if(MINGW)
    target_compile_options(KorektaBar PRIVATE -O2 ${KOREKTABAR_MARCH} -ffunction-sections -fdata-sections -Wall -Wextra -finput-charset=UTF-8)
    target_link_options(KorektaBar PRIVATE -mwindows -municode -static -static-libgcc -static-libstdc++ -Wl,--gc-sections -s)
endif()

target_link_libraries(KorektaBar PRIVATE
    d2d1
    dwrite
    dwmapi
    comctl32
    shell32
    ole32
    uuid
    uxtheme
)
