#include "graphics/pipeline/spk_pipeline.hpp"

#include "system/spk_chronometer.hpp"

namespace spk
{
	std::string executeIncludeLines(const std::string& p_source);

    std::string composeVertexCode(const std::string& p_inputCode);
    std::string composeFragmentCode(const std::string& p_inputCode);

	GLuint loadProgram(const std::string& p_vertexCode, const std::string& p_fragmentCode);

	void Pipeline::_load()
	{
		_programID = loadProgram(_vertexString, _fragmentString);

		for (const auto& [key, layout] : _constantsLayout)
		{
			if (_constants.contains(key) == false)
			{
				_constants.emplace(key, Constant(_programID, layout));
			}
			
			_activeConstants.push_back(&_constants.at(key));
		}

		for (auto& [key, layout] : _samplersLayout)
		{
			_textures.emplace(key, Texture(_programID, layout, _constants.at(layout.name.substr(0, layout.name.size() - 7))));
		}

		_isLoaded = true;
	}

	Pipeline::Pipeline()
	{
        _isLoaded = false;

		_vertexString = "";
		_fragmentString = "";
	}

	Pipeline::Pipeline(const std::string& p_code)
	{
        _isLoaded = false;

		std::string inputCode = executeIncludeLines(p_code);
		
		_vertexString = composeVertexCode(inputCode);
		_fragmentString = composeFragmentCode(inputCode);

		std::vector<OpenGL::ShaderInstruction> shaderInstructions = OpenGL::ShaderInstruction::parseShaderInstruction(_vertexString);

		_loadStructureFromInstructions(shaderInstructions);

		_storageLayout = _parseStorageLayout(shaderInstructions);
		_attributesLayout = _parseUniformLayout(shaderInstructions, Pipeline::Object::Attribute::BlockKey);
		_constantsLayout = _parseUniformLayout(shaderInstructions, Pipeline::Constant::BlockKey);
		_samplersLayout = _parseSamplerUniforms(shaderInstructions);
	}
	
	Pipeline::~Pipeline()
	{
		if (_isLoaded == true)
			glDeleteProgram(_programID);
	}

	void Pipeline::activate()
	{
		if (_isLoaded == false)
			_load();
		glUseProgram(_programID);
	}

	void Pipeline::launch(size_t p_nbTriangle)
	{
		for (auto& constant : _activeConstants)
		{
			constant->activate();
		}
			
		for (auto& [key, texture] : _textures)
			texture._activate();

		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(p_nbTriangle), GL_UNSIGNED_INT, nullptr);
	}

	Pipeline::Object Pipeline::createObject()
	{
		if (_isLoaded == false)
		{
			_load();
		}
		return (Object(this, _storageLayout, _attributesLayout));
	}

	Pipeline::Constant* Pipeline::globalConstant(const std::string& p_constantName)
	{
		if (_constants.contains(p_constantName) == false)
			return (nullptr);
		return (&(_constants.at(p_constantName)));
	}

	Pipeline::Constant& Pipeline::constant(const std::string& p_constantName)
	{
		if (_isLoaded == false)
			_load();
		if (_constants.contains(p_constantName) == false)
			throwException("Pipeline don't contain constant [" + p_constantName + "]");
		return (_constants.at(p_constantName));
	}

	Pipeline::Texture& Pipeline::texture(const std::string& p_textureName)
	{
		if (_isLoaded == false)
			_load();
		if (_textures.contains(p_textureName + "_handle") == false)
			throwException("Pipeline don't contain texture [" + p_textureName + "]");
		return (_textures.at(p_textureName + "_handle"));
	}
}