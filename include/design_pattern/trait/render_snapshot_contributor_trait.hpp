#pragma once

#include <exception>

#include "rendering/render_snapshot.hpp"

namespace spk
{
	class RenderSnapshotContributorTrait
	{
	protected:
		[[nodiscard]] virtual bool _canBuildRenderSnapshot() const
		{
			return true;
		}

		virtual void _beforeBuildRenderSnapshot(RenderSnapshot::Builder &)
		{
		}

		virtual void _buildRenderSnapshot(RenderSnapshot::Builder &)
		{
		}

		virtual void _afterBuildRenderSnapshot(RenderSnapshot::Builder &)
		{
		}

		virtual void _onBuildRenderSnapshotException(std::exception_ptr exception)
		{
			std::rethrow_exception(exception);
		}

	public:
		virtual ~RenderSnapshotContributorTrait() = default;

		virtual void buildRenderSnapshot(RenderSnapshot::Builder &builder) final
		{
			if (_canBuildRenderSnapshot() == false)
			{
				return;
			}

			try
			{
				_beforeBuildRenderSnapshot(builder);
				_buildRenderSnapshot(builder);
				_afterBuildRenderSnapshot(builder);
			} catch (...)
			{
				_onBuildRenderSnapshotException(std::current_exception());
			}
		}
	};
}
