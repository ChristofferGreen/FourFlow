cmake_minimum_required(VERSION 2.6)

project(FourFlow)

FIND_PACKAGE(ParaView REQUIRED)
FIND_PACKAGE(Qt4)
INCLUDE(${PARAVIEW_USE_FILE})

INCLUDE(${ParaView_CMAKE_DIR}/ParaViewBranding.cmake)
INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)


#------------------------------------------------------------------------------
# ParaView applications provides a mechanism to add gui xmls from modules.
# This is done by defining variables named ${vtk-module}_PARAVIEW_GUI_XMLS in
# the module.cmake file for the modules pointing to the paths for the
# GUI-xmls.  We process those here.
set (application_gui_xmls
       "${CMAKE_CURRENT_SOURCE_DIR}/xml/ParaViewSources.xml"
       "${CMAKE_CURRENT_SOURCE_DIR}/xml/ParaViewFilters.xml")
foreach (module IN LISTS VTK_MODULES_ENABLED)
  get_property(gui_xml GLOBAL PROPERTY ${module}_PARAVIEW_GUI_XMLS)
  if (gui_xml)
    foreach(xml IN LISTS gui_xml)
      list(APPEND application_gui_xmls ${xml})
    endforeach()
  endif()
endforeach()

#------------------------------------------------------------------------------
# Add extra library containing custom code for the client.
QT4_WRAP_CPP(MOC_BUILT_SOURCES
  src/FourFlowMainWindow.h
  src/pqInternals.h
  src/ImportDirectoryWindow.h
  src/ConnectionWindow.h)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ui/MainWindow.ui
  ui/ImportDirectoryWindow.ui
  ui/ConnectionWindow.ui
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

QT4_ADD_RESOURCES( APP_RESOURCES_OUT 
  ./ui/resources.qrc 
)

SET(ParaView_SOURCE_FILES
  src/FourFlowMainWindow.cxx
  src/FourFlowMainWindow.h
  src/pqInternals.cxx
  src/pqInternals.h
  src/ImportDirectoryWindow.h
  src/ImportDirectoryWindow.cxx
  src/ConnectionWindow.h
  src/ConnectionWindow.cxx
  src/staticFunctions.h
  src/staticFunctions.cxx
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES})

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR})

build_paraview_client(FourFlow
  TITLE "FourFlow"
  ORGANIZATION  "LundsUniversitet"
  VERSION_MAJOR 1 
  VERSION_MINOR 2
  VERSION_PATCH 13
  SPLASH_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/ui/logo.png"
  PVMAIN_WINDOW FourFlowMainWindow
  PVMAIN_WINDOW_INCLUDE src/FourFlowMainWindow.h
  APPLICATION_ICON  "ui/icons/vfront3.ico"
  GUI_CONFIGURATION_XMLS
    ${application_gui_xmls}
  SOURCES ${ParaView_SOURCE_FILES} ${APP_RESOURCES_OUT}
)
#vtkCommon
TARGET_LINK_LIBRARIES(FourFlow ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY}  vtkPVCommon pqCore pqComponents)
