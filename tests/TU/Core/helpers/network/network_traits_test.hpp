#pragma once

#include <gtest/gtest.h>
#include <network/network.hpp>
#include <set>

class NetworkTraitsTest : public testing::Test
{
protected:
	struct State
	{
		int value = 0;
	};
	struct Codec
	{
		inline static int decodes = 0;
		static void encode(spk::Message::Writer &writer, const State &state)
		{
			writer << state.value;
		}
		static State decode(const spk::Message::Reader &reader)
		{
			++decodes;
			return {reader.get<int>()};
		}
	};
	using ID = spk::UUID;
	using Clock = spk::Network::Clock;
	using Protocol = spk::Network::Protocol<State, Codec>;
	using Update = spk::Network::Update<State>;
	using Request = spk::Network::Request;
	using Reply = spk::Network::Reply;
	using Edit = spk::Network::Edit;
	using Status = spk::Network::RequestQueue::Status;

	class Object final : public spk::Network::PublishableTrait<State>, public spk::Network::ReplicableTrait<State>
	{
	protected:
		State _buildNetworkState() const override
		{
			++builds;
			if (failBuild)
			{
				throw spk::Exception("Snapshot failure");
			}
			return {value};
		}
		void _applyNetworkState(const State &state) override
		{
			if (failApply)
			{
				throw spk::Exception("Application failure");
			}
			value = state.value;
			++applications;
		}

	public:
		int value = 0, applications = 0;
		mutable int builds = 0;
		bool failBuild = false, failApply = false;
		void change(int next)
		{
			value = next;
			invalidateNetworkState();
		}
	};

	class Source : public spk::Network::PublicationSourceTrait<State, Codec>
	{
	protected:
		bool _sendMessage(ID peer, const spk::Message &message) override
		{
			if (onSend)
			{
				onSend();
			}
			if (throwing.contains(peer))
			{
				throw spk::Exception("Transport failure");
			}
			if (blocked.contains(peer))
			{
				return false;
			}
			sent.emplace_back(peer, message);
			return true;
		}

	public:
		Source() :
			PublicationSourceTrait(42)
		{
		}
		std::set<ID> blocked, throwing;
		std::vector<std::pair<ID, spk::Message>> sent;
		std::function<void()> onSend;
	};

	class Replicas : public spk::Network::ReplicaCollectionTrait<State, Codec>
	{
	protected:
		void _applyReplica(ID id, const State &state) override
		{
			if (onApply)
			{
				onApply();
			}
			if (failApply)
			{
				throw spk::Exception("Collection failure");
			}
			objects[id].applyNetworkState(state);
		}
		void _removeReplica(ID id) override
		{
			if (failRemove)
			{
				throw spk::Exception("Removal failure");
			}
			objects.erase(id);
			++removals;
		}

	public:
		explicit Replicas(std::size_t capacity = 65536) :
			ReplicaCollectionTrait(42, capacity)
		{
		}
		std::map<ID, Object> objects;
		int removals = 0;
		bool failApply = false, failRemove = false;
		std::function<void()> onApply;
	};

	class RequestSource : public spk::Network::RequestSourceTrait<State, Codec>
	{
	protected:
		bool _sendMessage(ID peer, const spk::Message &message) override
		{
			if (onSend)
			{
				onSend();
			}
			if (blocked)
			{
				return false;
			}
			sent.emplace_back(peer, message);
			return true;
		}
		void _requestObject(ID peer, const Request &request) override
		{
			requests.emplace_back(peer, request);
			if (failRequest)
			{
				throw spk::Exception("Provider failure");
			}
			if (immediate)
			{
				EXPECT_TRUE(fulfillRequest(peer, request, {37}));
			}
		}

	public:
		RequestSource() :
			RequestSourceTrait(42)
		{
		}
		std::vector<std::pair<ID, Request>> requests;
		std::vector<std::pair<ID, spk::Message>> sent;
		bool blocked = false, immediate = false, failRequest = false;
		std::function<void()> onSend;
	};

	class RequestReplicas : public spk::Network::RequestReplicaCollectionTrait<State, Codec>
	{
	protected:
		bool _sendMessage(const spk::Message &message) override
		{
			if (onSend)
			{
				onSend();
			}
			if (throwSend)
			{
				throw spk::Exception("Transport failure");
			}
			if (blocked)
			{
				return false;
			}
			sent.push_back(message);
			return true;
		}
		void _applyReplica(ID id, const State &state) override
		{
			objects[id].applyNetworkState(state);
		}
		void _removeReplica(ID id) override
		{
			objects.erase(id);
		}

	public:
		RequestReplicas() :
			RequestReplicaCollectionTrait(42)
		{
		}
		std::map<ID, Object> objects;
		std::vector<spk::Message> sent;
		bool blocked = false, throwSend = false;
		std::function<void()> onSend;
	};

	Protocol protocol{42};
	ID peer = ID::generate(), object = ID::generate(), session = ID::generate();
	Clock::time_point now{};
	void SetUp() override
	{
		Codec::decodes = 0;
	}
	Update update(int value, std::uint64_t revision = 1, std::uint64_t tracking = 1)
	{
		return {session, object, tracking, revision, Edit::Set, std::make_shared<const State>(State{value})};
	}
};
