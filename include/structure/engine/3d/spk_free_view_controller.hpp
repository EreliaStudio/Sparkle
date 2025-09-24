#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

#include "structure/system/device/spk_keyboard.hpp"
#include "structure/system/device/spk_mouse.hpp"

#include "structure/engine/spk_action.hpp"
#include "structure/engine/spk_component.hpp"

namespace spk
{
	class FreeViewController : public spk::Component
	{
	public:
		struct Configuration
		{
			static inline const std::wstring ForwardActionName = L"Forward";
			static inline const std::wstring LeftActionName = L"Left";
			static inline const std::wstring BackwardActionName = L"Backward";
			static inline const std::wstring RightActionName = L"Right";
			static inline const std::wstring UpActionName = L"Up";
			static inline const std::wstring DownActionName = L"Down";

			float moveSpeed = 5.0f;
			float mouseSensitivity = 0.1f;
			bool allowFly = true;
			float maxPitchDegree = 89.0f;
			spk::Mouse::Button lookButton = spk::Mouse::Button::Left;
			std::unordered_map<std::wstring, spk::Keyboard::Key> keymap = {
				{ForwardActionName, spk::Keyboard::Z},
				{LeftActionName, spk::Keyboard::Q},
				{BackwardActionName, spk::Keyboard::S},
				{RightActionName, spk::Keyboard::D},
				{UpActionName, spk::Keyboard::Space},
				{DownActionName, spk::Keyboard::Control}};
		};

	private:
		Configuration _config;

		bool _isMovingCamera;
		spk::Vector2Int _lastMousePosition;

		std::vector<std::unique_ptr<spk::Action>> _actions;

		spk::Vector3 _motionRequested = {0, 0, 0};
		spk::Vector2 _rotationRequested = {0, 0};

		void _applyConfiguration();

	public:
		FreeViewController(const std::wstring &p_name);

		void setConfiguration(const Configuration &p_configuration);
		const Configuration &configuration() const;

		void onUpdateEvent(spk::UpdateEvent &p_event) override;
	};
}