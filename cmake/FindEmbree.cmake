find_path(EMBREE_INCLUDE_PATH embree3/rtcore.h
  ${CMAKE_SOURCE_DIR}/embree/include
  /usr/include
  /usr/local/include
  /opt/local/include)

if (APPLE)
find_library(EMBREE_LIBRARY NAMES embree3 PATHS
  ${CMAKE_SOURCE_DIR}/embree/lib-macos
  /usr/lib
  /usr/local/lib
  /opt/local/lib)
elseif (WIN32)
find_library(EMBREE_LIBRARY NAMES embree3 PATHS
  ${CMAKE_SOURCE_DIR}/embree/lib-win32)
else ()
find_library(EMBREE_LIBRARY NAMES embree3 PATHS
  ${CMAKE_SOURCE_DIR}/embree/lib-linux
  /usr/lib
  /usr/local/lib
  /opt/local/lib)
endif ()

if (EMBREE_INCLUDE_PATH AND EMBREE_LIBRARY)
  set(EMBREE_FOUND TRUE)
endif ()