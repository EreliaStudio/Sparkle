#include "buffer_object_collection.hpp"

#include "structure/math/spk_matrix.hpp"

BufferObjectCollection::BufferObjectCollection()
{
	spk::OpenGL::ShaderStorageBufferObject& nodeMapSSBO = allocate<spk::OpenGL::ShaderStorageBufferObject>("nodeConstants", "NodeConstants_Type", 0, 4, 4, 20, 4);

	nodeMapSSBO.addElement("nbNodes", 0, 4);

	nodeMapSSBO["nbNodes"] = 0;

	nodeMapSSBO.validate();

	spk::OpenGL::UniformBufferObject& cameraUBO = allocate<spk::OpenGL::UniformBufferObject>("camera", "Camera_Type", 1, 128);
		
	cameraUBO.addElement("view", 0, 64);
	cameraUBO.addElement("projection", 64, 64);

	cameraUBO["view"] = spk::Matrix4x4::identity();
	cameraUBO["projection"] = spk::Matrix4x4::identity();

	cameraUBO.validate();

	spk::OpenGL::UniformBufferObject& chunkTextureInfo = allocate<spk::OpenGL::UniformBufferObject>("chunkTextureInfo", "ChunkTextureInfo_Type", 5, 8);

	chunkTextureInfo.addElement("unit", 0, 8);

	chunkTextureInfo["unit"] = spk::Vector2(1, 1);

	chunkTextureInfo.validate();

	spk::OpenGL::UniformBufferObject& systemInfo = allocate<spk::OpenGL::UniformBufferObject>("systemInfo", "SystemInfo_Type", 6, 4);

	systemInfo.addElement("time", 0, 4);

	systemInfo["time"] = 0;

	systemInfo.validate();
}

bool BufferObjectCollection::isAllocated(const std::string& p_name)
{
	return (_bindedBufferObjects.contains(p_name));
}

spk::OpenGL::UniformBufferObject& BufferObjectCollection::UBO(const std::string& p_uniformName)
{
	if (_bindedBufferObjects.contains(p_uniformName) == false)
	{
		throw std::runtime_error("UBO [" + p_uniformName + "] not found");
	}

	spk::OpenGL::UniformBufferObject* pointer = dynamic_cast<spk::OpenGL::UniformBufferObject*>(_bindedBufferObjects[p_uniformName]);
	
	if (pointer == nullptr)
	{
		throw std::runtime_error("UBO [" + p_uniformName + "] doesn't exist : [" + p_uniformName + "] seem to be a SSBO");
	}

	return (*(pointer));
}

spk::OpenGL::ShaderStorageBufferObject& BufferObjectCollection::SSBO(const std::string& p_uniformName)
{
	if (_bindedBufferObjects.contains(p_uniformName) == false)
	{
		throw std::runtime_error("SSBO [" + p_uniformName + "] not found");
	}

	spk::OpenGL::ShaderStorageBufferObject* pointer = dynamic_cast<spk::OpenGL::ShaderStorageBufferObject*>(_bindedBufferObjects[p_uniformName]);
	
	if (pointer == nullptr)
	{
		throw std::runtime_error("SSBO [" + p_uniformName + "] doesn't exist : [" + p_uniformName + "] seem to be a UBO");
	}

	return (*(pointer));
}