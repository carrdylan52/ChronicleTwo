# Object rules, from what scripts/build/layout.py says about main.yaml.

# Ask layout.py for one of its listings, one row per list element. Each ask is
# a python process that reads the whole split configuration, so what a flag
# produced is kept for the rest of the configure run.
function(layout_rows flag out_var)
    string(MAKE_C_IDENTIFIER "layout_rows_${flag}" key)
    get_property(known GLOBAL PROPERTY ${key} SET)
    if(known)
        get_property(rows GLOBAL PROPERTY ${key})
        set(${out_var} "${rows}" PARENT_SCOPE)
        return()
    endif()

    execute_process(
        COMMAND ${PYTHON} ${SCRIPTS_DIR}/build/layout.py ${flag}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE rows
        RESULT_VARIABLE status
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "scripts/build/layout.py ${flag} failed (${status})")
    endif()
    # Rows are tab-separated, and a list element cannot hold a `;`.
    string(REPLACE ";" "\;" rows "${rows}")
    string(REPLACE "\n" ";" rows "${rows}")
    set_property(GLOBAL PROPERTY ${key} "${rows}")
    set(${out_var} "${rows}" PARENT_SCOPE)
endfunction()

# Assemble one file splat wrote: a library unit's whole-unit file, or one
# section of a data-only unit. `fixup_args` are the objcopy arguments
# fixup_sections.sh applies (each section's retail alignment).
function(add_asm_object obj src fixup_args)
    kz_asm_object(${obj} ${src}
        POST COMMAND sh ${SCRIPTS_DIR}/build/fixup_sections.sh ${obj} ${fixup_args}
        DEPENDS ${INCLUDE_DIR}/macro.inc
                ${SCRIPTS_DIR}/build/fixup_sections.sh
                ${KZ_SPLIT_STAMP})
endfunction()

# The compiler's language flag for a game source.
function(cpp_lang src out_var)
    if(src MATCHES "\\.c$")
        set(${out_var} c PARENT_SCOPE)
    else()
        set(${out_var} c++ PARENT_SCOPE)
    endif()
endfunction()

# Compile one game unit with MWCC through Satan's Fiddle. The compile is also the
# objdiff base, the source alone, so it lands in objdiff/base; the linked object is that
# compile, copied and postprocessed. The two share one compile.
function(add_cpp_object obj src unit)
    get_filename_component(logical_source "${src}" NAME)
    cpp_lang(${src} lang)
    set(base ${BUILD_DIR}/objdiff/base/${unit}.cpp.o)
    kz_mwcc_object(${base} ${src}
        FLAGS -lang ${lang} ${CC_FLAGS} ${UNIT_CC_FLAGS_${unit}}
        ENV SATANSFIDDLE=${SATANSFIDDLE} SATANSFIDDLE_CONFIG=${SATANSFIDDLE_CONFIG}
            SATANSFIDDLE_TRANSLATION_UNIT=${logical_source}
        DEPENDS ${SATANSFIDDLE_DEPENDENCIES}
        COMMENT "CC ${src}")
    add_linked_object(${obj} ${base} ${src})
endfunction()

# The linked object: the objdiff base, copied and postprocessed.
function(add_linked_object obj base src)
    kz_absolute_path(${obj} obj_abs)
    kz_absolute_path(${base} base_abs)
    add_custom_command(
        OUTPUT ${obj_abs}
        COMMAND ${CMAKE_COMMAND} -E copy ${base_abs} ${obj_abs}
        COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${KZ_PYTHONPATH}
                ${PYTHON} ${SCRIPTS_DIR}/build/postprocess_object.py ${obj}
        COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${KZ_PYTHONPATH}
                sh ${SCRIPTS_DIR}/build/fixup_sections.sh ${obj}
        DEPENDS ${base_abs}
                ${CMAKE_SOURCE_DIR}/${SCRIPTS_DIR}/build/postprocess_object.py
                ${CMAKE_SOURCE_DIR}/${SCRIPTS_DIR}/build/fixup_sections.sh
                ${KZ_PYTHONPATH}/kazarov_elf/__init__.py
                ${KZ_SPLIT_STAMP}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "POST ${src}"
        VERBATIM)
endfunction()

# add_whole_program_objects(UNITS <unit>... [ALONE <unit>...])
#
# Compiles the game units as retail did: every unit sharing a flag set in one MWCC run,
# in the order given (link order), so the helper masks each unit leaves carry into the
# next and need no profile rows. A unit named in ALONE gets a run of its own, because it
# needs the masks a fresh run starts with. Each unit's source is in CPP_SOURCE_<unit>.
# The objects land where add_cpp_object puts them.
function(add_whole_program_objects)
    cmake_parse_arguments(WP "" "" "UNITS;ALONE" ${ARGN})
    set(keys "")
    foreach(unit IN LISTS WP_UNITS)
        cpp_lang(${CPP_SOURCE_${unit}} lang)
        set(flags -lang ${lang} ${CC_FLAGS} ${UNIT_CC_FLAGS_${unit}})
        if(unit IN_LIST WP_ALONE)
            set(key "alone ${unit}")
        else()
            string(JOIN " " key ${flags})
        endif()
        string(MD5 key "${key}")
        if(NOT key IN_LIST keys)
            list(APPEND keys ${key})
            set(flags_${key} "${flags}")
            set(units_${key} "")
        endif()
        list(APPEND units_${key} ${unit})
    endforeach()

    set(base_dir ${BUILD_DIR}/objdiff/base)
    foreach(key IN LISTS keys)
        set(sources "")
        foreach(unit IN LISTS units_${key})
            list(APPEND sources ${CPP_SOURCE_${unit}})
        endforeach()
        list(LENGTH sources count)
        list(GET units_${key} 0 first)
        kz_mwcc_batch(${base_dir}
            SOURCES ${sources}
            FLAGS ${flags_${key}}
            ENV SATANSFIDDLE=${SATANSFIDDLE} SATANSFIDDLE_CONFIG=${SATANSFIDDLE_CONFIG}
                SATANSFIDDLE_IGNORE=helper_masks
            DEPENDS ${SATANSFIDDLE_DEPENDENCIES}
            COMMENT "CC ${count} units in one run, from ${first}"
            OBJECTS objects)
        foreach(unit object IN ZIP_LISTS units_${key} objects)
            kz_absolute_path(${base_dir}/${unit}.cpp.o expected)
            if(NOT object STREQUAL expected)
                message(FATAL_ERROR "${CPP_SOURCE_${unit}} compiles to ${object}, not ${expected}")
            endif()
            add_linked_object(${BUILD_DIR}/obj/${unit}.cpp.o ${base_dir}/${unit}.cpp.o
                ${CPP_SOURCE_${unit}})
        endforeach()
    endforeach()
endfunction()
