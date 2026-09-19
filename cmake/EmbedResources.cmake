function(sparkle_embed_resources)
	set(options)
	set(oneValueArgs TARGET ROOT)
	set(multiValueArgs FILES)
	cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if(NOT ARG_TARGET OR NOT TARGET "${ARG_TARGET}")
		message(FATAL_ERROR "sparkle_embed_resources requires an existing TARGET")
	endif()
	if(NOT ARG_ROOT OR NOT IS_DIRECTORY "${ARG_ROOT}")
		message(FATAL_ERROR "sparkle_embed_resources ROOT must be an existing directory")
	endif()
	if(NOT TARGET sparkleResourceBuilder)
		message(FATAL_ERROR "sparkle_embed_resources requires sparkleResourceBuilder")
	endif()
	list(SORT ARG_FILES)

	set(resource_dependencies)
	foreach(resource IN LISTS ARG_FILES)
		if(IS_ABSOLUTE "${resource}")
			message(FATAL_ERROR "Resource paths must be relative to ROOT: ${resource}")
		endif()
		list(APPEND resource_dependencies "${ARG_ROOT}/${resource}")
	endforeach()

	set(generated_source "${CMAKE_CURRENT_BINARY_DIR}/generated/spk_generated_resources.cpp")
	add_custom_command(
		OUTPUT "${generated_source}"
		COMMAND $<TARGET_FILE:sparkleResourceBuilder>
			"${ARG_ROOT}"
			"${generated_source}"
			${ARG_FILES}
		DEPENDS sparkleResourceBuilder ${resource_dependencies}
		COMMENT "Embedding Sparkle resources"
		VERBATIM
	)

	set_source_files_properties("${generated_source}" PROPERTIES GENERATED TRUE)
	target_sources("${ARG_TARGET}" PRIVATE "${generated_source}" ${resource_dependencies})
	source_group("resources" FILES ${resource_dependencies})
endfunction()
