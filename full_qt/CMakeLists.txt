cmake_minimum_required(VERSION 3.19)
project(TSP_Visualize_Application LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(TSP_Visualize_Application
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    GraphCanvas.cpp
    GraphCanvas.h
    tsp_steps.cpp
    tsp_steps.h
    tsp_algorithms.cpp
    tsp_algorithms.h
    pseudopopup.cpp
    pseudopopup.h
)

target_link_libraries(TSP_Visualize_Application
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS TSP_Visualize_Application
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET TSP_Visualize_Application
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
