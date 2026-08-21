add_executable(sparkleResourceBuilder main.cpp)
target_compile_features(sparkleResourceBuilder PRIVATE cxx_std_23)
set_target_properties(sparkleResourceBuilder PROPERTIES
	CXX_EXTENSIONS OFF
	FOLDER tools
)
