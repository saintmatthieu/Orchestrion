# Define the script and stamp file
set(LRELEASE_SCRIPT ${CMAKE_SOURCE_DIR}/tools/translations/run_lrelease.py)
set(LRELEASE_STAMP ${CMAKE_BINARY_DIR}/lrelease.stamp)

# Create a custom target to run the script and generate the stamp
add_custom_target(run_lrelease ALL
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_SOURCE_DIR}/tools/translations python3 ${LRELEASE_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    BYPRODUCTS ${LRELEASE_STAMP}
    COMMAND ${CMAKE_COMMAND} -E touch ${LRELEASE_STAMP}
    COMMENT "Running lrelease to compile translations"
    VERBATIM
)

# Make the install target depend on the stamp file
install(CODE "include(\"${LRELEASE_STAMP}\")")