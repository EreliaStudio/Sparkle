#pragma once

#include "chunk.hpp"

#include "buffer_object_collection.hpp"

#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"

#include "structure/graphics/texture/spk_sprite_sheet.hpp"

#include "structure/engine/spk_transform.hpp"

class ChunkRenderer
{
private:
	BufferObjectCollection::Instanciator _bindingPointInstanciator;

	static inline spk::OpenGL::Program* _program = nullptr;
	spk::OpenGL::UniformBufferObject& _cameraUBO;
	spk::OpenGL::UniformBufferObject& _chunkTextureInfo;
	spk::OpenGL::UniformBufferObject& _systemInfo;
	spk::OpenGL::ShaderStorageBufferObject& _nodeMapSSBO;
	spk::OpenGL::BufferSet _bufferSet;
	spk::OpenGL::UniformBufferObject _transformUBO;
	spk::OpenGL::ShaderStorageBufferObject _cellListSSBO;
	spk::OpenGL::ShaderStorageBufferObject _chunkSSBO;
	spk::OpenGL::SamplerObject _spriteSheetSampler;

	void _initProgram();
	void _initBuffers();

public:
	ChunkRenderer();

	void updateSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet);
	void updateChunk(spk::SafePointer<const BakableChunk> p_chunk);
	void updateTransform(spk::Transform& p_transform);
	void updateChunkData(spk::SafePointer<const Chunk> p_chunk);

	void render();
};
