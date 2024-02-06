#pragma once

#include "math/spk_vector2.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{
	class Viewport
	{
		friend class Application;
		
	public:
		static inline const Viewport* _mainViewport = nullptr;
	private:
		static inline const Viewport* _activeViewport = nullptr;
		static inline spk::Pipeline::Constant* _screenConstant = nullptr;

		spk::Vector2Int _anchor;
		spk::Vector2UInt _size;

	public:
		static const Viewport* activeViewport() { return (_activeViewport); }

		Viewport();

		void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);
		void activate() const;

		const spk::Vector2Int& anchor() const;
		const spk::Vector2UInt& size() const;

		static spk::Vector2 convertScreenToGPUPosition(const spk::Vector2Int& p_screenPosition);
	};
}