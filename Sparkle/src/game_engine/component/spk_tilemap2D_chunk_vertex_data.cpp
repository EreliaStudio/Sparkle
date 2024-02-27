#include "game_engine/component/spk_tilemap2D.hpp"

namespace spk
{
	Tilemap2D::Chunk::VertexData::VertexData() :
		modelVertex(0),
		modelUVs(0),
		modelAnimationOffset(0),
		modelAnimationSize(0),
		modelAnimationDuration(0)
	{

	}

	Tilemap2D::Chunk::VertexData::VertexData( const spk::Vector3& p_modelVertex,
				const spk::Vector2& p_modelUVs,
				const spk::Vector2& p_modelAnimationOffset,
				int p_modelAnimationSize,
				int p_modelAnimationDuration ) :
		modelVertex(p_modelVertex),
		modelUVs(p_modelUVs),
		modelAnimationOffset(p_modelAnimationOffset),
		modelAnimationSize(p_modelAnimationSize),
		modelAnimationDuration(p_modelAnimationDuration)
	{

	}
}