#pragma once

namespace spk
{
	class ActivateObject
	{
	private:
		bool _isActive = false;

	public:
		void switchActivationState()
		{
			_isActive = !_isActive;
		}

		void activate()
		{
			_isActive = true;
		}

		void deactivate()
		{
			_isActive = false;
		}

		bool isActive() const
		{
			return (_isActive);
		}
	};
}