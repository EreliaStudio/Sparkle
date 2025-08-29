#pragma once

#include "structure/math/spk_vector2.hpp"

#include <ostream>

namespace spk
{
	class Edge2D
	{
	public:
		struct Identifier
		{
			spk::Vector2 a;
			spk::Vector2 b;

			bool operator==(const Identifier &p_other) const
			{
				return (a == p_other.a && b == p_other.b);
			}

			bool operator<(const Identifier &p_other) const
			{
				if ((a != p_other.a) == true)
				{
					return (a < p_other.a);
				}
				return (b < p_other.b);
			}

			static Identifier from(const Edge2D &p_edge)
			{
				return {p_edge.first(), p_edge.second()};
			}
		};

		struct IdentifierHash
		{
			size_t operator()(const Identifier &p_id) const noexcept
			{
				size_t h1 = std::hash<spk::Vector2>{}(p_id.a);
				size_t h2 = std::hash<spk::Vector2>{}(p_id.b);
				size_t seed = h1;
				seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				return seed;
			}
		};

	private:
		spk::Vector2 _first;
		spk::Vector2 _second;

	public:
		Edge2D(const spk::Vector2 &p_first, const spk::Vector2 &p_second) :
			_first(p_first),
			_second(p_second)
		{
		}

		const spk::Vector2 &first() const
		{
			return _first;
		}

		const spk::Vector2 &second() const
		{
			return _second;
		}

		bool operator==(const Edge2D &p_other) const
		{
			return (_first == p_other._first && _second == p_other._second);
		}

		bool operator<(const Edge2D &p_other) const
		{
			if ((_first != p_other._first) == true)
			{
				return (_first < p_other._first);
			}
			return (_second < p_other._second);
		}

		friend std::ostream &operator<<(std::ostream &p_os, const Edge2D &p_edge)
		{
			p_os << "(" << p_edge._first << ", " << p_edge._second << ")";
			return p_os;
		}

		friend std::wostream &operator<<(std::wostream &p_wos, const Edge2D &p_edge)
		{
			p_wos << L"(" << p_edge._first << L", " << p_edge._second << L")";
			return p_wos;
		}
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
			seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}
