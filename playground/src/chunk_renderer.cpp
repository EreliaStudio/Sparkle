#include "chunk_renderer.hpp"

#include "utils/spk_file_utils.hpp"

void ChunkRenderer::_initProgram()
{
	if (_program != nullptr)
	{
		return;
	}

	std::string vertexShaderSrc = spk::FileUtils::readFileAsString("playground/resources/shader/chunkShader.vert");

	std::string fragmentShaderSrc = spk::FileUtils::readFileAsString("playground/resources/shader/chunkShader.frag");

	_program = new spk::OpenGL::Program(vertexShaderSrc, fragmentShaderSrc);
}

void ChunkRenderer::_initBuffers()
{
	_bufferSet = spk::OpenGL::BufferSet({
		{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2} // inCellIndex
	});

	_bufferSet.clear();

	_bufferSet.layout() << spk::Vector2(0, 0);
	_bufferSet.layout() << spk::Vector2(0, 1);
	_bufferSet.layout() << spk::Vector2(1, 0);
	_bufferSet.layout() << spk::Vector2(1, 1);

	_bufferSet.indexes() << 0 << 1 << 2 << 2 << 1 << 3;

	_bufferSet.validate();

	_transformUBO = spk::OpenGL::UniformBufferObject("Transform_Type", 0, 64);
	_transformUBO.addElement("model", 0, 64);

	_transformUBO["model"] = spk::Matrix4x4::identity();

	_transformUBO.validate();

	_cellListSSBO = spk::OpenGL::ShaderStorageBufferObject("CellList_Type", 2, 0, 0, 12, 4);

	_chunkSSBO = spk::OpenGL::ShaderStorageBufferObject("Chunk_Type", 3, 5120, 0, 0, 0);

	_chunkSSBO.addElement("content", 0, 4, 1280, 0);

	_chunkSSBO["content"] = std::vector<int>(16 * 16 * 5, -1);

	_chunkSSBO.validate();

	_spriteSheetSampler = spk::OpenGL::SamplerObject("spriteSheet", spk::OpenGL::SamplerObject::Type::Texture2D, 0);
}

ChunkRenderer::ChunkRenderer() :
	_cameraUBO(BufferObjectCollection::instance()->UBO("camera")),
	_nodeMapSSBO(BufferObjectCollection::instance()->SSBO("nodeConstants")),
	_chunkTextureInfo(BufferObjectCollection::instance()->UBO("chunkTextureInfo")),
	_systemInfo(BufferObjectCollection::instance()->UBO("systemInfo"))
{
	_initProgram();
	_initBuffers();
}

void ChunkRenderer::updateSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet)
{
	_spriteSheetSampler.bind(p_spriteSheet);
	_chunkTextureInfo["unit"] = p_spriteSheet->unit();
	_chunkTextureInfo.validate();
}	

void ChunkRenderer::updateChunk(spk::SafePointer<const BakableChunk> p_chunk)
{
	_chunkSSBO["content"] = *(dynamic_cast<const Chunk*>(p_chunk.get()));

	_chunkSSBO.validate();
}

void ChunkRenderer::updateTransform(spk::Transform& p_transform)
{
	_transformUBO["model"] = p_transform.model();

	_transformUBO.validate();
}

void ChunkRenderer::updateChunkData(spk::SafePointer<const Chunk> p_chunk)
{
	std::vector<spk::Vector3Int> cellList;

	for (size_t i = 0; i < Chunk::Size; i++)
	{
		for (size_t j = 0; j < Chunk::Size; j++)
		{
			for (size_t k = 0; k < Chunk::Layer; k++)
			{
				spk::Vector3Int tmp = spk::Vector3Int(i, j, k);

				if (p_chunk->content(tmp) != -1)
				{
					cellList.push_back(tmp);
				}
			}
		}
	}

	_cellListSSBO.resizeDynamicArray(cellList.size());

	_cellListSSBO.dynamicArray() = cellList;

	_cellListSSBO.validate();
}

void ChunkRenderer::render()
{
	if (_spriteSheetSampler.texture() == nullptr)
	{
		throw std::runtime_error("Chunk sprite sheet not set");
	}

	_program->activate();

	_systemInfo.activate();
	_spriteSheetSampler.activate();
	_chunkSSBO.activate();
	_chunkTextureInfo.activate();
	_nodeMapSSBO.activate();
	_cellListSSBO.activate();
	_cameraUBO.activate();
	_transformUBO.activate();
	_bufferSet.activate();
	
	_program->render(_bufferSet.indexes().nbIndexes(), _cellListSSBO.dynamicArray().nbElement());

	_bufferSet.deactivate();
	_transformUBO.deactivate();
	_cameraUBO.deactivate();
	_cellListSSBO.deactivate();
	_nodeMapSSBO.deactivate();
	_chunkTextureInfo.deactivate();
	_chunkSSBO.deactivate();
	_spriteSheetSampler.deactivate();
	_systemInfo.deactivate();

	_program->deactivate();
}