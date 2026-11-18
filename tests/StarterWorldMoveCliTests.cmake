execute_process(
    COMMAND "${MOVER}" "${FIXTURE}" resource.normal_tree.001 30 30
    RESULT_VARIABLE good OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT good EQUAL 0 OR NOT output MATCHES "Valid move:" OR NOT output MATCHES "no files written")
    message(FATAL_ERROR "dry-run move failed: ${good}: ${output} ${error}")
endif()
execute_process(
    COMMAND "${MOVER}" "${FIXTURE}" resource.normal_tree.001 10 10
    RESULT_VARIABLE bad OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT bad EQUAL 1 OR NOT error MATCHES "walkable ground")
    message(FATAL_ERROR "invalid destination was not rejected: ${bad}: ${output} ${error}")
endif()
execute_process(
    COMMAND "${MOVER}" "${FIXTURE}" npc.development_guide.001 30 30
    RESULT_VARIABLE actor OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT actor EQUAL 1 OR NOT error MATCHES "not yet supported")
    message(FATAL_ERROR "unsupported spawn move was not rejected: ${actor}: ${output} ${error}")
endif()
execute_process(
    COMMAND "${MOVER}" "${FIXTURE}" resource.normal_tree.001 hello 30
    RESULT_VARIABLE usage OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT usage EQUAL 2 OR NOT error MATCHES "Usage:")
    message(FATAL_ERROR "invalid syntax was not rejected: ${usage}: ${output} ${error}")
endif()
