#pragma once

#include "structure/math/spk_vector3.hpp"

#include "structure/system/device/spk_keyboard.hpp"

#include "structure/engine/spk_action.hpp"
#include "structure/engine/spk_component.hpp"

namespace spk
{
	class TopDownViewController : public spk::Component
	{
	public:
		struct Configuration
		{
			static inline const std::wstring ForwardActionName = L"Forward";
			static inline const std::wstring LeftActionName = L"Left";
			static inline const std::wstring BackwardActionName = L"Backward";
			static inline const std::wstring RightActionName = L"Right";
			static inline const std::wstring RotateLeftActionName = L"RotateLeft";
			static inline const std::wstring RotateRightActionName = L"RotateRight";

			float moveSpeed = 5.0f;
			float rotateSpeed = 90.0f;

			std::unordered_map<std::wstring, spk::Keyboard::Key> keymap = {{ForwardActionName, spk::Keyboard::Z},
																		   {LeftActionName, spk::Keyboard::Q},
																		   {BackwardActionName, spk::Keyboard::S},
																		   {RightActionName, spk::Keyboard::D},
																		   {RotateLeftActionName, spk::Keyboard::A},
																		   {RotateRightActionName, spk::Keyboard::E}};
		};

	private:
		Configuration _config;
		std::vector<std::unique_ptr<spk::Action>> _actions;
		spk::Vector3 _motionRequested = {0, 0, 0};
		float _rotationRequested = 0.0f;
		spk::Vector3 _focusPoint = {0, 0, 0};

		void _applyConfiguration();

	public:
		TopDownViewController(const std::wstring &p_name);

		void setConfiguration(const Configuration &p_configuration);
		const Configuration &configuration() const;

		void setFocus(const spk::Vector3 &p_focus);
		const spk::Vector3 &focus() const;

		void onUpdateEvent(spk::UpdateEvent &p_event) override;
	};
}
