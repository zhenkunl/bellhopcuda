# bellhopcxx / bellhopcuda - C++/CUDA port of BELLHOP underwater acoustics simulator
# Copyright (C) 2021-2022 The Regents of the University of California
# c/o Jules Jaffe team at SIO / UCSD, jjaffe@ucsd.edu
# Based on BELLHOP, which is Copyright (C) 1983-2020 Michael B. Porter
# 
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
# 
# This program is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE. See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

set(BHC_RUN_DATABASE "TL:C;EIGENRAYS:E;ARRIVALS:A")
set(BHC_INFL_DATABASE "CERVENY_RAYCEN:R;CERVENY_CART:C;HAT_RAYCEN:g;HAT_CART:G;GAUSS_RAYCEN:b;GAUSS_CART:B;SGB:S")
set(BHC_SSP_DATABASE "N2LINEAR:N;CLINEAR:C;CUBIC:S;PCHIP:P;QUAD:Q;HEXAHEDRAL:H;ANALYTIC:A")

function(add_gen_template_defs_inner target_name type)
    foreach(pair IN LISTS BHC_${type}_DATABASE)
        if(pair MATCHES "(.+):(.+)")
            set(NAME "${CMAKE_MATCH_1}")
        else()
            message(FATAL_ERROR "Internal error with template generation: ${type}")
        endif()
        if(${BHC_${type}_ENABLE_${NAME}})
            target_compile_definitions(${target_name} PRIVATE BHC_${type}_ENABLE_${NAME}=1)
        endif()
    endforeach()
endfunction()

function(add_gen_template_defs target_name)
    add_gen_template_defs_inner(${target_name} RUN)
    add_gen_template_defs_inner(${target_name} INFL)
    add_gen_template_defs_inner(${target_name} SSP)
endfunction()

function(is_config_valid out_var_name)
    set(res 1)
    if(BHCGENSSP MATCHES "Q" AND BHCGENO3D STREQUAL "true")
        set(res 0)
    elseif(BHCGENSSP MATCHES "H" AND BHCGENO3D STREQUAL "false")
        set(res 0)
    elseif(BHCGENINFL MATCHES "[RCS]" AND BHCGENR3D STREQUAL "true")
        set(res 0)
    elseif(BHCGENINFL MATCHES "[RC]" AND BHCGENRUN MATCHES "[EAa]")
        set(res 0)
    endif()
    if(BHC_LIMIT_FEATURES)
        if(BHCGENSSP MATCHES "P" AND BHCGENO3D STREQUAL "true")
            set(res 0)
        elseif(BHCGENINFL MATCHES "C" AND BHCGENO3D STREQUAL "true")
            set(res 0)
        elseif(BHCGENINFL MATCHES "b" AND BHCGENO3D STREQUAL "false")
            set(res 0)
        endif()
    endif()
    set(${out_var_name} ${res} PARENT_SCOPE)
endfunction()

function(gen_templates_inner EXTENSION SOURCE_LIST_INNER_VAR)
    set(SOURCE_LIST_INNER "")
    foreach(run_pair IN LISTS BHC_RUN_DATABASE)
        if(run_pair MATCHES "(.+):(.+)")
            set(RUN_NAME "${CMAKE_MATCH_1}")
            set(BHCGENRUN "'${CMAKE_MATCH_2}'")
        else()
            message(FATAL_ERROR "Internal error with template generation: run")
        endif()
        if(NOT ${BHC_RUN_ENABLE_${RUN_NAME}})
            continue()
        endif()
        foreach(infl_pair IN LISTS BHC_INFL_DATABASE)
            if(infl_pair MATCHES "(.+):(.+)")
                set(INFL_NAME "${CMAKE_MATCH_1}")
                set(BHCGENINFL "'${CMAKE_MATCH_2}'")
            else()
                message(FATAL_ERROR "Internal error with template generation: infl")
            endif()
            if(NOT ${BHC_INFL_ENABLE_${INFL_NAME}})
                continue()
            endif()
            foreach(ssp_pair IN LISTS BHC_SSP_DATABASE)
                if(ssp_pair MATCHES "(.+):(.+)")
                    set(SSP_NAME "${CMAKE_MATCH_1}")
                    set(BHCGENSSP "'${CMAKE_MATCH_2}'")
                else()
                    message(FATAL_ERROR "Internal error with template generation: ssp")
                endif()
                if(NOT ${BHC_SSP_ENABLE_${SSP_NAME}})
                    continue()
                endif()
                is_config_valid(isvalid)
                if(NOT isvalid)
                    message(DEBUG "Not building ${DIM_NAME}_${RUN_NAME}_${INFL_NAME}_${SSP_NAME}")
                    continue()
                endif()
                set(OUT_FILENAME "field_${DIM_NAME}_${RUN_NAME}_${INFL_NAME}_${SSP_NAME}.${EXTENSION}")
                set(OUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/gen_templates/${OUT_FILENAME}")
                configure_file(
                    "${CMAKE_SOURCE_DIR}/src/runfieldmodesimpl.${EXTENSION}.in"
                    "${OUT_FILE}"
                )
                list(APPEND SOURCE_LIST_INNER "${OUT_FILE}")
            endforeach()
        endforeach()
    endforeach()
    if(NOT SOURCE_LIST_INNER)
        message(WARNING "No field settings selected to build; ray mode only")
    endif()
    set(${SOURCE_LIST_INNER_VAR} "${SOURCE_LIST_INNER}" PARENT_SCOPE)
endfunction()

function(gen_templates EXTENSION SOURCE_LIST_VAR)
    set(SOURCE_LIST "")
    if(BHC_DIM_ENABLE_2D)
        set(DIM_NAME "2D")
        set(BHCGENO3D "false")
        set(BHCGENR3D "false")
        gen_templates_inner(${EXTENSION} SOURCE_LIST_TEMP)
        list(APPEND SOURCE_LIST "${SOURCE_LIST_TEMP}")
    endif()
    if(BHC_DIM_ENABLE_3D)
        set(DIM_NAME "3D")
        set(BHCGENO3D "true")
        set(BHCGENR3D "true")
        gen_templates_inner(${EXTENSION} SOURCE_LIST_TEMP)
        list(APPEND SOURCE_LIST "${SOURCE_LIST_TEMP}")
    endif()
    if(BHC_DIM_ENABLE_NX2D)
        set(DIM_NAME "NX2D")
        set(BHCGENO3D "true")
        set(BHCGENR3D "false")
        gen_templates_inner(${EXTENSION} SOURCE_LIST_TEMP)
        list(APPEND SOURCE_LIST "${SOURCE_LIST_TEMP}")
    endif()
    set(${SOURCE_LIST_VAR} "${SOURCE_LIST}" PARENT_SCOPE)
endfunction()
