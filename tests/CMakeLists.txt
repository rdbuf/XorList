# Download and unpack googletest at configure time
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.in ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                 ${CMAKE_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

add_executable(run tests.cc)
target_link_libraries(run gtest gtest_main XorList)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-stdlib=libc++ -fprofile-instr-generate -fcoverage-mapping")
	get_target_property(XORLIST_INCLUDE_DIR XorList INTERFACE_INCLUDE_DIRECTORIES)
	file(GLOB XORLIST_INCLUDE_HEADERS ${XORLIST_INCLUDE_DIR}*.hpp)
	add_custom_command( # generate coverage analysis helper
		TARGET run
		POST_BUILD
		VERBATIM
		COMMAND truncate -s 0 coverage.sh
		COMMAND sh -c
			"echo '[[ -z $LLVM_PROFILE_FILE ]] && export LLVM_PROFILE_FILE=default.profraw' >>coverage.sh"
		COMMAND sh -c
			"echo '[[ ! -f $LLVM_PROFILE_FILE ]] && echo Run binary first && exit 1 || llvm-profdata merge -sparse $LLVM_PROFILE_FILE -o $<TARGET_FILE_DIR:run>/tests.profdata' >>coverage.sh"
		COMMAND sh -c
			"echo llvm-cov show -show-line-counts-or-regions -Xdemangler c++filt -instr-profile=$<TARGET_FILE_DIR:run>/tests.profdata $<TARGET_FILE:run> ${XORLIST_INCLUDE_HEADERS} >>coverage.sh"
		COMMAND sh -c
			"echo llvm-cov report -instr-profile=$<TARGET_FILE_DIR:run>/tests.profdata $<TARGET_FILE:run> ${XORLIST_INCLUDE_HEADERS} >>coverage.sh" # for some incredible reason, report ignores the SOURCES arg
		COMMAND "chmod" "+x" "coverage.sh"
	)
endif()
