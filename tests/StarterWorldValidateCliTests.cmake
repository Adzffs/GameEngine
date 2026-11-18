cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED VALIDATOR OR NOT DEFINED FIXTURE OR NOT DEFINED WORK_DIR)
    message(FATAL_ERROR "Validator, fixture and isolated work directory are required")
endif()
file(MAKE_DIRECTORY "${WORK_DIR}")
file(READ "${FIXTURE}" fixture_before)
file(GLOB_RECURSE before LIST_DIRECTORIES true "${WORK_DIR}/*")
execute_process(COMMAND "${VALIDATOR}" "${FIXTURE}"
    WORKING_DIRECTORY "${WORK_DIR}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result STREQUAL "0" OR NOT errors STREQUAL "" OR
    NOT output MATCHES "Valid starter world: 100 x 100, 20 placements")
    message(FATAL_ERROR "Valid baseline was rejected (${result}): ${errors}")
endif()
execute_process(COMMAND "${VALIDATOR}" "--help"
    WORKING_DIRECTORY "${WORK_DIR}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result STREQUAL "0" OR output STREQUAL "" OR NOT errors STREQUAL "")
    message(FATAL_ERROR "Validator help failed")
endif()
foreach(arguments IN ITEMS "" "--unknown" "--help;extra")
    execute_process(COMMAND "${VALIDATOR}" ${arguments}
        WORKING_DIRECTORY "${WORK_DIR}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
    if(NOT result STREQUAL "2" OR NOT output STREQUAL "" OR errors STREQUAL "")
        message(FATAL_ERROR "Bad validator arguments did not fail cleanly: ${arguments}")
    endif()
endforeach()
foreach(path IN ITEMS "${WORK_DIR}/missing.json" "${WORK_DIR}")
    execute_process(COMMAND "${VALIDATOR}" "${path}"
        WORKING_DIRECTORY "${WORK_DIR}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
    if(NOT result STREQUAL "1" OR NOT output STREQUAL "" OR errors STREQUAL "")
        message(FATAL_ERROR "Invalid file target did not fail cleanly: ${path}")
    endif()
endforeach()
file(READ "${FIXTURE}" fixture_after)
file(GLOB_RECURSE after LIST_DIRECTORIES true "${WORK_DIR}/*")
if(NOT fixture_before STREQUAL fixture_after OR NOT before STREQUAL after)
    message(FATAL_ERROR "Validator modified content or created a file")
endif()
message(STATUS "Runtime snapshot validation CLI checks passed")
