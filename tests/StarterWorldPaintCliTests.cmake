# Paint edits are candidate-only unless explicitly committed to a fresh sibling
# draft against the exact source revision. Use a temporary fixture copy.
file(MAKE_DIRECTORY "${WORK_DIR}")
set(SOURCE "${WORK_DIR}/source.json")
set(DRAFT "${WORK_DIR}/painted-draft.json")
set(STALE "${WORK_DIR}/stale-draft.json")
configure_file("${FIXTURE}" "${SOURCE}" COPYONLY)
file(REMOVE "${DRAFT}" "${STALE}")
file(SHA256 "${SOURCE}" SOURCE_HASH)

execute_process(COMMAND "${MOVER}" --paint "${SOURCE}" 30 30 D
    RESULT_VARIABLE DRY_RESULT OUTPUT_VARIABLE DRY_OUT ERROR_VARIABLE DRY_ERR)
if(NOT DRY_RESULT EQUAL 0 OR NOT DRY_OUT MATCHES "Valid paint:"
    OR NOT DRY_OUT MATCHES "no files written")
    message(FATAL_ERROR "dry-run paint rejected: ${DRY_RESULT}: ${DRY_OUT} ${DRY_ERR}")
endif()
execute_process(COMMAND "${MOVER}" --paint "${SOURCE}" 30 30 G
    RESULT_VARIABLE NOOP_RESULT OUTPUT_VARIABLE NOOP_OUT ERROR_VARIABLE NOOP_ERR)
if(NOT NOOP_RESULT EQUAL 0 OR NOT NOOP_OUT MATCHES "Valid unchanged terrain:")
    message(FATAL_ERROR "valid no-op paint failed: ${NOOP_RESULT}: ${NOOP_OUT} ${NOOP_ERR}")
endif()
foreach(CASE "5;5;W" "0;0;W" "30;30;X" "100;30;D")
    list(GET CASE 0 X)
    list(GET CASE 1 Y)
    list(GET CASE 2 SYMBOL)
    execute_process(COMMAND "${MOVER}" --paint "${SOURCE}" "${X}" "${Y}" "${SYMBOL}"
        RESULT_VARIABLE BAD_RESULT OUTPUT_VARIABLE BAD_OUT ERROR_VARIABLE BAD_ERR)
    if(NOT BAD_RESULT EQUAL 3)
        message(FATAL_ERROR "invalid paint ${CASE} accepted: ${BAD_RESULT}: ${BAD_OUT} ${BAD_ERR}")
    endif()
endforeach()
file(SHA256 "${SOURCE}" AFTER_DRY)
if(NOT AFTER_DRY STREQUAL SOURCE_HASH)
    message(FATAL_ERROR "dry-run changed the source")
endif()

execute_process(COMMAND "${MOVER}" --paint "${SOURCE}" 30 30 D
    --expected-hash "${SOURCE_HASH}" --write-draft "${DRAFT}"
    RESULT_VARIABLE WRITE_RESULT OUTPUT_VARIABLE WRITE_OUT ERROR_VARIABLE WRITE_ERR)
if(NOT WRITE_RESULT EQUAL 0 OR NOT WRITE_OUT MATCHES "Saved new draft:" OR NOT EXISTS "${DRAFT}")
    message(FATAL_ERROR "guarded paint did not save draft: ${WRITE_RESULT}: ${WRITE_OUT} ${WRITE_ERR}")
endif()
execute_process(COMMAND "${MOVER}" --hash "${DRAFT}"
    RESULT_VARIABLE CHECK_RESULT OUTPUT_VARIABLE CHECK_HASH ERROR_VARIABLE CHECK_ERR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT CHECK_RESULT EQUAL 0 OR CHECK_HASH STREQUAL SOURCE_HASH)
    message(FATAL_ERROR "saved paint draft does not validate or had no change: ${CHECK_RESULT}: ${CHECK_HASH} ${CHECK_ERR}")
endif()
file(SHA256 "${DRAFT}" DRAFT_HASH)
if(NOT CHECK_HASH STREQUAL DRAFT_HASH)
    message(FATAL_ERROR "validated draft hash does not match actual file")
endif()
execute_process(COMMAND "${MOVER}" --paint "${SOURCE}" 30 30 S
    --expected-hash "${SOURCE_HASH}" --write-draft "${DRAFT}"
    RESULT_VARIABLE EXISTS_RESULT OUTPUT_VARIABLE EXISTS_OUT ERROR_VARIABLE EXISTS_ERR)
if(NOT EXISTS_RESULT EQUAL 4 OR NOT EXISTS_ERR MATCHES "output already exists")
    message(FATAL_ERROR "existing draft was not protected: ${EXISTS_RESULT}: ${EXISTS_ERR}")
endif()
file(APPEND "${SOURCE}" "\n")
execute_process(COMMAND "${MOVER}" --paint "${SOURCE}" 30 30 S
    --expected-hash "${SOURCE_HASH}" --write-draft "${STALE}"
    RESULT_VARIABLE STALE_RESULT OUTPUT_VARIABLE STALE_OUT ERROR_VARIABLE STALE_ERR)
if(NOT STALE_RESULT EQUAL 4 OR NOT STALE_ERR MATCHES "CONTENT_CONFLICT" OR EXISTS "${STALE}")
    message(FATAL_ERROR "stale paint source was not protected: ${STALE_RESULT}: ${STALE_ERR}")
endif()
file(SHA256 "${DRAFT}" AFTER_REJECTED)
if(NOT AFTER_REJECTED STREQUAL DRAFT_HASH)
    message(FATAL_ERROR "rejected operation mutated existing draft")
endif()
