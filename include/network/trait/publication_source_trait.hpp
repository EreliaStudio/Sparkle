#pragma once

#include "network/replication/operation_guard.hpp"
#include "network/replication/protocol.hpp"
#include "network/replication/publisher.hpp"
#include "publishable_trait.hpp"

namespace spk::Network
{
	// One owner thread, including invalidation and object destruction. The source
	// observes application-owned objects; it never owns their storage.
	template <typename State, typename Codec>
	class PublicationSourceTrait
	{
	public:
		using Configuration = typename Publisher<State>::Configuration;
		using DispatchResult = typename Publisher<State>::DispatchResult;

	private:
		struct Registration
		{
			PublishableTrait<State> *object;
			std::weak_ptr<void> lifetime;
			std::shared_ptr<bool> dirty;
			typename PublishableTrait<State>::EditionContract contract;
		};
		Publisher<State> _publisher;
		Protocol<State, Codec> _protocol;
		std::map<ObjectID, Registration> _registrations;
		bool _dispatching = false;

		void _capture(ObjectID id, Registration &registration)
		{
			*registration.dirty = false;
			try
			{
				_publisher.publish(id, registration.object->buildNetworkState());
			} catch (...)
			{
				*registration.dirty = true;
				throw;
			}
		}

		void _captureChanges()
		{
			std::erase_if(_registrations, [](const auto &entry) {
				return entry.second.lifetime.expired();
			});
			for (auto &[id, registration] : _registrations)
			{
				if (!registration.lifetime.expired() && *registration.dirty)
				{
					_capture(id, registration);
				}
			}
		}

		auto _register(ObjectID id, PublishableTrait<State> &object)
		{
			const auto previous = _registrations.find(id);
			if (previous != _registrations.end() && previous->second.lifetime.expired())
			{
				_registrations.erase(previous);
			}
			auto dirty = std::make_shared<bool>(true);
			auto contract = object.subscribeToNetworkEdition([dirty] {
				*dirty = true;
			});
			auto [found, inserted] = _registrations.emplace(id, Registration{&object, object._lifetime, dirty, std::move(contract)});
			if (!inserted)
			{
				throw spk::Exception("Network object already registered");
			}
			return found;
		}

	protected:
		// true means this ordered transport accepted the frame, not remote ACK.
		[[nodiscard]] virtual bool _sendMessage(PeerID peer, const spk::Message &message) = 0;
		virtual void _closeRequests(PeerID)
		{
		}
		virtual void _dispatchReplies(std::size_t)
		{
		}
		virtual bool _receiveRequest(PeerID, const Request &)
		{
			throw spk::Exception("This publication source does not serve requests");
		}
		[[nodiscard]] Publisher<State> &_publication() noexcept
		{
			return _publisher;
		}
		[[nodiscard]] const Protocol<State, Codec> &_protocolCodec() const noexcept
		{
			return _protocol;
		}
		void _requireIdle() const
		{
			if (_dispatching)
			{
				throw spk::Exception("Publication source mutation during dispatch");
			}
		}

	public:
		explicit PublicationSourceTrait(spk::Message::Type type, Configuration configuration = {}, std::size_t maximumBytes = 2 * 1024 * 1024) :
			_publisher(configuration),
			_protocol(type, maximumBytes)
		{
		}
		virtual ~PublicationSourceTrait() = default;

		void registerObject(ObjectID id, PublishableTrait<State> &object)
		{
			_requireIdle();
			const auto found = _register(id, object);
			try
			{
				OperationGuard guard(_dispatching);
				_capture(id, found->second);
			} catch (...)
			{
				_registrations.erase(found);
				throw;
			}
		}

		// Local detachment retains the last published snapshot and followers.
		void unregisterObject(ObjectID id)
		{
			_requireIdle();
			_registrations.erase(id);
		}

		// Explicit authoritative deletion, unlike detachment or C++ destruction.
		void destroyObject(ObjectID id)
		{
			_requireIdle();
			_publisher.destroy(id);
			_registrations.erase(id);
		}

		[[nodiscard]] SessionID openPeer(PeerID peer)
		{
			_requireIdle();
			return _publisher.open(peer);
		}

		virtual void closePeer(PeerID peer) final
		{
			_requireIdle();
			_closeRequests(peer);
			_publisher.close(peer);
		}

		void follow(PeerID peer, ObjectID object)
		{
			_requireIdle();
			_publisher.follow(peer, object);
		}

		void forget(PeerID peer, ObjectID object)
		{
			_requireIdle();
			_publisher.forget(peer, object);
		}

		[[nodiscard]] virtual bool receiveMessage(PeerID peer, const spk::Message &message) final
		{
			_requireIdle();
			return _receiveRequest(peer, _protocol.decodeRequest(message));
		}

		virtual DispatchResult dispatch(Clock::time_point now, std::size_t maximumAttempts = 64) final
		{
			OperationGuard guard(_dispatching);
			if (maximumAttempts != 0 && _publisher.publicationDue(now))
			{
				_captureChanges();
			}
			auto result = _publisher.dispatch(now, maximumAttempts, [this](PeerID peer, const Update<State> &update) {
				return _sendMessage(peer, _protocol.encode(update));
			});
			_dispatchReplies(maximumAttempts);
			return result;
		}
	};
}
