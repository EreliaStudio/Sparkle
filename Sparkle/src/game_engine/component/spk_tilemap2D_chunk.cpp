#include "game_engine/component/spk_tilemap2D.hpp"

namespace spk
{
	const Tilemap2D::Chunk* Tilemap2D::Chunk::_getChunkNeighbour(const spk::Vector2Int& p_chunkOffset)
	{
		_searchChunkNeighbour(p_chunkOffset);
		return (_neightbours[p_chunkOffset.x + 1][p_chunkOffset.y + 1]);
	}

	void Tilemap2D::Chunk::_searchChunkNeighbour(const spk::Vector2Int& p_chunkOffset)
	{
		spk::Vector2Int relativeChunkPosition = p_chunkOffset + 1;
		if (_neightbours[relativeChunkPosition.x][relativeChunkPosition.y] == nullptr)
		{
			const spk::GameObject* chunkobject = _tilemapCreator->chunkObject(position() + p_chunkOffset);
			_neightbours[relativeChunkPosition.x][relativeChunkPosition.y] = (chunkobject == nullptr ? nullptr : chunkobject->getComponent<Chunk>());
		}
	}

	const Tilemap2D::Chunk* Tilemap2D::Chunk::_getNeightbourChunk(spk::Vector3Int& p_relativePosition)
	{
		spk::Vector2Int chunkOffset = 0;

		if (p_relativePosition.x < 0)
		{
			chunkOffset.x--;
			p_relativePosition.x += SizeX;
		}
		else if (p_relativePosition.x >= SizeX)
		{
			chunkOffset.x++;
			p_relativePosition.x -= SizeX;
		}

		if (p_relativePosition.y < 0)
		{
			chunkOffset.y--;
			p_relativePosition.y += SizeY;
		}
		else if (p_relativePosition.y >= SizeY)
		{
			chunkOffset.y++;
			p_relativePosition.y -= SizeY;
		}

		return (_getChunkNeighbour(chunkOffset));
	}
	
	void Tilemap2D::Chunk::_onObjectRendering()
	{
		_textureObject.attach(_spriteSheet);
		_renderingObject.render();
	}

	void Tilemap2D::Chunk::_updateTransform()
	{
		_renderingObjectSelfTransformTranslationElement = owner()->globalPosition();
		_renderingObjectSelfTransformScaleElement = owner()->globalScale();
		_renderingObjectSelfTransformRotationElement = owner()->globalRotation();
		_renderingObjectSelfAttribute.update();
	}

	Tilemap2D::Chunk::Chunk(const Tilemap2D* p_tilemapCreator, const spk::Vector2Int& p_chunkPosition) :
		IChunk(p_chunkPosition),
		_tilemapCreator(p_tilemapCreator),
		_renderingObject(_renderingPipeline.createObject()),
		_textureObject(_renderingPipeline.texture("textureID")),
		_renderingObjectSelfAttribute(_renderingObject.attribute("self")),
		_renderingObjectSelfTransformTranslationElement(_renderingObjectSelfAttribute["transform"]["translation"]),
		_transformTranslationContract(owner()->transform().translation.subscribe([&](){_updateTransform();})),
		_renderingObjectSelfTransformScaleElement(_renderingObjectSelfAttribute["transform"]["scale"]),
		_transformScaleContract(owner()->transform().scale.subscribe([&](){_updateTransform();})),
		_renderingObjectSelfTransformRotationElement(_renderingObjectSelfAttribute["transform"]["rotation"]),
		_transformRotationContract(owner()->transform().rotation.subscribe([&](){_updateTransform();}))
	{
		_updateTransform();

		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				_searchChunkNeighbour(spk::Vector2Int(x, y));
			}
		}
		
		bake();
	}

	void Tilemap2D::Chunk::setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
	{
		_spriteSheet = p_spriteSheet;
	}
	
	const spk::SpriteSheet* Tilemap2D::Chunk::spriteSheet()
	{
		return (_spriteSheet);
	}

	const spk::Mesh& Tilemap2D::Chunk::collisionMesh() const 
	{
		return (_collisionMesh);
	}

	bool Tilemap2D::Chunk::isObstacle(const spk::Vector2Int& p_position)
	{
		for (size_t k = 0; k < SizeZ; k++)
		{
			Chunk::NodeIndexType index = content(p_position, k);
			if (_tilemapCreator->containsNode(index) == true)
			{
				const Node& tmpNode = _tilemapCreator->node(index);

				if ((tmpNode.flags & Node::OBSTACLE) == Node::OBSTACLE)
					return (true);
			}
		}
		return (false);
	}
}