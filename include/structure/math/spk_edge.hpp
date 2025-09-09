#pragma once

#include "structure/math/spk_vector3.hpp"

#include <ostream>

namespace spk
{
	class Edge
	{
	public:
		struct Identifier
		{
			spk::Vector3 a;
			spk::Vector3 b;

			bool operator==(const Identifier &p_other) const;
			bool operator<(const Identifier& p_other) const;
			static Identifier from(const Edge &p_edge);
		};

		struct IdentifierHash
		{
			size_t operator()(const Identifier &p_edge) const noexcept;
		};
	private:
		spk::Vector3 _first;
		spk::Vector3 _second;

		spk::Vector3 _delta;
		spk::Vector3 _direction;

	public:
		Edge(const spk::Vector3 &p_first, const spk::Vector3 &p_second);

		const spk::Vector3 &first() const;
		const spk::Vector3 &second() const;
		const spk::Vector3 &delta() const;
		const spk::Vector3 &direction() const;

		float orientation(const spk::Vector3 &p_point, const spk::Vector3 &p_normal) const;
		bool contains(const spk::Vector3 &p_point, bool p_checkAlignment = true) const;
		float project(const spk::Vector3 &p_point) const;

		bool isInverse(const Edge &p_other) const;
		Edge inverse() const;
		bool isParallel(const Edge &p_other) const;
		bool isColinear(const Edge &p_other) const;
		bool isSame(const Edge &p_other) const;

		bool operator==(const Edge &p_other) const;
		bool operator<(const Edge &p_other) const;

		friend std::ostream &operator<<(std::ostream &p_os, const Edge &p_edge);
		friend std::wostream &operator<<(std::wostream &p_wos, const Edge &p_edge);
	};
}

template <>
struct std::hash<spk::Edge>
{
	size_t operator()(const spk::Edge &p_edge) const noexcept;
};
