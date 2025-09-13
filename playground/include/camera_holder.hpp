#pragma once

#include <sparkle.hpp>

namespace taag
{
	class CameraHolder : public spk::Entity
	{
	private:
		static inline spk::SafePointer<const CameraHolder> _mainCamera = nullptr;

		spk::SafePointer<spk::CameraComponent> _cameraComponent;

	public:
		CameraHolder(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
			spk::Entity(p_name, p_parent),
			_cameraComponent(addComponent<spk::CameraComponent>(p_name + L"/CameraComponent"))
		{
			transform().place({0.0f, 0.0f, 20.0f});
			transform().lookAtLocal({0, 0, 0});
		}

		const spk::Camera &camera() const
		{
			return (_cameraComponent->camera());
		}

		void setOrthographic(spk::Vector2 p_viewSize)
		{
			_cameraComponent->setOrthographic(p_viewSize.x, p_viewSize.y);
			EventDispatcher::emit(Event::RefreshView);
		}

		void setAsMainCamera() const
		{
			_mainCamera = this;
		}

		static spk::SafePointer<const CameraHolder> mainCamera()
		{
			return (_mainCamera);
		}
	};
}