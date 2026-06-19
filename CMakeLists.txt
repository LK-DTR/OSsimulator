cmake_minimum_required(VERSION 3.16)
project(SimuladorQt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(simulador_so
    main.cpp
    simulador.hpp
    core/simulacao.hpp
    core/scheduler.hpp
    core/memory.hpp
    core/tipos.hpp
)

qt_add_qml_module(simulador_so
    URI SimuladorQt
    QML_FILES
        Main.qml
)

set_target_properties(simulador_so PROPERTIES WIN32_EXECUTABLE OFF)
target_link_libraries(simulador_so
    PRIVATE Qt6::Quick
    PRIVATE Qt6::QuickControls2

)

install(TARGETS simulador_so
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
