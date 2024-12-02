#include "structure/graphics/spk_pipeline.hpp"

#include "utils/spk_string_utils.hpp"

#include <unordered_map>
#include <regex>
#include <string>

namespace spk
{
	Pipeline::ShaderParser::ConstantInformation::ConstantInformation(const spk::OpenGL::UniformBufferObject::Factory& factory, int bindingPoint, size_t size)
		: factory(factory), bindingPoint(bindingPoint), size(size) {}

	std::string Pipeline::ShaderParser::_getFileSection(const std::string& p_inputCode, const std::string& p_delimiter)
	{

		auto startPos = p_inputCode.find(p_delimiter);
		if (startPos == std::string::npos)
		{
			throw std::runtime_error("Delimiter [" + p_delimiter + "] not found in input code.");
		}

		startPos += p_delimiter.length() + 1;

		auto endPos = p_inputCode.find("##", startPos);
		if (endPos == std::string::npos)
		{
			endPos = p_inputCode.length();
		}

		return p_inputCode.substr(startPos, endPos - startPos);
	}

	std::string Pipeline::ShaderParser::_cleanAndCondenseWhitespace(const std::string& input)
	{
		std::string result = input;

		std::replace(result.begin(), result.end(), '\t', ' ');
		std::replace(result.begin(), result.end(), '\n', ' ');

		std::regex multipleSpaces("\\s+");
		result = std::regex_replace(result, multipleSpaces, " ");

		result = StringUtils::trimWhitespace(result);

		return result;
	}

	void Pipeline::ShaderParser::_parseUniformBufferLayout(spk::OpenGL::UniformBufferObject::Layout& p_layoutToFeed, const std::vector<std::string>& p_words, size_t& p_index)
	{
		spk::OpenGL::UniformBufferObject::Layout newLayout;

		std::wstring name = spk::StringUtils::stringToWString(p_words[p_index++]);

		size_t baseCpuOffset = static_cast<size_t>(std::stol(p_words[p_index++]));
		newLayout._cpu.size = static_cast<size_t>(std::stol(p_words[p_index++]));
		size_t baseGpuOffset = static_cast<size_t>(std::stol(p_words[p_index++]));
		newLayout._gpu.size = static_cast<size_t>(std::stol(p_words[p_index++]));

		size_t nbElements = static_cast<size_t>(std::stol(p_words[p_index++]));
		size_t bytesBetweenElements = static_cast<size_t>(std::stol(p_words[p_index++]));

		if (p_words[p_index] == "{")
		{
			p_index++;
			while (p_words[p_index] != "}")
			{
				_parseUniformBufferLayout(newLayout, p_words, p_index);
			}
			p_index++;
		}
		else if (p_words[p_index] == "{}")
		{
			p_index++;
		}

		std::vector<spk::OpenGL::UniformBufferObject> bufferToInsert;
		for (size_t i = 0; i < nbElements; i++)
		{
			newLayout._cpu.offset = baseCpuOffset + newLayout._cpu.size * i;
			newLayout._gpu.offset = baseGpuOffset + (newLayout._gpu.size + bytesBetweenElements) * i;

			p_layoutToFeed._innerLayouts[name].layouts.push_back(newLayout);
		}
	}

	std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory> Pipeline::ShaderParser::_parseUniformDescriptors(const std::string& p_descriptors)
	{
		std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory> result;

		std::string section = spk::StringUtils::mergeWhitespace(p_descriptors);

		std::vector<std::string> words = spk::StringUtils::splitString(section, " ");

		if (words.size() > 2)
		{
			for (size_t i = 0; i < words.size(); i++)
			{
				spk::OpenGL::UniformBufferObject::Factory newFactory;

				newFactory._typeName = words[i++];
				std::string uniformName = words[i++];
				newFactory._layout._cpu = { 0, static_cast<size_t>(std::stol(words[i++])) };
				newFactory._layout._gpu = { 0, static_cast<size_t>(std::stol(words[i++])) };

				i++;
				while (words[i] != "}")
				{
					_parseUniformBufferLayout(newFactory._layout, words, i);
				}

				result[spk::StringUtils::stringToWString(uniformName)] = newFactory;
			}
		}

		return result;
	}

	void Pipeline::ShaderParser::_assignBindingPointsToConstants()
	{
		for (auto& [key, factory] : _currentConstantFactories)
		{
			std::wstring typeName = StringUtils::stringToWString(factory.typeName());
			if (_constantInfoMap.find(typeName) == _constantInfoMap.end())
			{
				int bindingPoint = static_cast<int>(_constantInfoMap.size());
				size_t size = factory.mainLayout()._cpu.size;

				_constantInfoMap[typeName] = ConstantInformation(factory, bindingPoint, size);

				factory.setBindingPoint(bindingPoint);
			}
			else
			{
				const ConstantInformation& existingInfo = _constantInfoMap[typeName];
				if (existingInfo.size != factory.mainLayout()._cpu.size)
				{
					throw std::runtime_error("Constant [" + factory.typeName() + "] already parsed with a different size");
				}

				factory.setBindingPoint(existingInfo.bindingPoint);
			}
		}
	}

