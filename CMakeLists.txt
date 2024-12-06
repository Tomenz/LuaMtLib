cmake_minimum_required(VERSION 3.0.0)
project(luamtlib VERSION 1.0.0)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

# specify the C++ standard
set(SUPPORTED_CXX_STANDARDS 17)
if(NOT DEFINED CMAKE_CXX_STANDARD)
    message(STATUS "Setting C++ version to '17' as none was specified.")
    set(CMAKE_CXX_STANDARD 17)
else()
    message(STATUS "CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} set by parent project.")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# load the "lua" as submodule
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        if(NOT EXISTS "${PROJECT_SOURCE_DIR}/lua")
            execute_process(COMMAND ${GIT_EXECUTABLE} submodule add -f https://github.com/lua/lua.git
                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                            RESULT_VARIABLE GIT_SUBMOD_RESULT)
        else()
            execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -f
                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                            RESULT_VARIABLE GIT_SUBMOD_RESULT)
        endif()
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/lua/lua.h")
    message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

include_directories(${CMAKE_SOURCE_DIR}/lua)

set(LUA_LIB_SRCS
    "lua/lapi.c"
    "lua/lcode.c"
    "lua/lctype.c"
    "lua/ldebug.c"
    "lua/ldo.c"
    "lua/ldump.c"
    "lua/lfunc.c"
    "lua/lgc.c"
    "lua/llex.c"
    "lua/lmem.c"
    "lua/lobject.c"
    "lua/lopcodes.c"
    "lua/lparser.c"
    "lua/lstate.c"
    "lua/lstring.c"
    "lua/ltable.c"
    "lua/ltm.c"
    "lua/lundump.c"
    "lua/lvm.c"
    "lua/lzio.c"
    "lua/lauxlib.c"
    "lua/lbaselib.c"
    "lua/lcorolib.c"
    "lua/ldblib.c"
    "lua/liolib.c"
    "lua/lmathlib.c"
    "lua/loadlib.c"
    "lua/loslib.c"
    "lua/lstrlib.c"
    "lua/ltablib.c"
    "lua/lutf8lib.c"
    "lua/linit.c"
)

set_source_files_properties(${LUA_LIB_SRCS} PROPERTIES LANGUAGE C)
ADD_DEFINITIONS(-DLUA_USER_H="../lualock.h")
add_library(${PROJECT_NAME} STATIC ${LUA_LIB_SRCS}  ${CMAKE_CURRENT_LIST_DIR}/LuaScript.cpp)

# Test executable
add_executable(LuaTest LuaTest.cpp)
target_link_libraries(LuaTest ${PROJECT_NAME})
if (NOT MSVC)
    target_link_libraries(LuaTest pthread)
endif()
