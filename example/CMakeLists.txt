cmake_minimum_required(VERSION 3.6)

include_directories(${WORKFLOW_INCLUDE_DIR})

foreach(EXE provider_demo consumer_demo local_test)
    add_executable(${EXE} ${EXE}.cc)
    target_link_libraries(${EXE}
        ${LIBRARY_NAME}
        ${YAML_CPP_LIBRARIES}
        ${WORKFLOW_LIB_DIR}/libworkflow.a
        ssl crypto pthread
    )
endforeach(EXE)
