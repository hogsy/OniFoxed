if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
		set(BINKW32_IMPLIB "${CMAKE_CURRENT_BINARY_DIR}/binkw32.dll.lib")
		add_custom_command(
			OUTPUT "${BINKW32_IMPLIB}"
			COMMAND ${CMAKE_AR} ${CMAKE_STATIC_LINKER_FLAGS} /DEF:binkw32.def /OUT:"${BINKW32_IMPLIB}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			DEPENDS binkw32.def
		)
	else()
		set(BINKW32_IMPLIB "${CMAKE_CURRENT_BINARY_DIR}/libbinkw32.dll.a")
		add_custom_command(
			OUTPUT "${BINKW32_IMPLIB}"
			COMMAND ${CMAKE_DLLTOOL} ${CMAKE_STATIC_LINKER_FLAGS} --input-def binkw32.def --output-lib "${BINKW32_IMPLIB}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			DEPENDS binkw32.def
		)
	endif()
	add_custom_target(binkw32-implib DEPENDS "${BINKW32_IMPLIB}")
	target_sources(binkw32-implib PRIVATE binkw32.def)
	set(BINK_LIB "${BINKW32_IMPLIB}")
	set(BINK_DEP "binkw32-implib")
endif()

if(BINK_LIB)
	add_library(bink INTERFACE)
	target_link_libraries(bink
		INTERFACE
			"${BINK_LIB}"
	)
	add_dependencies(bink ${BINK_DEP})
endif()
