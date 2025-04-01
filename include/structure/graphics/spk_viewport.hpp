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
		static spk::SafePointer<const Viewport> _appliedViewport;

		bool _invertXAxis = false;
		bool _invertYAxis = false;
		bool _invertZAxis = false;

		Geometry2D _geometry;
		spk::Vector2UInt _windowSize;

	public:
		Viewport();
		Viewport(const Geometry2D &p_geometry);

		const Geometry2D &geometry() const;
		void setGeometry(const Geometry2D &p_geometry);

		void setWindowSize(const spk::Vector2UInt &p_windowSize);
		const spk::Vector2UInt &windowSize() const;

		void invertXAxis();
		void invertYAxis();
		void invertZAxis();

		void apply() const;
		static spk::SafePointer<const Viewport> activeViewport();
		static spk::Matrix4x4 matrix();
		static spk::Vector2 convertScreenToOpenGL(const spk::Vector2Int p_screenPosition);
		static spk::Vector2 convertScreenToOpenGL(int p_screenPositionX, int p_screenPositionY);
		static float convertLayerToOpenGL(const float &p_layer);
		static spk::Vector3 convertScreenToOpenGL(const spk::Vector2Int p_screenPosition, const float &p_layer);
		static spk::Vector3 convertScreenToOpenGL(int p_screenPositionX, int p_screenPositionY, const float &p_layer);
	};
}