	void Pipeline::ShaderParser::_assignBindingPointsToAttributes()
	{
		size_t offset = 0;
		for (auto& [key, factory] : _attributeFactories)
		{
			factory.setBindingPoint(static_cast<int>(_constantInfoMap.size() + offset));
			offset++;
		}
	}

	void Pipeline::ShaderParser::_applyBindingPoints(const std::string& type, const std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory>& factories)
	{
		for (const auto& [key, factory] : factories)
		{
			std::string typeName = factory.typeName();
			int bindingPoint = factory.bindingPoint();

			std::regex pattern("layout\\(" + type + "\\) uniform " + typeName + "\\b");
			std::string replacement = "layout(std140, binding = " + std::to_string(bindingPoint) + ") uniform " + typeName;

			_vertexShaderCode = std::regex_replace(_vertexShaderCode, pattern, replacement);
			_fragmentShaderCode = std::regex_replace(_fragmentShaderCode, pattern, replacement);
		}
	}

	spk::OpenGL::BufferSet::Factory Pipeline::ShaderParser::_parseLayoutDescriptors(const std::string& p_layoutDescriptors)
	{
		std::unordered_map<std::string, std::tuple<size_t, OpenGL::LayoutBufferObject::Attribute::Type> > _typeToSizeMap = {
			{"bool", {1, OpenGL::LayoutBufferObject::Attribute::Type::Byte}},
			{"int", {1, OpenGL::LayoutBufferObject::Attribute::Type::Int}},
			{"uint", {1, OpenGL::LayoutBufferObject::Attribute::Type::UInt}},
			{"float", {1, OpenGL::LayoutBufferObject::Attribute::Type::Float}},

			{"Vector2", {2, OpenGL::LayoutBufferObject::Attribute::Type::Float}},
			{"Vector2Int", {2, OpenGL::LayoutBufferObject::Attribute::Type::Int}},
			{"Vector2UInt", {2, OpenGL::LayoutBufferObject::Attribute::Type::UInt}},

			{"Vector3", {3, OpenGL::LayoutBufferObject::Attribute::Type::Float}},
			{"Vector3Int", {3, OpenGL::LayoutBufferObject::Attribute::Type::Int}},
			{"Vector3UInt", {3, OpenGL::LayoutBufferObject::Attribute::Type::UInt}},

			{"Vector4", {4, OpenGL::LayoutBufferObject::Attribute::Type::Float}},
			{"Vector4Int", {4, OpenGL::LayoutBufferObject::Attribute::Type::Int}},
			{"Vector4UInt", {4, OpenGL::LayoutBufferObject::Attribute::Type::UInt}},

			{"Color", {4, OpenGL::LayoutBufferObject::Attribute::Type::Float}},
		};
		spk::OpenGL::BufferSet::Factory result;

		std::string section = StringUtils::mergeWhitespace(p_layoutDescriptors);

		std::vector<std::string> words = StringUtils::splitString(section, " ");

		for (size_t i = 0; i < words.size(); i += 3)
		{
			std::string locationString = words[i + 0];
			std::string layoutTypeString = words[i + 1];
			std::string layoutNameString = words[i + 2];

			result.insert(
					std::stol(locationString),
					std::get<0>(_typeToSizeMap[layoutTypeString]),
					std::get<1>(_typeToSizeMap[layoutTypeString])
				);
		}

		return (result);
	}

	std::unordered_map<std::wstring, spk::OpenGL::SamplerObject::Factory> Pipeline::ShaderParser::_parseSamplerDescriptors(const std::string& p_textureDescriptors)
	{
		std::unordered_map<std::wstring, spk::OpenGL::SamplerObject::Factory> result;

		std::string section = StringUtils::mergeWhitespace(p_textureDescriptors);

		std::vector<std::string> words = StringUtils::splitString(section, " ");

		size_t index = 0;

		for (size_t i = 0; i < words.size(); i += 2)
		{
			std::string name = words[i];
			std::string designator = words[i + 1];

			spk::OpenGL::SamplerObject::Factory factory;
			factory.setName(name);
			factory.setDesignator(designator);
			factory.setIndex(index);

			result[StringUtils::stringToWString(name)] = factory;

			index++;
		}

		return result;
	}

