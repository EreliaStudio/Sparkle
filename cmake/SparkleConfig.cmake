
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was SparkleConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

find_package(GLEW REQUIRED)

set(SPARKLE_INCLUDE_DIRS "C:/Program Files (x86)/Sparkle/include" ${GLEW_INCLUDE_DIRS})
set(SPARKLE_LIBRARY_DIRS "C:/Program Files (x86)/Sparkle/lib/x32/Debug" ${GLEW_LIBRARY_DIRS})

set(SPARKLE_LIBRARIES Sparkle OpenGL32.lib GLEW::GLEW)

include("C:/Program Files (x86)/Sparkle/lib/cmake/Sparkle/SparkleTargets.cmake")
