#pragma once

#include <unordered_map>

#include "structure/design_pattern/spk_singleton.hpp"

#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"
#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"

class BufferObjectCollection : public spk::Singleton<BufferObjectCollection>
{
friend class spk::Singleton<BufferObjectCollection>;
private:
	std::unordered_map<std::string, spk::OpenGL::BindedBufferObject*> _bindedBufferObjects;

	BufferObjectCollection();

public:
	template<typename TBindedObject, typename... TArgs>
	TBindedObject& allocate(const std::string& p_name, TArgs... p_args)
	{
		if (_bindedBufferObjects.contains(p_name))
		{
			throw std::runtime_error("Binding point already allocated");
		}
		TBindedObject* object = new TBindedObject(p_args...);
		_bindedBufferObjects[p_name] = object;
		return (*object); 
	}

	bool isAllocated(const std::string& p_name);

	spk::OpenGL::UniformBufferObject& UBO(const std::string& p_uniformName);
	spk::OpenGL::ShaderStorageBufferObject& SSBO(const std::string& p_uniformName);
};