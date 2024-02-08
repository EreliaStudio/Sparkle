#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{
	size_t Pipeline::Object::Storage::nbTriangles() const
	{
		return (_indexes.size() / sizeof(unsigned int));
	}

	Pipeline::Object::Storage::Storage(const Storage::Layout& p_storageLayout) :
		_stride(p_storageLayout.stride),
		_vertices(OpenGL::VertexBufferObject::Mode::Vertices),
		_indexes(OpenGL::VertexBufferObject::Mode::Indexes)
	{
		activate();

		for (const auto& element : p_storageLayout.elements)
		{
			glEnableVertexAttribArray(element.location);

			switch (element.type)
			{
			case OpenGL::Type::Byte:
			case OpenGL::Type::UnsignedByte:
			case OpenGL::Type::Short:
			case OpenGL::Type::UnsignedShort:
			case OpenGL::Type::Integer: 
			case OpenGL::Type::UnsignedInteger:
			{
				glVertexAttribIPointer(
							element.location,
							element.nbElement,
							static_cast<GLenum>(element.type),
							p_storageLayout.stride,
							(void *)(element.offset)
						);
						break;
			}
			default :
			{
				glVertexAttribPointer(
							element.location,
							element.nbElement,
							static_cast<GLenum>(element.type),
							GL_FALSE,
							p_storageLayout.stride,
							(void *)(element.offset)
						);
						break;
			}
			}			
		}

		deactivate();
	}

	void Pipeline::Object::Storage::setIndexes(const std::vector<size_t> p_indexesData)
	{
		_indexes.push(p_indexesData.data(), p_indexesData.size() * sizeof(size_t));
	}

	void Pipeline::Object::Storage::activate()
	{
		_vertices.activate();
		_indexes.activate();
	}

	void Pipeline::Object::Storage::deactivate()
	{
		_vertices.deactivate();
		_indexes.deactivate();
	}

	Pipeline::Object::Storage::Layout Pipeline::_parseStorageLayout(const std::vector<OpenGL::ShaderInstruction>& p_shaderInstruction)
	{
		Object::Storage::Layout result;
		
		result.stride = 0;

		for (const auto& instruction : p_shaderInstruction)
		{
			if (instruction.type == OpenGL::ShaderInstruction::Type::Input)
			{
				size_t location = std::stoi(instruction.informations[OpenGL::ShaderInstruction::LocationKey].as<std::string>());
				auto structure = _structures[instruction.informations[OpenGL::ShaderInstruction::TypeKey].as<std::string>()];

				Object::Storage::Layout::Element newElement;

				newElement.location = std::stoi(instruction.informations[OpenGL::ShaderInstruction::LocationKey].as<std::string>());
				newElement.offset = result.stride;
				newElement.nbElement = structure.nbElement;
				newElement.type = structure.type;

				result.stride += structure.sizeWithoutPadding;
				result.elements.push_back(newElement);
			}
		}
		
		return (result);
	}
}