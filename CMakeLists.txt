cmake_minimum_required(VERSION 3.16)

project(untitled VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- STRICT MODE ---
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# Ищем все пакеты сразу
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Core Gui Qml Test Widgets QuickControls2)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

qt_standard_project_setup(REQUIRES 6.5)

# --- СПИСОК ИСХОДНИКОВ ЛОГИКИ ---
# Выносим в переменную, чтобы использовать и в App, и в Tests
set(LOGIC_SOURCES
    src/config.h src/config.cpp
    src/blockanalyzerthread.h src/blockanalyzerthread.cpp
    src/filereaderthread.h src/filereaderthread.cpp
    src/topwordsmodel.h src/topwordsmodel.cpp
    src/logger.h src/logger.cpp
    src/idataprovider.h src/idataprovider.cpp
    src/wordpulseviewmodel.h src/wordpulseviewmodel.cpp
)

# === 1. ОСНОВНОЕ ПРИЛОЖЕНИЕ ===
qt_add_executable(appuntitled
    src/main.cpp
)

qt_add_qml_module(appuntitled
    URI untitled
    VERSION 1.0
    QML_FILES resources/Main.qml
    SOURCES ${LOGIC_SOURCES}
)

target_link_libraries(appuntitled PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::Widgets Qt6::QuickControls2
)

target_include_directories(appuntitled PRIVATE src)

# === 2. ТЕСТЫ ===
enable_testing()

qt_add_executable(analyzer_test
    tests/tst_analyzer.cpp
    tests/mockdataprovider.h
    tests/mockdataprovider.cpp
    ${LOGIC_SOURCES}
)

target_link_libraries(analyzer_test PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Widgets
    Qt6::QuickControls2
)

target_include_directories(analyzer_test PRIVATE src tests)

add_test(NAME AnalyzerTest COMMAND analyzer_test)

# === 3. УСТАНОВКА ===
include(GNUInstallDirs)
install(TARGETS appuntitled
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
