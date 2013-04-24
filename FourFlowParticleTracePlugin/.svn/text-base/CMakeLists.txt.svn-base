 # Locate ParaView build and then import CMake configuration, 
 # macros etc. from it.
FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

INCLUDE(${ParaView_CMAKE_DIR}/ParaViewBranding.cmake)
INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)
 
# Use the ADD_PARAVIEW_PLUGIN macro to build a plugin
SET(PLUGIN_NAME vtkFourFlowParticleTrace)
ADD_PARAVIEW_PLUGIN(
	${PLUGIN_NAME} "1.0"
	SERVER_MANAGER_XML ParticleTracePlugin.xml
	SERVER_MANAGER_SOURCES vtkFourFlowParticleTrace.cxx
	GUI_RESOURCE_FILES ParticleTracePluginGUI.xml
)

TARGET_LINK_LIBRARIES(${PLUGIN_NAME} vtkPVCommon pqCore pqComponents pqApplicationComponents vtkFiltersFlowPaths
  ${pv_common}
  ${vtkWidgets}
)