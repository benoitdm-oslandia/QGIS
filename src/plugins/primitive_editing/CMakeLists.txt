
########################################################
# Files

set (primitive_editing_plugin_SRCS
  src/primitive_editing_plugin.cpp
  src/qgs3dcreateprimitivedialog.cpp
  src/qgs3dcreateprimitiveboxdialog.cpp
  src/qgs3dcreateprimitiveconedialog.cpp
  src/qgs3dcreateprimitivecylinderdialog.cpp
  src/qgs3dcreateprimitivespheredialog.cpp
  src/qgs3dcreateprimitivetorusdialog.cpp
  src/qgs3dmaptoolcreateprimitive.cpp
  src/qgs3dprimitiveeditingtoolbar.cpp
)

set (primitive_editing_plugin_UIS
  src/createprimitivedialog.ui
)

set (primitive_editing_plugin_RCCS  resources/images.qrc)

########################################################
# Build

qt_wrap_ui(primitive_editing_plugin_UIS_H ${primitive_editing_plugin_UIS})

add_library (plugin_primitiveediting MODULE
  ${primitive_editing_plugin_SRCS}
  ${primitive_editing_plugin_RCCS}
  ${primitive_editing_plugin_UIS_H}
)

target_link_libraries(plugin_primitiveediting
  qgis_core
  qgis_gui
  qgis_3d
  qgis_app
)

include_directories(
  ${CMAKE_SOURCE_DIR}/src/plugins

  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

########################################################
# Install

install(TARGETS plugin_primitiveediting
	RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
	LIBRARY DESTINATION ${QGIS_PLUGIN_DIR}
)
