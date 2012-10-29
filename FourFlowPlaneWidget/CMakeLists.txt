PROJECT("FourFlowPlaneWidget")

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

#--------------------------------------------------
# Set project include directories 
# pv-common is assumed to be checkout out too
#--------------------------------------------------
INCLUDE_DIRECTORIES(
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_BINARY_DIR}"
)

#--------------------------------------------------
# Setup GUI custom Qt panel sources and wrapping
#--------------------------------------------------
#--------------------------------------------------
# make sure all *.ui dialogs/etc are wrapped
#--------------------------------------------------
#QT4_WRAP_UI(UI_BUILT_SOURCES
#  pqFourFlowPlaneWidget.ui
#)

#--------------------------------------------------
# make sure all *.h files for ui are wrapped by moc
#--------------------------------------------------
QT4_WRAP_CPP(UI_MOC
    pqFourFlowPlaneWidget.h
)
  
#--------------------------------------------------
# invoke macro that adds our grid source 3D widget
#--------------------------------------------------
ADD_3DWIDGET(
  FOURFLOW_IFACE_W
  FOURFLOW_IFACE_SRCS_W
  CLASS_NAME 
    pqFourFlowPlaneWidget
  WIDGET_TYPE 
    # string found in <hints> section of xml
    "FourFlowPlane"
)

#--------------------------------------------------
# Define Wrapping hints
#--------------------------------------------------
SET(VTK_WRAP_HINTS "${CMAKE_CURRENT_SOURCE_DIR}/hints" )
 

#--------------------------------------------------
# Define Plugin 
#--------------------------------------------------
SET(PLUGIN_NAME vtkFourFlowPlaneWidgetPlugin)
ADD_PARAVIEW_PLUGIN(
  ${PLUGIN_NAME}
  "1.0" 

  SERVER_MANAGER_XML 
    ${CMAKE_CURRENT_SOURCE_DIR}/FourFlowPlaneWidget.xml
    ${TIPSY_XML}
  SERVER_MANAGER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkFourFlowPlaneRepresentation.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkFourFlowPlaneWidget.cxx
    ${TIPSY_CXX}
  GUI_INTERFACES 
    ${FOURFLOW_IFACE_W}
  GUI_SOURCES
    ${UI_BUILT_SOURCES}
    ${UI_MOC}
    ${CMAKE_CURRENT_SOURCE_DIR}/pqFourFlowPlaneWidget.cxx
    ${FOURFLOW_IFACE_SRCS_W}
  GUI_RESOURCE_FILES
    #${CMAKE_CURRENT_SOURCE_DIR}/pqFourFlowPlaneWidget.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/FourFlowPlaneWidget.xml
)

TARGET_LINK_LIBRARIES(${PLUGIN_NAME}
  ${pv_common}
  ${vtkWidgets}
  ${TIPSY_LIB}
)

#--------------------------------------------------------
# Create the configuration header.
#--------------------------------------------------------
#CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/pv_meshless_configure.h.in
#               ${PROJECT_BINARY_DIR}/vtkCSCSMeshlessConfigure.h
#               @ONLY IMMEDIATE)


#--------------------------------------------------
# Install
#--------------------------------------------------
SET(INSTALL_PATH 
  "${CMAKE_INSTALL_PREFIX}/lib/paraview-${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
)

#INSTALL(
#  FILES ${TOOL_CFG_FILES}
#  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
#)

INSTALL(
  TARGETS
    ${PLUGIN_NAME}
  DESTINATION 
    ${INSTALL_PATH}
)
