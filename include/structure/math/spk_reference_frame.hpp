#pragma once
#include "structure/math/spk_matrix.hpp" // <- header that defines IMatrix / Matrix4x4
#include "structure/math/spk_plane.hpp"
#include "structure/math/spk_polygon.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/system/spk_exception.hpp"
#include <cmath>
#include <cstddef>

namespace spk
{
	class ReferenceFrame
	{
	private:
		spk::Plane _plane;
		spk::Vector3 _origin{0.0f, 0.0f, 0.0f};
		spk::Vector3 _xAxis{1.0f, 0.0f, 0.0f};
		spk::Vector3 _yAxis{0.0f, 1.0f, 0.0f};
		spk::Vector3 _zAxis{0.0f, 0.0f, 1.0f};

		spk::Matrix4x4 _t{};
		spk::Matrix4x4 _tInv{};

		inline void _buildMatrices() noexcept
		{
			_t[0][0] = _xAxis.x;
			_t[0][1] = _xAxis.y;
			_t[0][2] = _xAxis.z;
			_t[0][3] = 0.0f;
			_t[1][0] = _yAxis.x;
			_t[1][1] = _yAxis.y;
			_t[1][2] = _yAxis.z;
			_t[1][3] = 0.0f;
			_t[2][0] = _zAxis.x;
			_t[2][1] = _zAxis.y;
			_t[2][2] = _zAxis.z;
			_t[2][3] = 0.0f;
			_t[3][0] = _origin.x;
			_t[3][1] = _origin.y;
			_t[3][2] = _origin.z;
			_t[3][3] = 1.0f;

			const float tx = -(_xAxis.x * _origin.x + _xAxis.y * _origin.y + _xAxis.z * _origin.z);
			const float ty = -(_yAxis.x * _origin.x + _yAxis.y * _origin.y + _yAxis.z * _origin.z);
			const float tz = -(_zAxis.x * _origin.x + _zAxis.y * _origin.y + _zAxis.z * _origin.z);

			_tInv[0][0] = _xAxis.x;
			_tInv[0][1] = _yAxis.x;
			_tInv[0][2] = _zAxis.x;
			_tInv[0][3] = 0.0f;
			_tInv[1][0] = _xAxis.y;
			_tInv[1][1] = _yAxis.y;
			_tInv[1][2] = _zAxis.y;
			_tInv[1][3] = 0.0f;
			_tInv[2][0] = _xAxis.z;
			_tInv[2][1] = _yAxis.z;
			_tInv[2][2] = _zAxis.z;
			_tInv[2][3] = 0.0f;
			_tInv[3][0] = tx;
			_tInv[3][1] = ty;
			_tInv[3][2] = tz;
			_tInv[3][3] = 1.0f;
		}

		static inline void _planeTangentBasis(const spk::Vector3 &p_n, spk::Vector3 &p_pu, spk::Vector3 &p_pv) noexcept
		{
			const spk::Vector3 z = p_n.normalize();
			const spk::Vector3 helper = (std::fabs(z.x) > 0.9f) ? spk::Vector3{0, 1, 0} : spk::Vector3{1, 0, 0};
			p_pu = (helper - z * helper.dot(z)).normalize();
			p_pv = z.cross(p_pu);
		}

		inline spk::Vector3 _toWorld(const spk::Vector3 &p_local) const noexcept
		{
			return _t * p_local;
		}

		inline spk::Vector3 _fromWorld(const spk::Vector3 &p_world) const noexcept
		{
			return _tInv * p_world;
		}

	public:
		ReferenceFrame() :
			_plane(spk::Plane{spk::Vector3{0.0f, 1.0f, 0.0f}, spk::Vector3{0.0f, 0.0f, 0.0f}})
		{
			_buildMatrices();
		}

		static const ReferenceFrame &standard()
		{
			static const ReferenceFrame s{};
			return s;
		}

		ReferenceFrame(const spk::Plane &p_plane, const spk::Vector2 &p_u2 = {1, 0}, const spk::Vector2 &p_v2 = {0, 1}) :
			_plane(p_plane)
		{
			_origin = p_plane.origin;

			_zAxis = p_plane.normal.normalize();

			spk::Vector3 planeU, planeV;
			_planeTangentBasis(_zAxis, planeU, planeV);

			const spk::Vector3 x = (planeU * p_u2.x + planeV * p_u2.y).normalize();
			const spk::Vector3 y = (planeU * p_v2.x + planeV * p_v2.y).normalize();

			_xAxis = x;
			_yAxis = _zAxis.cross(_xAxis).normalize();
			if (_yAxis.dot(y) < 0.0f)
			{
				_yAxis = _yAxis * -1.0f;
			}

			_buildMatrices();
		}

		ReferenceFrame(const spk::Plane &p_plane, const spk::Vector3 &p_xDir, const spk::Vector3 &p_zDir) :
			_plane(p_plane)
		{
			_origin = p_plane.origin;
			_zAxis = p_zDir.normalize();

			spk::Vector3 x = (p_xDir - _zAxis * _zAxis.dot(p_xDir)).normalize();
			spk::Vector3 y = _zAxis.cross(x).normalize();

			_xAxis = x;
			_yAxis = y;

			_buildMatrices();
		}

		inline const spk::Vector3 &origin() const noexcept
		{
			return _origin;
		}
		inline const spk::Vector3 &xAxis() const noexcept
		{
			return _xAxis;
		}
		inline const spk::Vector3 &yAxis() const noexcept
		{
			return _yAxis;
		}
		inline const spk::Vector3 &zAxis() const noexcept
		{
			return _zAxis;
		}

		inline spk::Vector3 convertTo(const spk::Vector3 &p_v, const ReferenceFrame &p_current = ReferenceFrame::standard()) const noexcept
		{
			return _fromWorld(p_current._toWorld(p_v));
		}

		inline spk::Vector3 convertFrom(const spk::Vector3 &p_vThis, const ReferenceFrame &p_dest = ReferenceFrame::standard()) const noexcept
		{
			return p_dest._fromWorld(_toWorld(p_vThis));
		}

		inline spk::Polygon convertTo(const spk::Polygon &p_poly, const ReferenceFrame &p_currentValueReference = ReferenceFrame::standard()) const
		{
			std::vector<spk::Vector3> pts;
			for (const auto &pt : p_poly.points())
			{
				pts.push_back(convertTo(pt, p_currentValueReference));
			}
			return spk::Polygon::fromLoop(pts);
		}

		inline spk::Polygon convertFrom(
			const spk::Polygon &p_polyInThis, const ReferenceFrame &p_destinationReferenceFrame = ReferenceFrame::standard()) const
		{
			std::vector<spk::Vector3> pts;
			for (const auto &pt : p_polyInThis.points())
			{
				pts.push_back(convertFrom(pt, p_destinationReferenceFrame));
			}
			return spk::Polygon::fromLoop(pts);
		}

		inline void convertToInPlace(spk::Polygon &p_poly, const ReferenceFrame &p_currentValueReference = ReferenceFrame::standard()) const
		{
			p_poly = convertTo(p_poly, p_currentValueReference);
		}

		inline void convertFromInPlace(
			spk::Polygon &p_polyInThis, const ReferenceFrame &p_destinationReferenceFrame = ReferenceFrame::standard()) const
		{
			p_polyInThis = convertFrom(p_polyInThis, p_destinationReferenceFrame);
		}
	};
}
