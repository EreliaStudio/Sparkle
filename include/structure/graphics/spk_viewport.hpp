#pragma once

#include "structure/graphics/spk_geometry_2D.hpp"

namespace spk
{
	class Viewport
	{
	private:
		static inline float _maxLayer = 10000;
		
		Geometry2D _geometry;

	public:
		Viewport();
		Viewport(const Geometry2D& p_geometry);

		const Geometry2D& geometry() const;
		void setGeometry(const Geometry2D& p_geometry);

		void apply() const;
	};
}