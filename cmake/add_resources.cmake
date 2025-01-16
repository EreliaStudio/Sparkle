add_subdirectory(helper)

function(add_resources)
    set(_resource_files ${ARGV})

    set(_generated_header "${CMAKE_BINARY_DIR}/spk_generated_resources.hpp")

    set(_base_dir "${CMAKE_SOURCE_DIR}")

    add_custom_command(
        OUTPUT "${_generated_header}"
        COMMAND sparkleResourcesConverter
                "${_generated_header}"
                "${_base_dir}"
                ${_resource_files}
        DEPENDS sparkleResourcesConverter ${_resource_files}
        COMMENT "Generating embedded resource header: ${_generated_header}"
        VERBATIM
    )

    add_custom_target(SparkleResourcesHeaderTarget
        DEPENDS "${_generated_header}"
    )
endfunction()