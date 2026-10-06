#pragma once

#include "operation_guard.hpp"
#include "update.hpp"
#include <cstddef>
#include <exception.hpp>
#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace spk::Network
{
	// Tracking history belongs to the receiver, not to removable replicas.
	// All operations run on one owner thread.
	template <typename State>
	class Receiver final
	{
		struct Tracking
		{
			std::uint64_t identity = 0, revision = 0;
			bool active = false;
		};
		SessionID _session;
		std::size_t _maximumTracked;
		std::map<ObjectID, Tracking> _tracking;
		bool _receiving = false;

		static void _validate(const Update<State> &update)
		{
			if (update.object.isNull())
			{
				throw spk::Exception("Null network identity");
			}
			if (update.tracking == 0 || update.revision == 0 || update.edit > Edit::Destroy ||
				(update.edit == Edit::Set) != static_cast<bool>(update.state))
			{
				throw spk::Exception("Invalid replication update");
			}
		}

		[[nodiscard]] static bool _obsolete(const Tracking &tracked, const Update<State> &update)
		{
			return update.tracking < tracked.identity ||
				   (update.tracking == tracked.identity &&
					(!tracked.active || (update.edit == Edit::Set && update.revision <= tracked.revision)));
		}

		template <typename Consumer>
		void _apply(const Update<State> &update, Consumer &&consume)
		{
			auto [found, inserted] = _tracking.try_emplace(update.object);
			try
			{
				std::invoke(std::forward<Consumer>(consume), update);
			} catch (...)
			{
				if (inserted)
				{
					_tracking.erase(found);
				}
				throw;
			}
			found->second = {update.tracking, update.revision, update.edit == Edit::Set};
		}

	public:
		explicit Receiver(std::size_t maximumTracked = 65536) :
			_maximumTracked(maximumTracked)
		{
			if (maximumTracked == 0)
			{
				throw spk::Exception("Invalid receiver capacity");
			}
		}

		void reset(SessionID session)
		{
			OperationGuard guard(_receiving);
			if (session.isNull())
			{
				throw spk::Exception("Null network identity");
			}
			if (_session != session)
			{
				_session = session;
				_tracking.clear();
			}
		}

		void close()
		{
			OperationGuard guard(_receiving);
			_session = {};
			_tracking.clear();
		}

		[[nodiscard]] SessionID session() const noexcept
		{
			return _session;
		}

		[[nodiscard]] bool contains(ObjectID id) const
		{
			const auto found = _tracking.find(id);
			return found != _tracking.end() && found->second.active;
		}

		[[nodiscard]] std::vector<ObjectID> objects() const
		{
			std::vector<ObjectID> result;
			for (const auto &[id, tracking] : _tracking)
			{
				if (tracking.active)
				{
					result.push_back(id);
				}
			}
			return result;
		}

		// Commit tracking only after application succeeds. A throwing consumer
		// must leave its application object unchanged and can retry the frame.
		template <typename Consumer>
		[[nodiscard]] bool receive(const Update<State> &update, Consumer &&consume)
		{
			OperationGuard guard(_receiving);
			if (_session.isNull() || update.session != _session)
			{
				return false;
			}
			_validate(update);
			const auto found = _tracking.find(update.object);
			if (found != _tracking.end() && _obsolete(found->second, update))
			{
				return false;
			}
			if (found == _tracking.end() && _tracking.size() == _maximumTracked)
			{
				throw spk::Exception("Tracking history full; negotiate a fresh session");
			}
			_apply(update, std::forward<Consumer>(consume));
			return true;
		}

		[[nodiscard]] std::optional<Update<State>> receive(Update<State> update)
		{
			if (!receive(update, [](const Update<State> &) {
				}))
			{
				return std::nullopt;
			}
			return update;
		}
	};
}
