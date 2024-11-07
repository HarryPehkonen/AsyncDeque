cmake_minimum_required(VERSION 3.14)
project(async_deque VERSION 0.1.0 LANGUAGES CXX)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add option to build tests, examples, and documentation
option(ASYNC_DEQUE_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
### option(ASYNC_DEQUE_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(ASYNC_DEQUE_BUILD_DOCS "Build documentation" ${PROJECT_IS_TOP_LEVEL})

# Configure threading support
find_package(Threads REQUIRED)

# Create interface library for the header-only library
add_library(async_deque INTERFACE)
add_library(async_deque::async_deque ALIAS async_deque)

# Set up include directories
target_include_directories(async_deque INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link against threading library
target_link_libraries(async_deque INTERFACE Threads::Threads)

# Documentation configuration
if(ASYNC_DEQUE_BUILD_DOCS)
    # Find Doxygen
    find_package(Doxygen)
    
    if(DOXYGEN_FOUND)
        # Set Doxygen input and output directories
        set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/docs)
        
        # Create docs directory
        file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
        
        # Configure Doxyfile
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            @ONLY
        )
        
        # Add documentation target
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        
        # Optional: Make docs part of ALL target
        # set_target_properties(docs PROPERTIES EXCLUDE_FROM_ALL FALSE)
        
        # Install documentation
        install(DIRECTORY ${DOXYGEN_OUTPUT_DIR}/html
            DESTINATION share/doc/${PROJECT_NAME}
            OPTIONAL
        )
    else()
        message(STATUS "Doxygen not found, documentation will not be built")
    endif()
endif()

# Tests configuration
if(ASYNC_DEQUE_BUILD_TESTS)
    enable_testing()
    
    # Find GTest package
    find_package(GTest REQUIRED)
    
    # Create test executable
    add_executable(async_deque_tests tests/async_deque_tests.cpp)
    
    # Set include directories for tests
    target_include_directories(async_deque_tests 
        PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    # Link against necessary libraries
    target_link_libraries(async_deque_tests 
        PRIVATE 
        async_deque
        GTest::GTest 
        GTest::Main
        Threads::Threads
    )
    
    # Enable testing through CTest
    include(GoogleTest)
    gtest_discover_tests(async_deque_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES TIMEOUT 120  # Set test timeout to 120 seconds
    )
    
    # Optional: Add test coverage if gcc or clang
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(async_deque_tests PRIVATE --coverage)
        target_link_options(async_deque_tests PRIVATE --coverage)
    endif()
endif()

# Examples configuration
### if(ASYNC_DEQUE_BUILD_EXAMPLES)
###     add_executable(producer_consumer examples/producer_consumer.cpp)
###     target_link_libraries(producer_consumer PRIVATE 
###         async_deque 
###         Threads::Threads
###     )
### endif()

# Basic install rules
install(DIRECTORY include/ DESTINATION include)
