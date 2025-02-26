project(vlfeat)

include(OptimizeTheiaCompilerFlags)
optimizetheiacompilerflags()

include_directories(./vl)
set(vl_sources
  vl/fisher.c
  vl/generic.c
  vl/gmm.c
  vl/host.c
  vl/imopv.c
  vl/imopv_sse2.c
  vl/kmeans.c
  vl/kdtree.c
  vl/mathop.c
  vl/mathop_sse2.c
  vl/random.c
  vl/sift.c
  vl/vlad.c)
set_source_files_properties(${vl_sources} PROPERTIES LANGUAGE C)

if (MSVC)
  add_definitions(-DVL_BUILD_DLL)
  add_definitions(-DVL_DISABLE_SSE2)
endif (MSVC)

if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64" )
    add_definitions(-DVL_DISABLE_SSE2)
endif()

add_library(vlfeat SHARED ${vl_sources})
install(TARGETS vlfeat
  EXPORT  TheiaExport
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib${LIB_SUFFIX}
  ARCHIVE DESTINATION lib${LIB_SUFFIX})
