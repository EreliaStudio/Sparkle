#include "game_engine/component/spk_tilemap2D.hpp"

namespace spk
{
	bool Tilemap2D::Chunk::_isValidSquare(bool (&p_used)[SizeX][SizeY], const spk::Vector2Int& p_start, const spk::Vector2Int& p_end)
	{
		if (p_end.x >= SizeX || p_end.y >= SizeY)
			return (false);

		for (int x = p_start.x; x <= p_end.x; x++)
		{
			for (int y = p_start.y; y <= p_end.y; y++)
			{
				if (p_used[x][y] == true)
					return (false);
			}
		}

		return (true);
	}

	spk::Vector2Int Tilemap2D::Chunk::_searchEndPosition(bool (&p_used)[SizeX][SizeY], const spk::Vector2Int& p_start)
	{
		spk::Vector2Int result = p_start;

		while (_isValidSquare(p_used, p_start, result + spk::Vector2Int(1, 0)) == true)result += spk::Vector2Int(1, 0);
		while (_isValidSquare(p_used, p_start, result + spk::Vector2Int(0, 1)) == true)result += spk::Vector2Int(0, 1);

		return result;
	}

	void Tilemap2D::Chunk::_createCollisionMesh()
	{
		unsigned int indexes[6] = {
			0, 1, 2, 2, 1, 3
		};
		bool used[SizeX][SizeY];

		for (int y = 0; y < SizeY; y++)
		{
			for (int x = 0; x < SizeX; x++)
			{
				used[x][y] = isObstacle(spk::Vector2Int(x, y));
			}
		}

		for (int y = 0; y < SizeY; y++)
		{
			for (int x = 0; x < SizeX; x++)
			{
				if (used[x][y] == false)
				{
					spk::Vector2Int start = spk::Vector2Int(x, y);
					spk::Vector2Int end = _searchEndPosition(used, start);

					for (int i = start.x; i <= end.x; i++)
					{
						for (int j = start.y; j <= end.y; j++)
						{
							used[i][j] = true;
						}
					}

					unsigned int baseIndexPoint = _collisionMesh.points().size();
					unsigned int baseIndexNormal = _collisionMesh.normals().size();

					_collisionMesh.addPoint(spk::Vector3(start.x, start.y, 0));
					_collisionMesh.addPoint(spk::Vector3(start.x, end.y, 0));
					_collisionMesh.addPoint(spk::Vector3(end.x, start.y, 0));
					_collisionMesh.addPoint(spk::Vector3(end.x, end.y, 0));

					_collisionMesh.addNormal(spk::Vector3(0, 0, 1));

					for (size_t i = 0; i < 6; i++)
						_collisionMesh.addIndex(baseIndexPoint + indexes[i], -1, baseIndexNormal);
				}
			}
		}
	}
}