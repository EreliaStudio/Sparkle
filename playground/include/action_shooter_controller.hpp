#include <algorithm>
#include <cmath>
#include <iostream>
#include <list>
#include <sparkle.hpp>
#include <unordered_map>

namespace taag
{
	class ActionShooterController : public spk::Component
	{
	public:
		struct Configuration
		{
			static inline const std::wstring ForwardActionName = L"Forward";
			static inline const std::wstring LeftActionName = L"Left";
			static inline const std::wstring BackwardActionName = L"Backward";
			static inline const std::wstring RightActionName = L"Right";

			float moveSpeed = 5.0f;
			std::unordered_map<std::wstring, spk::Keyboard::Key> keymap = {
				{ForwardActionName, spk::Keyboard::Z},
				{LeftActionName, spk::Keyboard::Q},
				{BackwardActionName, spk::Keyboard::S},
				{RightActionName, spk::Keyboard::D}};
		};

	private:
		Configuration _config;
		std::vector<std::unique_ptr<spk::Action>> _actions;
		spk::Vector3 _motionRequested = {0, 0, 0};

		spk::SafePointer<spk::Entity> _player;
		spk::SafePointer<spk::Entity> _camera;
		float _lastMouseAngleRad = 0.0f;

		void _applyConfiguration();

	public:
		ActionShooterController(const std::wstring &p_name);

		void setConfiguration(const Configuration &p_configuration);

		const Configuration &configuration() const
		{
			return (_config);
		}

		void awake() override;

		void onUpdateEvent(spk::UpdateEvent &p_event) override;

		void onMouseEvent(spk::MouseEvent &p_event) override;
	};
}
