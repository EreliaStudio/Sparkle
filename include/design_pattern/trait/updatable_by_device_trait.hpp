#pragma once

#include <exception>

namespace spk
{
	struct DeviceContext;
	struct UpdateContext;

	class UpdatableByDeviceTrait
	{
	protected:
		[[nodiscard]] virtual bool _canUpdateByDevice() const
		{
			return true;
		}

		virtual void _beforeUpdate(UpdateContext &, DeviceContext &)
		{
		}

		virtual void _updateState(UpdateContext &, DeviceContext &)
		{
		}

		virtual void _afterUpdate(UpdateContext &, DeviceContext &)
		{
		}

		virtual void _onUpdateByDeviceException(std::exception_ptr exception)
		{
			std::rethrow_exception(exception);
		}

	public:
		virtual ~UpdatableByDeviceTrait() = default;

		virtual void updateState(UpdateContext &context, DeviceContext &deviceContext) final
		{
			if (_canUpdateByDevice() == false)
			{
				return;
			}

			try
			{
				_beforeUpdate(context, deviceContext);
				_updateState(context, deviceContext);
				_afterUpdate(context, deviceContext);
			} catch (...)
			{
				_onUpdateByDeviceException(std::current_exception());
			}
		}
	};
}
