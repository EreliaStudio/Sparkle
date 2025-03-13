#pragma once

#include <GL/glew.h>

#include <GL/gl.h>

#include "spk_texture_object.hpp"
#include "structure/graphics/texture/spk_texture.hpp"
#include "structure/spk_safe_pointer.hpp"
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace spk::OpenGL
{
	class TextureCollection
	{
	private:
		struct TexturePair
		{
			spk::SafePointer<const spk::Texture> cpuPtr;
			std::unique_ptr<spk::OpenGL::TextureObject> gpuPtr;
		};

		static inline std::unordered_map<spk::Texture::ID, TexturePair> _cpuToGpuMap;
		static inline std::unordered_map<GLuint, spk::Texture::ID> _gpuToCpuMap;

	public:
		static spk::SafePointer<spk::OpenGL::TextureObject> textureObject(const spk::SafePointer<const spk::Texture> &p_cpuTexture,
																		  std::unique_ptr<spk::OpenGL::TextureObject> p_gpuTexture = nullptr)
		{
			if (p_cpuTexture == nullptr || p_cpuTexture->_id < 0)
			{
				return nullptr;
			}

			spk::Texture::ID cpuID = p_cpuTexture->_id;
			auto it = _cpuToGpuMap.find(cpuID);

			if (it != _cpuToGpuMap.end())
			{
				return it->second.gpuPtr.get();
			}

			std::unique_ptr<spk::OpenGL::TextureObject> newGPU =
				(p_gpuTexture == nullptr ? std::make_unique<spk::OpenGL::TextureObject>() : std::move(p_gpuTexture));
			GLuint gpuID = newGPU->id();

			TexturePair pair;
			pair.cpuPtr = p_cpuTexture;
			pair.gpuPtr = std::move(newGPU);

			_cpuToGpuMap.insert({cpuID, std::move(pair)});
			_gpuToCpuMap.insert({gpuID, cpuID});

			return _cpuToGpuMap[cpuID].gpuPtr.get();
		}

		static spk::SafePointer<const spk::Texture> texture(const spk::OpenGL::TextureObject *p_gpuTexture)
		{
			if (p_gpuTexture == nullptr)
			{
				return nullptr;
			}

			GLuint gpuID = p_gpuTexture->id();
			auto it = _gpuToCpuMap.find(gpuID);
			if (it == _gpuToCpuMap.end())
			{
				return nullptr;
			}

			spk::Texture::ID cpuID = it->second;
			auto jt = _cpuToGpuMap.find(cpuID);
			if (jt == _cpuToGpuMap.end())
			{
				return nullptr;
			}

			return jt->second.cpuPtr;
		}
	};
}
