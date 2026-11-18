file(MAKE_DIRECTORY "${WORK_DIR}")
set(SOURCE "${WORK_DIR}/source.json")
set(SAVED "${WORK_DIR}/session-draft.json")
set(SCRIPT "${WORK_DIR}/commands.txt")
configure_file("${FIXTURE}" "${SOURCE}" COPYONLY)
file(REMOVE "${SAVED}")
file(SHA256 "${SOURCE}" ORIGINAL_HASH)
file(WRITE "${SCRIPT}" "list\nmap 30 30\nmove resource.normal_tree.001 30 30\nstatus\nundo\nstatus\nredo\nmove station.furnace.001 10 10\nsave ${SAVED}\nquit\n")
execute_process(COMMAND "${MOVER}" --session "${SOURCE}"
    INPUT_FILE "${SCRIPT}"
    RESULT_VARIABLE RESULT
    OUTPUT_VARIABLE OUTPUT
    ERROR_VARIABLE ERRORS)
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "interactive session failed: ${RESULT}: ${OUTPUT} ${ERRORS}")
endif()
foreach(EXPECTED "Existing placements" "Map x=" "Moved resource.normal_tree.001" "Undone." "Redone." "Changed: no" "Saved new draft:" "Session closed.")
    string(FIND "${OUTPUT}" "${EXPECTED}" FOUND)
    if(FOUND EQUAL -1)
        message(FATAL_ERROR "session missing ${EXPECTED}: ${OUTPUT}")
    endif()
endforeach()
if(NOT ERRORS MATCHES "walkable ground")
    message(FATAL_ERROR "invalid intermediate move was not rejected: ${ERRORS}")
endif()
if(NOT EXISTS "${SAVED}")
    message(FATAL_ERROR "interactive session did not create its requested draft")
endif()
file(SHA256 "${SOURCE}" AFTER_HASH)
if(NOT AFTER_HASH STREQUAL ORIGINAL_HASH)
    message(FATAL_ERROR "interactive session mutated its source")
endif()
execute_process(COMMAND "${MOVER}" --hash "${SAVED}"
    RESULT_VARIABLE HASH_RESULT OUTPUT_VARIABLE HASH_OUTPUT ERROR_VARIABLE HASH_ERROR)
if(NOT HASH_RESULT EQUAL 0)
    message(FATAL_ERROR "interactive draft fails runtime validation: ${HASH_ERROR}")
endif()
file(SHA256 "${SAVED}" VERIFIED_HASH)
string(STRIP "${HASH_OUTPUT}" HASH_OUTPUT)
if(NOT HASH_OUTPUT STREQUAL VERIFIED_HASH)
    message(FATAL_ERROR "interactive output hash differs from file bytes")
endif()

# A second session cannot overwrite the already-published draft.
file(WRITE "${SCRIPT}" "move resource.normal_tree.001 31 30\nsave ${SAVED}\nquit\n")
execute_process(COMMAND "${MOVER}" --session "${SOURCE}"
    INPUT_FILE "${SCRIPT}" RESULT_VARIABLE RETRY_RESULT
    OUTPUT_VARIABLE RETRY_OUTPUT ERROR_VARIABLE RETRY_ERRORS)
if(NOT RETRY_RESULT EQUAL 0 OR NOT RETRY_ERRORS MATCHES "output already exists")
    message(FATAL_ERROR "existing draft was not protected: ${RETRY_RESULT}: ${RETRY_ERRORS}")
endif()
file(SHA256 "${SAVED}" STILL_SAVED_HASH)
if(NOT VERIFIED_HASH STREQUAL STILL_SAVED_HASH)
    message(FATAL_ERROR "existing draft changed after a rejected publication")
endif()

# An editor pinned to a specific source revision must reject a newer source
# before accepting any move or save command, without leaving an output draft.
set(STALE_DRAFT "${WORK_DIR}/stale-revision-draft.json")
file(REMOVE "${STALE_DRAFT}")
file(APPEND "${SOURCE}" "\n")
file(WRITE "${SCRIPT}" "move resource.normal_tree.001 30 30\nsave ${STALE_DRAFT}\nquit\n")
execute_process(COMMAND "${MOVER}" --session "${SOURCE}" --expected-hash "${ORIGINAL_HASH}"
    INPUT_FILE "${SCRIPT}" RESULT_VARIABLE STALE_RESULT
    OUTPUT_VARIABLE STALE_OUTPUT ERROR_VARIABLE STALE_ERRORS)
if(NOT STALE_RESULT EQUAL 4 OR NOT STALE_ERRORS MATCHES "CONTENT_CONFLICT")
    message(FATAL_ERROR "stale source revision not rejected before commands: ${STALE_RESULT} ${STALE_OUTPUT} ${STALE_ERRORS}")
endif()
if(EXISTS "${STALE_DRAFT}")
    message(FATAL_ERROR "stale source unexpectedly published a draft")
endif()
