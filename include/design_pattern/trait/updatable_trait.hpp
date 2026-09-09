#pragma once

#include <exception>

namespace spk
{
	struct UpdateContext;

	class UpdatableTrait
	{
	protected:
		[[nodiscard]] virtual bool _canUpdate() const
		{
			return true;
		}

		virtual void _beforeUpdate(UpdateContext &)
		{
		}

		virtual void _updateState(UpdateContext &)
		{
		}

		virtual void _afterUpdate(UpdateContext &)
		{
		}

		virtual void _onUpdateException(std::exception_ptr exception)
		{
			std::rethrow_exception(exception);
		}

	public:
		virtual ~UpdatableTrait() = default;

		virtual void updateState(UpdateContext &context) final
		{
			if (_canUpdate() == false)
			{
				return;
			}

			try
			{
				_beforeUpdate(context);
				_updateState(context);
				_afterUpdate(context);
			} catch (...)
			{
				_onUpdateException(std::current_exception());
			}
		}
	};
}
