 # Locate ParaView build and then import CMake configuration, 
 # macros etc. from it.
FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

INCLUDE(${ParaView_CMAKE_DIR}/ParaViewBranding.cmake)
INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)
 
# Use the ADD_PARAVIEW_PLUGIN macro to build a plugin
ADD_PARAVIEW_PLUGIN(
	vtkFourFlowVolumeTrack "1.0"
	SERVER_MANAGER_XML vtkFourFlowVolumeTrack.xml
	SERVER_MANAGER_SOURCES vtkFourFlowVolumeTrack.cxx
	GUI_RESOURCE_FILES vtkFourFlowVolumeTrackGUI.xml
)

