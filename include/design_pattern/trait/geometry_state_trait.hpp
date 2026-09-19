#pragma once

#include "math/rect2d.hpp"

namespace spk
{
	class GeometryStateTrait
	{
	private:
		Rect2D _geometry{};

	private:
		virtual void _onGeometryChange(const Rect2D &)
		{
		}

		virtual void _afterGeometryChange(const Rect2D &)
		{
		}

		virtual void _onSetGeometry()
		{
		}

	public:
		virtual ~GeometryStateTrait() = default;

		void setGeometry(const Rect2D &geometry, bool notify = true)
		{
			_geometry = geometry;
			_onSetGeometry();

			if (notify)
			{
				notifyGeometryChange();
			}
		}

		void notifyGeometryChange()
		{
			_onGeometryChange(_geometry);
			_afterGeometryChange(_geometry);
		}

		[[nodiscard]] const Rect2D &geometry() const noexcept
		{
			return _geometry;
		}
	};
}
