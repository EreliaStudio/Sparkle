#pragma once

#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/math/spk_matrix.hpp"
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	class Viewport
	{
	private:
		static float _maxLayer;
		static Matrix4x4 _matrix;
		static spk::Vector2 _convertionOffset;
		static spk::Vector2UInt _windowSize;
		static spk::SafePointer<const Viewport> _appliedViewport;
		
		Geometry2D _geometry;

	public:
		Viewport();
		Viewport(const Geometry2D& p_geometry);

		const Geometry2D& geometry() const;
		void setGeometry(const Geometry2D& p_geometry);

		void apply() const;
		void setAsRootViewport() const;
		static spk::SafePointer<const Viewport> activeViewport();
		static spk::Matrix4x4 matrix();
		static spk::Vector2 convertScreenToOpenGL(const spk::Vector2Int p_screenPosition);
		static spk::Vector2 convertScreenToOpenGL(int p_screenPositionX, int p_screenPositionY);
		static float convertLayerToOpenGL(const float& p_layer);
		static spk::Vector3 convertScreenToOpenGL(const spk::Vector2Int p_screenPosition, const float& p_layer);
		static spk::Vector3 convertScreenToOpenGL(int p_screenPositionX, int p_screenPositionY, const float& p_layer);
	};
}