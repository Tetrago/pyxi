cmake_minimum_required(VERSION 3.20)
project(pyxi LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_11)

target_include_directories(${PROJECT_NAME} INTERFACE "${PROJECT_SOURCE_DIR}")

option(PYXI_BUILD_TESTS "Build unit tests" ON)
if(PYXI_BUILD_TESTS)
	include(FetchContent)
	FetchContent_Declare(
		googletest
		URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
	)

	if(WIN32)
		set(gtest_force_shared_crt
			ON
			CACHE BOOL "" FORCE
		)
	endif()

	FetchContent_MakeAvailable(googletest)

	enable_testing()

	add_executable(${PROJECT_NAME}_test "test.cpp")

	target_compile_features(${PROJECT_NAME}_test PRIVATE cxx_std_14)

	target_link_libraries(
		${PROJECT_NAME}_test PRIVATE ${PROJECT_NAME} GTest::gtest_main
	)

	if(CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
		target_compile_definitions(
			${PROJECT_NAME}_test PRIVATE CMAKE_BIG_ENDIAN
		)
	elseif(CMAKE_CXX_BYTE_ORDER STREQUAL "LITTLE_ENDIAN")
		target_compile_definitions(
			${PROJECT_NAME}_test PRIVATE CMAKE_LITTLE_ENDIAN
		)
	endif()

	include(GoogleTest)
	gtest_discover_tests(${PROJECT_NAME}_test)
endif()
