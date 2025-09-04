#pragma once

#include "structure/math/spk_vector2.hpp"

#include <ostream>
#include <algorithm>
#include <cmath>

namespace spk
{
	class Edge2D
	{
	public:
		struct Identifier
		{
			Vector2 a;
			Vector2 b;

			bool operator==(const Identifier &p_other) const;
			bool operator<(const Identifier &p_other) const;

			static Identifier from(const Edge2D &p_edge);
		};

		struct IdentifierHash
		{
			size_t operator()(const Identifier &p_id) const noexcept
			{
				size_t h1 = std::hash<Vector2>{}(p_id.a);
				size_t h2 = std::hash<Vector2>{}(p_id.b);
				size_t seed = h1;
				seed ^= h2 + 0x9e3779b9u + (seed << 6) + (seed >> 2);
				return seed;
			}
		};

	private:
		Vector2 _first;
		Vector2 _second;

		Vector2 _delta;
		Vector2 _direction;

	public:
		Edge2D(const Vector2 &p_first, const Vector2 &p_second);

		const Vector2 &first() const;
		const Vector2 &second() const;
		const Vector2 &delta() const;
		const Vector2 &direction() const;

		float orientation(const Vector2 &p_point) const;

		bool contains(const Vector2 &p_point, bool p_checkAlignment = true) const;

		float project(const Vector2 &p_point) const;

		bool isInverse(const Edge2D &p_other) const;
		Edge2D inverse() const;

		bool isParallel(const Edge2D &p_other) const;
		bool isColinear(const Edge2D &p_other) const;
		bool isSame(const Edge2D &p_other) const;

		bool operator==(const Edge2D &p_other) const;
		bool operator!=(const Edge2D &p_other) const;
		bool operator<(const Edge2D &p_other) const;

		friend std::ostream &operator<<(std::ostream &p_os, const Edge2D &p_edge);
		friend std::wostream &operator<<(std::wostream &p_wos, const Edge2D &p_edge);
	};
}

namespace std
{
	template <>
	struct hash<spk::Edge2D>
	{
		size_t operator()(const spk::Edge2D &p_edge) const noexcept
		{
			size_t h1 = std::hash<spk::Vector2>{}(p_edge.first());
			size_t h2 = std::hash<spk::Vector2>{}(p_edge.second());
			size_t seed = h1;
			seed ^= h2 + 0x9e3779b9u + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}
