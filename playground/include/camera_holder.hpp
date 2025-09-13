#pragma once

#include <sparkle.hpp>

#include "event.hpp"

namespace taag
{
	class CameraHolder : public spk::Entity
	{
	private:
		static inline spk::SafePointer<const CameraHolder> _mainCamera = nullptr;

		EventDispatcher::Instanciator _instanciator;
		spk::SafePointer<spk::CameraComponent> _cameraComponent;

	public:
		CameraHolder(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent);

		const spk::Camera &camera() const;

		void setOrthographic(spk::Vector2 p_viewSize);

		void setAsMainCamera() const;
		static spk::SafePointer<const CameraHolder> mainCamera();
	};
}