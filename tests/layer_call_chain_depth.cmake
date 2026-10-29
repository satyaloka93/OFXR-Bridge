# Runs one call-chain scenario at the pipeline depth PIPELINE_INI sets. The
# layer reads `deep_pipeline` from the ofxr_bridge.ini beside its DLL, and the
# shipped ini turns it on, so the other depth runs from a scratch directory
# with an ini of its own.
foreach(required_variable IN ITEMS LAYER_DLL CALL_CHAIN PIPELINE_INI WORK_DIR MODE)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} was not provided")
    endif()
endforeach()

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")
file(COPY "${LAYER_DLL}" DESTINATION "${WORK_DIR}")
file(COPY "${PIPELINE_INI}" DESTINATION "${WORK_DIR}")
get_filename_component(pipeline_ini_name "${PIPELINE_INI}" NAME)
file(RENAME "${WORK_DIR}/${pipeline_ini_name}" "${WORK_DIR}/ofxr_bridge.ini")

# With QUEUE_LAYER_DLL, the bridge's Vulkan queue layer is put in the test
# process's layer chain through the loader's explicit-layer path, and the
# call chain checks that it loaded.
if(DEFINED QUEUE_LAYER_DLL)
    file(COPY "${QUEUE_LAYER_DLL}" DESTINATION "${WORK_DIR}")
    get_filename_component(queue_layer_name "${QUEUE_LAYER_DLL}" NAME)
    file(TO_NATIVE_PATH "${WORK_DIR}/${queue_layer_name}" queue_layer_native)
    string(REPLACE "\\" "\\\\" queue_layer_json "${queue_layer_native}")
    file(WRITE "${WORK_DIR}/VK_LAYER_OFXR_queue_serialize.json"
"{
  \"file_format_version\": \"1.2.0\",
  \"layer\": {
    \"name\": \"VK_LAYER_OFXR_queue_serialize\",
    \"type\": \"GLOBAL\",
    \"library_path\": \"${queue_layer_json}\",
    \"api_version\": \"1.3.296\",
    \"implementation_version\": \"1\",
    \"description\": \"OFXR queue layer under test\",
    \"functions\": {
      \"vkNegotiateLoaderLayerInterfaceVersion\": \"OFXR_vkNegotiateLoaderLayerInterfaceVersion\",
      \"vkGetInstanceProcAddr\": \"OFXR_vkGetInstanceProcAddr\",
      \"vkGetDeviceProcAddr\": \"OFXR_vkGetDeviceProcAddr\"
    }
  }
}
")
    file(TO_NATIVE_PATH "${WORK_DIR}" work_dir_native)
    set(ENV{VK_LAYER_PATH} "${work_dir_native}")
    set(ENV{VK_INSTANCE_LAYERS} "VK_LAYER_OFXR_queue_serialize")
    set(ENV{OFXR_TEST_EXPECT_QUEUE_LAYER} "1")
endif()

get_filename_component(layer_name "${LAYER_DLL}" NAME)
set(mode_arguments "${MODE}")
if(MODE STREQUAL "default")
    set(mode_arguments "")
endif()
execute_process(
    COMMAND "${CALL_CHAIN}"
        "${WORK_DIR}/${layer_name}"
        "${WORK_DIR}/fake-runtime.log"
        ${mode_arguments}
    RESULT_VARIABLE call_chain_result
    OUTPUT_VARIABLE call_chain_output
    ERROR_VARIABLE call_chain_error)
if(NOT call_chain_result EQUAL 0)
    message(FATAL_ERROR
        "Call chain '${MODE}' at the other pipeline depth failed (${call_chain_result}):\n"
        "${call_chain_output}\n${call_chain_error}")
endif()

if(EXPECT_INDEPENDENT_QUEUES)
    file(GLOB flight_logs "${WORK_DIR}/ofxr-bridge-flight-*.log")
    set(queue_ids "")
    foreach(flight_log IN LISTS flight_logs)
        file(READ "${flight_log}" trace)
        string(REGEX MATCHALL "op=presenter_transition result=951 dur_us=0 a=[0-9]+ b=[0-9]+ c=1" records "${trace}")
        foreach(record IN LISTS records)
            string(REGEX REPLACE ".* b=([0-9]+) c=1" "\\1" queue_id "${record}")
            list(APPEND queue_ids "${queue_id}")
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES queue_ids)
    list(LENGTH queue_ids queue_count)
    if(NOT queue_count EQUAL 2)
        message(FATAL_ERROR "Expected two distinct independent eye queues, got ${queue_count}: ${queue_ids}")
    endif()
endif()
