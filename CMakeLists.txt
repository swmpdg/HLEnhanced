###################################################
#                                                 #
#                                                 #
#	Half-Life Enhanced CMake build file           #
#                                                 #
#                                                 #
###################################################
cmake_minimum_required( VERSION 3.6 )

# function to collect all the sources from sub-directories
# into a single list
function(add_sources)
  get_property(is_defined GLOBAL PROPERTY SRCS_LIST DEFINED)
  if(NOT is_defined)
    define_property(GLOBAL PROPERTY SRCS_LIST
      BRIEF_DOCS "List of source files"
      FULL_DOCS "List of source files to be compiled in one library")
  endif()
  # make absolute paths
  set(SRCS)
  foreach(s IN LISTS ARGN)
    if(NOT IS_ABSOLUTE "${s}")
      get_filename_component(s "${s}" ABSOLUTE)
    endif()
    list(APPEND SRCS "${s}")
  endforeach()
  # append to global list
  set_property(GLOBAL APPEND PROPERTY SRCS_LIST "${SRCS}")
endfunction(add_sources)

# preprocess sources
function( preprocess_sources )
	set(PREP_SRCS PARENT_SCOPE)
	get_property(SRCS GLOBAL PROPERTY SRCS_LIST)
	foreach(s IN LISTS SRCS)
	  file(RELATIVE_PATH rs "${CMAKE_CURRENT_SOURCE_DIR}" "${s}")
	  string(REGEX REPLACE "r$" "" o "${CMAKE_CURRENT_BINARY_DIR}/${rs}")
	  add_custom_command(
		OUTPUT "${o}"
		COMMAND ${CMAKE_COMMAND} -E copy "${s}" "${o}"
		DEPENDS "${s}"
		COMMENT "Creating ${o}"
		VERBATIM
		)
	  list(APPEND PREP_SRCS "${s}")
	endforeach()
	set(PREP_SRCS ${PREP_SRCS} PARENT_SCOPE)
endfunction( preprocess_sources )

function( create_source_groups _src_root_path )
	get_property(SRCS GLOBAL PROPERTY SRCS_LIST)
	foreach(_source IN ITEMS ${SRCS})
		get_filename_component(_source_path "${_source}" PATH)
		file(RELATIVE_PATH _source_path_rel "${_src_root_path}" "${_source_path}")
		string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
		source_group("${_group_path}" FILES "${_source}")
	endforeach()
endfunction( create_source_groups )

#Function to clear the sources list.
function( clear_sources )
	set_property( GLOBAL PROPERTY SRCS_LIST "" )
	set( PREP_SRCS PARENT_SCOPE )
endfunction( clear_sources )

# Function to add include files.
function( add_includes )
  get_property(is_defined GLOBAL PROPERTY INCLUDES_LIST DEFINED)
  if(NOT is_defined)
    define_property(GLOBAL PROPERTY INCLUDES_LIST
      BRIEF_DOCS "List of include files"
      FULL_DOCS "List of include files to be compiled in one library")
  endif()
  # make absolute paths
  set(INCLUDES)
  foreach(s IN LISTS ARGN)
    if(NOT IS_ABSOLUTE "${s}")
      get_filename_component(s "${s}" ABSOLUTE)
    endif()
    list(APPEND INCLUDES "${s}")
  endforeach()
  # append to global list
  set_property(GLOBAL APPEND PROPERTY INCLUDES_LIST "${INCLUDES}")
endfunction( add_includes )

# Function to install includes.
function( install_includes _include_root_path )
	get_property(INCLUDES GLOBAL PROPERTY INCLUDES_LIST)
	
	foreach(_include IN ITEMS ${INCLUDES})
		get_filename_component(_include_path "${_include}" PATH)
		file(RELATIVE_PATH _include_path_rel "${_include_root_path}" "${_include_path}")
		string(REPLACE "/" "\\" _group_path "${_include_path_rel}")
		install( FILES "${_include}" DESTINATION "include/${_group_path}" )
	endforeach()
	
	set_property( GLOBAL PROPERTY INCLUDES_LIST "" )
endfunction( install_includes )

