file(MAKE_DIRECTORY "${WORK_DIR}")
set(INPUT "${WORK_DIR}/source.json")
set(DRAFT "${WORK_DIR}/revision.json")
configure_file("${FIXTURE}" "${INPUT}" COPYONLY)
file(REMOVE "${DRAFT}")
file(SHA256 "${INPUT}" initial_sha)
execute_process(COMMAND "${MOVER}" --hash "${INPUT}"
    RESULT_VARIABLE hash_status OUTPUT_VARIABLE hash_output ERROR_VARIABLE hash_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT hash_status EQUAL 0 OR NOT hash_output STREQUAL initial_sha)
    message(FATAL_ERROR "hash inspection does not match file bytes: ${hash_status}: ${hash_output} ${hash_error}")
endif()
execute_process(COMMAND "${MOVER}" "${INPUT}" resource.normal_tree.001 30 30
    RESULT_VARIABLE good OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT good EQUAL 0 OR NOT output MATCHES "Valid move:" OR NOT output MATCHES "no files written")
    message(FATAL_ERROR "dry-run move failed: ${good}: ${output} ${error}")
endif()
execute_process(COMMAND "${MOVER}" "${INPUT}" resource.normal_tree.001 10 10
    RESULT_VARIABLE bad OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT bad EQUAL 3 OR NOT error MATCHES "walkable ground")
    message(FATAL_ERROR "invalid destination was not rejected: ${bad}: ${output} ${error}")
endif()
execute_process(COMMAND "${MOVER}" "${INPUT}" npc.development_guide.001 30 30
    RESULT_VARIABLE actor OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT actor EQUAL 0 OR NOT output MATCHES "Valid move:" OR NOT output MATCHES "no files written")
    message(FATAL_ERROR "supported actor dry-run move failed: ${actor}: ${output} ${error}")
endif()
execute_process(COMMAND "${MOVER}" "${INPUT}" resource.normal_tree.001 hello 30
    RESULT_VARIABLE usage OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT usage EQUAL 2 OR NOT error MATCHES "Usage:")
    message(FATAL_ERROR "invalid syntax was not rejected: ${usage}: ${output} ${error}")
endif()
execute_process(COMMAND "${MOVER}" "${INPUT}" resource.normal_tree.001 30 30
    --expected-hash "${initial_sha}" --write-draft "${DRAFT}"
    RESULT_VARIABLE write_status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT write_status EQUAL 0 OR NOT output MATCHES "Saved new draft:")
    message(FATAL_ERROR "guarded new-draft write failed: ${write_status}: ${output} ${error}")
endif()
file(SHA256 "${INPUT}" unchanged_sha)
if(NOT unchanged_sha STREQUAL initial_sha)
    message(FATAL_ERROR "source file changed during guarded write")
endif()
execute_process(COMMAND "${MOVER}" --hash "${DRAFT}"
    RESULT_VARIABLE draft_status OUTPUT_VARIABLE draft_hash ERROR_VARIABLE error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT draft_status EQUAL 0 OR draft_hash STREQUAL initial_sha)
    message(FATAL_ERROR "saved draft is unreadable or has no semantic change: ${draft_status}: ${draft_hash} ${error}")
endif()
execute_process(COMMAND "${MOVER}" "${INPUT}" resource.normal_tree.001 31 30
    --expected-hash "${initial_sha}" --write-draft "${DRAFT}"
    RESULT_VARIABLE exists_status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT exists_status EQUAL 4)
    message(FATAL_ERROR "existing target was not protected: ${exists_status}: ${error}")
endif()
file(SHA256 "${DRAFT}" draft_before)
file(APPEND "${INPUT}" "\n")
execute_process(COMMAND "${MOVER}" "${INPUT}" resource.normal_tree.001 31 30
    --expected-hash "${initial_sha}" --write-draft "${WORK_DIR}/stale.json"
    RESULT_VARIABLE stale_status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT stale_status EQUAL 4 OR NOT error MATCHES "CONTENT_CONFLICT")
    message(FATAL_ERROR "stale revision was not rejected: ${stale_status}: ${error}")
endif()
if(EXISTS "${WORK_DIR}/stale.json")
    message(FATAL_ERROR "conflicting revision created a draft")
endif()
file(SHA256 "${DRAFT}" draft_after)
if(NOT draft_before STREQUAL draft_after)
    message(FATAL_ERROR "stale overwrite attempt changed existing draft")
endif()
