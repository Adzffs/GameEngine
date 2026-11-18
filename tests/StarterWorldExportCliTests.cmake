cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED EXPORTER OR NOT DEFINED FIXTURE OR NOT DEFINED WORK_DIR)
    message(FATAL_ERROR "Exporter, fixture and isolated working directory are required")
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")
file(GLOB_RECURSE before LIST_DIRECTORIES true "${WORK_DIR}/*")

foreach(attempt RANGE 1 2)
    execute_process(
        COMMAND "${EXPORTER}"
        WORKING_DIRECTORY "${WORK_DIR}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE snapshot_${attempt}
        ERROR_VARIABLE errors
    )
    if(NOT result STREQUAL "0" OR NOT errors STREQUAL "")
        message(FATAL_ERROR "Export failed (${result}): ${errors}")
    endif()
endforeach()

if(NOT snapshot_1 STREQUAL snapshot_2)
    message(FATAL_ERROR "Repeated exports changed bytes")
endif()

# CMake's independent JSON reader verifies the actual executable's output;
# no third-party parser or Python is required to run this integration test.
string(JSON format GET "${snapshot_1}" format)
string(JSON version GET "${snapshot_1}" schema_version)
string(JSON region GET "${snapshot_1}" region_id)
string(JSON width GET "${snapshot_1}" size width)
string(JSON height GET "${snapshot_1}" size height)
string(JSON row_count LENGTH "${snapshot_1}" terrain rows)
string(JSON placement_count LENGTH "${snapshot_1}" placements)
if(NOT format STREQUAL "embervale.starter-world-snapshot" OR
   NOT version EQUAL 1 OR NOT region STREQUAL "embervale.development_starter" OR
   NOT width EQUAL 100 OR NOT height EQUAL 100 OR
   NOT row_count EQUAL 100 OR NOT placement_count EQUAL 20)
    message(FATAL_ERROR "Snapshot envelope or complete starter counts changed")
endif()

foreach(row_index RANGE 0 99)
    string(JSON row GET "${snapshot_1}" terrain rows ${row_index})
    string(LENGTH "${row}" row_width)
    if(NOT row_width EQUAL 100 OR NOT row MATCHES "^[GDSW]+$")
        message(FATAL_ERROR "Terrain row ${row_index} is incomplete or unsupported")
    endif()
endforeach()

file(READ "${FIXTURE}" expected)
if(NOT snapshot_1 STREQUAL expected)
    message(FATAL_ERROR
        "Export differs from the reviewed starter-world fixture. Inspect semantic changes before updating it.")
endif()

foreach(flag IN ITEMS --help --version)
    execute_process(
        COMMAND "${EXPORTER}" "${flag}"
        WORKING_DIRECTORY "${WORK_DIR}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors
    )
    if(NOT result STREQUAL "0" OR output STREQUAL "" OR NOT errors STREQUAL "")
        message(FATAL_ERROR "${flag} failed (${result}): ${errors}")
    endif()
endforeach()

foreach(arguments IN ITEMS --unknown "--help;unexpected" "--output;unused.json")
    execute_process(
        COMMAND "${EXPORTER}" ${arguments}
        WORKING_DIRECTORY "${WORK_DIR}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors
    )
    if(NOT result STREQUAL "2" OR NOT output STREQUAL "" OR errors STREQUAL "")
        message(FATAL_ERROR "Unsupported arguments did not fail cleanly: ${arguments}")
    endif()
endforeach()

file(GLOB_RECURSE after LIST_DIRECTORIES true "${WORK_DIR}/*")
if(NOT before STREQUAL after)
    message(FATAL_ERROR "The exporter created files in its working directory")
endif()

message(STATUS "Canonical starter-world CLI output, JSON, fixture and read-only behaviour passed")
