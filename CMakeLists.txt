cmake_minimum_required(VERSION 3.20)
project(NetAntsReborn VERSION 2.0.0 LANGUAGES CXX)

# ======================================================
# NetAnts Reborn - Clon limpio modernizado
# Stack: C++17, wxWidgets 3.2+, libcurl, cpr, nlohmann/json
# Objetivo: gcc (MinGW-w64 / UCRT64) en Windows
# ======================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Configuración global de dependencias externas ---
find_package(wxWidgets REQUIRED CONFIG)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# CPR - Buscar local o descargar vía FetchContent
find_package(cpr CONFIG QUIET)
if(NOT cpr_FOUND)
    include(FetchContent)
    # Evita que CPR intente compilar sus propios tests invasivos
    set(CPR_BUILD_TESTS OFF CACHE INTERNAL "")
    set(CPR_USE_SYSTEM_CURL ON CACHE INTERNAL "")
    
    FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.10.5
    )
    FetchContent_MakeAvailable(cpr)
endif()

# --- Definición de Fuentes del Proyecto ---
set(SOURCES
    src/main.cpp
    src/App.cpp
    src/MainFrame.cpp
    src/DownloadManager.cpp
    src/DownloadJob.cpp
    src/JobStorage.cpp
    src/AddUrlDialog.cpp
    src/OptionsDialog.cpp
    src/Utils.cpp
)

set(HEADERS
    src/App.h
    src/MainFrame.h
    src/DownloadManager.h
    src/DownloadJob.h
    src/JobStorage.h
    src/AddUrlDialog.h
    src/OptionsDialog.h
    src/Icons.h
    src/Animations.h
    src/Utils.h
)

# Crear el ejecutable (WIN32 oculta la consola por defecto en modo Release)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE src)

# --- Optimizaciones Específicas para el Ejecutable (Evita romper cpr) ---
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Opciones de compilación aplicadas solo a tu aplicación gráfica
    target_compile_options(${PROJECT_NAME} PRIVATE
        -O3
        -flto=auto
        -pipe
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -march=x86-64
        -mtune=generic
        -municode
    )
    
    # Soporte Unicode para Win32 API
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

    target_link_options(${PROJECT_NAME} PRIVATE
        -flto=auto
        -s
        -static-libgcc
        -static-libstdc++
    )

    # Configuración dinámica para el modo de depuración (Debug)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g3 -fno-omit-frame-pointer)
        # Fuerza la aparición de la consola negra en Windows solo para ver logs de Debug
        target_link_options(${PROJECT_NAME} PRIVATE -Wl,--subsystem,console)
    else()
        # Oculta la consola por completo en Release
        target_link_options(${PROJECT_NAME} PRIVATE -Wl,--subsystem,windows)
    endif()
endif()

# --- Vinculación de Librerías (Uso estricto de targets de CMake Moderno) ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${wxWidgets_LIBRARIES}          # Usa las librerías resueltas por el paquete
    CURL::libcurl
    cpr::cpr
    nlohmann_json::nlohmann_json
    # Librerías esenciales del SDK de Windows requeridas por los sockets de curl y wx
    ws2_32 wsock32 wininet winmm comctl32 ole32 oleaut32 uuid
)
# Recursos Windows (Iconos integrados en el ejecutable .exe)
if(WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc")
        target_sources(${PROJECT_NAME} PRIVATE resources/app.rc)
    endif()
endif()

# --- Sección de Instalación ---
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    install(DIRECTORY resources/ DESTINATION share/${PROJECT_NAME}/resources)
endif()

# Mensajes informativos de depuración de CMake
message(STATUS "=== NetAntsReborn Config ===")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")