macro(configure_msvc_runtime)
  if(MSVC)
    # Default to statically-linked runtime.
    if("${MSVC_RUNTIME}" STREQUAL "")
      set(MSVC_RUNTIME "static")
    endif()
    # Set compiler options.
    set(variables
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    if(${MSVC_RUNTIME} STREQUAL "static")
      message(STATUS
        "MSVC -> forcing use of statically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endif()
      endforeach()
    else()
      message(STATUS
        "MSVC -> forcing use of dynamically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MT")
          string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
        endif()
      endforeach()
    endif()
  endif()
endmacro()

project( HLEnhanced )

#Needs static CRT to avoid alloc issues.
set( MSVC_RUNTIME "static" )

configure_msvc_runtime()

set(variables
  CMAKE_C_FLAGS_DEBUG
  CMAKE_C_FLAGS_MINSIZEREL
  CMAKE_C_FLAGS_RELEASE
  CMAKE_C_FLAGS_RELWITHDEBINFO
  CMAKE_CXX_FLAGS_DEBUG
  CMAKE_CXX_FLAGS_MINSIZEREL
  CMAKE_CXX_FLAGS_RELEASE
  CMAKE_CXX_FLAGS_RELWITHDEBINFO
)
message(STATUS "Initial build flags:")
foreach(variable ${variables})
  message(STATUS "  '${variable}': ${${variable}}")
endforeach()
message(STATUS "")

#Get the steam/steamapps/common directory
if( WIN32 )
	set( STEAMCOMMON "$ENV{SteamCommon}" )
else()
	set( STEAMCOMMON "" )
	#TODO
endif()

#Base dir for game outputs
set( GAME_BASE_PATH "${STEAMCOMMON}/Half-Life/hlenhanced" )

#Shared preprocessor definitions
set( SHARED_DEFS 
	_CRT_SECURE_NO_WARNINGS
)

#Shared directories

add_subdirectory( common )
add_subdirectory( game/shared )
add_subdirectory( pm_shared )
add_subdirectory( public )

#Save the shares sources into SHARED_SRCS
get_property(SHARED_SRCS GLOBAL PROPERTY SRCS_LIST)

clear_sources()

#
#Client library
#

#Add in the shared sources
add_sources( "${SHARED_SRCS}" )

#Add in the client directories
add_subdirectory( game/client )
add_subdirectory( vgui_utils )
add_subdirectory( shared/engine/client )

preprocess_sources()

#Cache the old include directories list
get_property( INCLUDE_DIRS DIRECTORY PROPERTY INCLUDE_DIRECTORIES )

include_directories(
	common
	public
	public/math
	engine
	shared/engine/client
	game_shared
	game/shared
	game/shared/entities/player
	utils/vgui/include
	vgui_utils
	pm_shared
	game/client
	game/client/ui/hud
	game/client/ui/vgui
	game/server
	external
	source_sdk/public/tier1
	source_sdk/public
	source_sdk/common
)

add_library( client SHARED ${PREP_SRCS} )

target_compile_definitions( client PRIVATE
	"${SHARED_DEFS}"
	CLIENT_DLL
	CLIENT_WEAPONS
	HL_DLL
)

#Find client dependencies
find_library( VGUI1 vgui PATHS utils/vgui/lib/win32_vc6/ )
find_library( SDL2 sdl2 PATHS lib/public/ )
find_library( VGUI2 vgui_controls PATHS source_sdk/lib/public/ )
find_library( TIER0 tier0 PATHS lib/public/ )
find_library( TIER1 tier1 PATHS source_sdk/lib/public/ )
find_library( VSTDLIB vstdlib PATHS lib/public/ )

#Set the list of platform specific libraries
if( WIN32 )
	set( CLIENT_PLAT_LIBS "wsock32" )
else()
	set( CLIENT_PLAT_LIBS "" )
endif()

#Link with client dependencies
target_link_libraries( client 
	"${VGUI1}"
	"${CLIENT_PLAT_LIBS}"
	"${SDL2}"
	"${VGUI2}"
	"${TIER0}"
	"${TIER1}"
	"${VSTDLIB}"
)

#CMake places libraries in /Debug or /Release on Windows, so explicitly set the paths for both.
set_target_properties( client PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${GAME_BASE_PATH}/cl_dlls"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${GAME_BASE_PATH}/cl_dlls"
)

#Set Windows subsystem
if( WIN32 )
	if( MSVC )
		set_target_properties( client PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS" )
	endif()
endif()

#Create filters
create_source_groups( "${CMAKE_SOURCE_DIR}" )

#Set the startup project to the client
#TODO: does this even work?
set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT client )

clear_sources()

#
#End client library
#

#Reset include dirs
set_property( DIRECTORY PROPERTY INCLUDE_DIRECTORIES "${INCLUDE_DIRS}" )

#
#Server library
#

#Add in the shared sources
add_sources( "${SHARED_SRCS}" )

#Add in the server directories
add_subdirectory( game/server )
add_subdirectory( game/shared/Angelscript )

preprocess_sources()

include_directories(
	common
	public
	public/math
	engine
	game_shared
	game/shared
	game/shared/entities/player
	pm_shared
	game/server
	external/Angelscript/include
	external
)

add_library( hl SHARED ${PREP_SRCS} )

#So many of these... Do we need all of them?
target_compile_definitions( hl PRIVATE
	"${SHARED_DEFS}"
	SERVER_DLL
	QUIVER
	VOXEL
	QUAKE2
	VALVE_DLL
	CLIENT_WEAPONS
)

#Find server dependencies
find_library( ANGELSCRIPTLIB Angelscript PATHS external/Angelscript/lib/ )
find_library( ANGELSCRIPTUTILS AngelscriptUtils PATHS external/Angelscript/lib/ )

#Link with server dependencies
target_link_libraries( hl 
	"${ANGELSCRIPTLIB}"
	"${ANGELSCRIPTUTILS}"
)

#CMake places libraries in /Debug or /Release on Windows, so explicitly set the paths for both.
set_target_properties( hl PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${GAME_BASE_PATH}/dlls"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${GAME_BASE_PATH}/dlls"
)

if( WIN32 )
	if( MSVC )
		#Set Windows subsystem
		#Add the module definition file. Needed for GiveFnptrsToDll function export. Requires the project dir to be near the file.
		set_target_properties( hl PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /DEF:../hl.def" )
	endif()
endif()

#Create filters
create_source_groups( "${CMAKE_SOURCE_DIR}" )

clear_sources()

#
#End server library
#

#TODO: add utility exes here

#project( HLEnhanced_Utils )