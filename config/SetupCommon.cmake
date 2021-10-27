
set(CMAKE_CXX_STANDARD 14) # C++14
set(CMAKE_CXX_STANDARD_REQUIRED ON) # ...is required
set(CMAKE_CXX_EXTENSTIONS OFF) # ...without compiler extensions like gnu++11
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Necessary to build shared libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

function(bellhop_setup_target target_name)
    if(USE_FLOAT)
        target_compile_definitions(${target_name} PUBLIC USE_FLOATS=1)
    endif()
    target_include_directories(${target_name} PUBLIC "${CMAKE_SOURCE_DIR}/glm")
endfunction()

function(prepend OUT_VAR PREFIX) #Arguments 3, 4, etc. are items to prepend to
    set(TEMP "")
    foreach(ITEM ${ARGN})
        set(TEMP "${TEMP} ${PREFIX}${ITEM}")
    endforeach()
    set(${OUT_VAR} "${TEMP}" PARENT_SCOPE)
endfunction()

set(COMMON_SOURCE
    angles.hpp
    arrivals.hpp
    attenuation.hpp
    beampattern.hpp
    boundary.hpp
    common.hpp
    curves.hpp
    influence.hpp
    ldio.hpp
    readenv.cpp
    refcoef.hpp
    sourcereceiver.hpp
    ssp.cpp
    ssp.hpp
    step.hpp
    subtab.hpp
    trace.hpp
    writeray.hpp
)

prepend(COMMON_SOURCE "${CMAKE_SOURCE_DIR}/src/" ${COMMON_SOURCE})
