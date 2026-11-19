cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(VTK_Slicer)

# Find the VTK package (Version 9+)
find_package(VTK COMPONENTS
  RenderingCore
    RenderingOpenGL2
    InteractionStyle
    InteractionWidgets
    RenderingAnnotation #
    CommonCore
    FiltersCore
    IOGeometry
    IOImage
    RenderingCore
    RenderingOpenGL2
    InteractionStyle
    REQUIRED
)

# 1. Add ALL necessary source files to the executable target
add_executable(vtk_slicer
    main.cpp
    serial_input/serialib.cpp
)

# 2. Tell the compiler where to find serialib.h
target_include_directories(vtk_slicer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/serial_input
)

# Link the required VTK modules
target_link_libraries(vtk_slicer PRIVATE ${VTK_LIBRARIES})

# VTK 9 requires this auto-initialization macro for the rendering backend
vtk_module_autoinit(
  TARGETS vtk_slicer
  MODULES ${VTK_LIBRARIES}
)
