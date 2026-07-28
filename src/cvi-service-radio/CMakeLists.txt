cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(cvi-service-radio
        VERSION 1.0.0
        LANGUAGES CXX C
)

add_definitions(-Wall)
add_definitions("-fPIC -Wno-deprecated -Werror")

# Set Compiler Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall  -Wtrampolines -fstack-protector-all -Wformat -Wformat-security  -D_FORTIFY_SOURCE=2 -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++11 -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_C99 -Wtrampolines -fstack-protector-all -Wformat -Wformat-security -D_FORTIFY_SOURCE=2") #-Werror
# -Wextra
set (CMAKE_CXX_FLAGS_DEBUG     "-O0 -g")
set (CMAKE_CXX_FLAGS_RELEASE   "-O2 -DNDEBUG ")

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64le")
    set(PLATFORM_LIB dlt)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(PLATFORM_LIB )
endif()

set(PUBLIC_INCS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(PRIVATE_INCS
        $<BUILD_INTERFACE:$ENV{CVIOS_INCLUDES}/>
        $<BUILD_INTERFACE:$ENV{CVIOS_INCLUDES}/cvi/ipc>
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/framework SRC_FRAMEWORK)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/projectcode SRC_PROJECTCODE)

add_executable(${PROJECT_NAME}  ${SRC_FRAMEWORK} ${SRC_PROJECTCODE})
target_link_libraries(${PROJECT_NAME}  ${PLATFORM_LIB} cvi-lib-log spdlog)
target_include_directories(${PROJECT_NAME}
        PUBLIC  ${PUBLIC_INCS}
        PRIVATE ${PRIVATE_INCS})
target_link_directories(${PROJECT_NAME} PRIVATE $ENV{CVIOS_LIBRARY_DIRECTORY})

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64le")
    target_link_libraries(${PROJECT_NAME} slog2)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION
        EXPORT ${PROJECT_NAME}_targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin/${PROJECT_NAME})
