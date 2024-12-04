#pragma once

#include "structure/math/spk_vector3.hpp"
#include <cmath>

namespace spk
{
	struct Polygon;

	struct Plane
	{
		struct Identifier
		{
			spk::Vector3 normal;
			float dotValue;

			static Identifier from(const spk::Plane &p_p);

			bool operator==(const Plane::Identifier &p_other) const;
		};

		spk::Vector3 origin;
		spk::Vector3 normal;

		Plane() = default;
		Plane(const spk::Vector3 &p_normal, const spk::Vector3 &p_origin);

		bool isSame(const spk::Plane &p_plane) const;

		bool contains(const spk::Polygon &p_polygon) const;
		bool contains(const spk::Vector3 &p_point) const;
		bool operator==(const spk::Plane &p_plane) const;
		bool operator!=(const spk::Plane &p_plane) const;
		bool operator<(const spk::Plane &p_plane) const;
	};
}

template <>
struct std::hash<spk::Plane::Identifier>
{
	size_t operator()(const spk::Plane::Identifier &p_k) const noexcept
	{
		auto to_i = [](float p_v) -> long long { return static_cast<long long>(std::llround(p_v / spk::Constants::pointPrecision)); };

		const long long ix = to_i(p_k.normal.x);
		const long long iy = to_i(p_k.normal.y);
		const long long iz = to_i(p_k.normal.z);
		const long long id = to_i(p_k.dotValue);

		size_t h = 0;
		auto mix = [&](size_t p_x) { h ^= p_x + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2); };
		mix(std::hash<long long>()(ix));
		mix(std::hash<long long>()(iy));
		mix(std::hash<long long>()(iz));
		mix(std::hash<long long>()(id));
		return h;
	}
};

template <>
struct std::hash<spk::Plane>
{
	size_t operator()(const spk::Plane &p_plane) const
	{
		return std::hash<spk::Plane::Identifier>()(spk::Plane::Identifier::from(p_plane));
	}
};