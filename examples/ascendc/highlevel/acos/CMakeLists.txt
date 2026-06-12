cmake_minimum_required(VERSION 3.16)
project(ascendc_acos_sim)

# Run mode: sim = link against the CAModel simulation runtime (card-free / no-NPU simulation)
set(RUN_MODE "sim" CACHE STRING "run mode: cpu/sim/npu")
set(SOC_VERSION "Ascend950PR_9599" CACHE STRING "system on chip type")
set(ASCEND_CANN_PACKAGE_PATH $ENV{ASCEND_TOOLKIT_HOME} CACHE PATH "ASCEND CANN package install dir")
set(ASCEND_PYTHON_EXECUTABLE "python3" CACHE STRING "ascend python executable")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Locate the AscendC kernel cmake infrastructure inside the toolkit
if(EXISTS ${ASCEND_CANN_PACKAGE_PATH}/compiler/tikcpp/ascendc_kernel_cmake)
    set(ASCENDC_CMAKE_DIR ${ASCEND_CANN_PACKAGE_PATH}/compiler/tikcpp/ascendc_kernel_cmake)
elseif(EXISTS ${ASCEND_CANN_PACKAGE_PATH}/tools/tikcpp/ascendc_kernel_cmake)
    set(ASCENDC_CMAKE_DIR ${ASCEND_CANN_PACKAGE_PATH}/tools/tikcpp/ascendc_kernel_cmake)
else()
    message(FATAL_ERROR "ascendc_kernel_cmake not found under ${ASCEND_CANN_PACKAGE_PATH}")
endif()
include(${ASCENDC_CMAKE_DIR}/ascendc.cmake)

# Compile the device-side kernel; auto-generates the host direct-launch stub header aclrtlaunch_k_custom.h
ascendc_library(kernels STATIC kernel.cpp)

# host-side executable
add_executable(sim_exe main.cpp)
target_include_directories(sim_exe PRIVATE ${ASCEND_CANN_PACKAGE_PATH}/include)
target_link_libraries(sim_exe PRIVATE kernels)

# The simulation runtime's transitive dependencies live in this SOC's lib/camodel dir; symbols are resolved at runtime by CAModel
set(SIM_DIR ${ASCEND_CANN_PACKAGE_PATH}/tools/simulator/${SOC_VERSION})
target_link_directories(sim_exe PRIVATE ${SIM_DIR}/lib ${SIM_DIR}/camodel)
target_link_options(sim_exe PRIVATE
    -Wl,--allow-shlib-undefined
    -Wl,-rpath-link,${SIM_DIR}/lib
    -Wl,-rpath-link,${SIM_DIR}/camodel)