	spk::OpenGL::FrameBufferObject::Factory Pipeline::ShaderParser::parseFramebufferDescriptors(const std::string& descriptors)
	{
		spk::OpenGL::FrameBufferObject::Factory factory;
		std::istringstream stream(descriptors);
		std::string line;

		while (std::getline(stream, line))
		{
			line.erase(line.begin(), std::find_if(line.begin(), line.end(),
				[](unsigned char ch) { return !std::isspace(ch); }));
			line.erase(std::find_if(line.rbegin(), line.rend(),
				[](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());

			if (line.empty())
				continue;

			std::istringstream lineStream(line);
			int colorAttachmentIndex;
			std::string typeStr;
			std::string nameStr;

			if (!(lineStream >> colorAttachmentIndex >> typeStr >> nameStr))
			{
				throw std::runtime_error("Failed to parse framebuffer descriptor line: " + line);
			}

			// Map string types to FrameBufferObject::Type
			static const std::unordered_map<std::string, spk::OpenGL::FrameBufferObject::Type> stringToFrameBufferType = {
				{"float", spk::OpenGL::FrameBufferObject::Type::Float},
				{"int", spk::OpenGL::FrameBufferObject::Type::Int},
				{"uint", spk::OpenGL::FrameBufferObject::Type::UInt},

				{"Vector2", spk::OpenGL::FrameBufferObject::Type::Float2},
				{"Vector2Int", spk::OpenGL::FrameBufferObject::Type::Int2},
				{"Vector2UInt", spk::OpenGL::FrameBufferObject::Type::UInt2},

				{"Vector3", spk::OpenGL::FrameBufferObject::Type::Float3},
				{"Vector3Int", spk::OpenGL::FrameBufferObject::Type::Int3},
				{"Vector3UInt", spk::OpenGL::FrameBufferObject::Type::UInt3},

				{"Color", spk::OpenGL::FrameBufferObject::Type::Float4},
				{"Vector4", spk::OpenGL::FrameBufferObject::Type::Float4},
				{"Vector4Int", spk::OpenGL::FrameBufferObject::Type::Int4},
				{"Vector4UInt", spk::OpenGL::FrameBufferObject::Type::UInt4},
			};

			auto typeIt = stringToFrameBufferType.find(typeStr);
			if (typeIt == stringToFrameBufferType.end())
			{
				throw std::runtime_error("Unknown framebuffer type: " + typeStr);
			}

			spk::OpenGL::FrameBufferObject::Type type = typeIt->second;
			std::wstring nameWStr = spk::StringUtils::stringToWString(nameStr);

			factory.addAttachment(nameWStr, colorAttachmentIndex, type);
		}

		return factory;
	}

	Pipeline::ShaderParser::ShaderParser(const std::string& p_fileContent)
	{
		_vertexShaderCode = _getFileSection(p_fileContent, Pipeline::ShaderParser::VERTEX_DELIMITER);
		_fragmentShaderCode = _getFileSection(p_fileContent, Pipeline::ShaderParser::FRAGMENT_DELIMITER);

		std::string layoutDescriptors = _getFileSection(p_fileContent, Pipeline::ShaderParser::LAYOUTS_DELIMITER);
		_objectBufferFactory = _parseLayoutDescriptors(layoutDescriptors);

		std::string frameBufferDescriptors = _getFileSection(p_fileContent, Pipeline::ShaderParser::FRAMEBUFFER_DELIMITER);
		_frameBufferFactory = parseFramebufferDescriptors(frameBufferDescriptors);

		std::string textureDescriptors = _getFileSection(p_fileContent, Pipeline::ShaderParser::TEXTURES_DELIMITER);
		_samplerFactories = _parseSamplerDescriptors(textureDescriptors);

		std::string constantDescriptors = _getFileSection(p_fileContent, Pipeline::ShaderParser::CONSTANTS_DELIMITER);
		_currentConstantFactories = _parseUniformDescriptors(constantDescriptors);
		_assignBindingPointsToConstants();

		std::string attributeDescriptor = _getFileSection(p_fileContent, Pipeline::ShaderParser::ATTRIBUTES_DELIMITER);
		_attributeFactories = _parseUniformDescriptors(attributeDescriptor);
		_assignBindingPointsToAttributes();

		_applyBindingPoints("constants", _currentConstantFactories);
		_applyBindingPoints("attributes", _attributeFactories);
	}

	const std::unordered_map<std::wstring, Pipeline::ShaderParser::ConstantInformation>& Pipeline::ShaderParser::getConstantInfoMap()
	{
		return _constantInfoMap;
	}

	const std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory>& Pipeline::ShaderParser::getCurrentConstantFactories() const
	{
		return _currentConstantFactories;
	}

	const std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory>& Pipeline::ShaderParser::getAttributeFactories() const
	{
		return _attributeFactories;
	}

	const spk::OpenGL::FrameBufferObject::Factory& Pipeline::ShaderParser::getFrameBufferFactory() const
	{
		return _frameBufferFactory;
	}

	const spk::OpenGL::BufferSet::Factory& Pipeline::ShaderParser::getLayoutFactory() const
	{
		return _objectBufferFactory;
	}

	const std::unordered_map<std::wstring, spk::OpenGL::SamplerObject::Factory>& Pipeline::ShaderParser::getSamplerFactories() const
	{
		return _samplerFactories;
	}

	const std::string& Pipeline::ShaderParser::getVertexShaderCode() const
	{
		return _vertexShaderCode;
	}

	const std::string& Pipeline::ShaderParser::getFragmentShaderCode() const
	{
		return _fragmentShaderCode;
	}
}
