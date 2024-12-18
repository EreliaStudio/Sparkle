#pragma once

#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/math/spk_matrix.hpp"

namespace spk
{
	class Viewport
	{
	private:
		static inline float _maxLayer = 10000;
		static inline Matrix4x4 _matrix = spk::Matrix4x4();
		
		Geometry2D _geometry;

	public:
		Viewport();
		Viewport(const Geometry2D& p_geometry);

		const Geometry2D& geometry() const;
		void setGeometry(const Geometry2D& p_geometry);

		void apply() const;
		static spk::Vector3 convertScreenToOpenGL(const spk::Vector2Int p_screenPosition, const float& p_layer = 0);
	};
}