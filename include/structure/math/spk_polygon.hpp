#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include <unordered_map>
#include <vector>

namespace spk
{
	struct Polygon
	{
	private:
		static float _polyScale(const std::vector<spk::Vector3> &p_points);
		static bool _approxEq(float p_a, float p_b, float p_tol);
		static bool _inRange(float p_x, float p_a, float p_b, float p_tol);
		static bool _pointOnSegment2D(const spk::Vector2 &p_p, const spk::Vector2 &p_a, const spk::Vector2 &p_b, float p_tol);

		std::unordered_map<spk::Vector3, std::vector<spk::Vector3>> _edges;
		std::vector<spk::Vector3> _points;
		void _insertEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b);
		bool _hasEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b) const;
		void _removeEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b);
		void _computeWire();

	public:
		const std::vector<spk::Vector3> &points() const;
		std::vector<spk::Vector3> rewind() const;

		bool canInsert(const Polygon &p_polygon) const;
		void addTriangle(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c);
		void addQuad(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c, const spk::Vector3 &p_d);
		void addPolygon(const Polygon &p_polygon);
		bool isConvex() const;
		std::vector<Polygon> triangulize() const;
		std::vector<Polygon> split() const;
	};
}
