#pragma once

#include <functional>
#include <gtest/gtest.h>
#include <network/network.hpp>
#include <set>
#include <vector>

class NetworkTraitsTest : public testing::Test
{
protected:
	static spk::Message payload(int value)
	{
		spk::Message::Writer writer;
		writer << value;
		return std::move(writer).build();
	}
	using ID = spk::UUID;
	using Clock = spk::Network::Clock;
	using Protocol = spk::Network::Protocol;
	using Update = spk::Network::Update;
	using Request = spk::Network::Request;
	using Edit = spk::Network::Edit;
	using Status = spk::Network::ReplicaCollectionTrait::RequestStatus;

	class Object final : public spk::Network::PublishableTrait, public spk::Network::ReplicableTrait
	{
	protected:
		void _writeNetworkState(spk::Message::Writer &writer) const override
		{
			++builds;
			if (failBuild)
			{
				throw spk::Exception("Snapshot failure");
			}
			writer << value;
		}
		void _readNetworkState(const spk::Message::Reader &reader) override
		{
			if (failApply)
			{
				throw spk::Exception("Application failure");
			}
			value = reader.get<int>();
			++reads;
			++applications;
		}

	public:
		inline static int reads = 0;
		int value = 0, applications = 0;
		mutable int builds = 0;
		bool failBuild = false, failApply = false;
		void change(int next)
		{
			value = next;
			invalidate();
		}
	};

	class Source : public spk::Network::PublicationSourceTrait
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
		explicit Source(Configuration configuration = {}) :
			PublicationSourceTrait(42, configuration)
		{
		}
		std::set<ID> blocked, throwing;
		std::vector<std::pair<ID, spk::Message>> sent;
		std::function<void()> onSend;
	};

	class Replicas : public spk::Network::ReplicaCollectionTrait
	{
	protected:
		spk::Network::ReplicableTrait *_findReplica(ID id) override
		{
			if (onApply)
			{
				onApply();
			}
			if (failApply)
			{
				throw spk::Exception("Collection failure");
			}
			auto found = objects.find(id);
			return found == objects.end() ? nullptr : &found->second;
		}
		spk::Network::ReplicableTrait &_createReplica(ID id) override
		{
			++creations;
			auto &object = objects[id];
			object.failApply = failInitialApply;
			return object;
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
		int removals = 0, creations = 0;
		bool failApply = false, failRemove = false, failInitialApply = false;
		std::function<void()> onApply;
	};

	class RequestSource : public Source
	{
	protected:
		void _requestObject(ID peer, const Request &request) override
		{
			requests.emplace_back(peer, request);
			if (failRequest)
			{
				throw spk::Exception("Provider failure");
			}
			if (immediate)
			{
				EXPECT_TRUE(fulfillRequest(peer, request, payload(37)));
			}
		}

	public:
		using Source::Source;
		std::vector<std::pair<ID, Request>> requests;
		bool immediate = false, failRequest = false;
	};
	class RequestReplicas : public Replicas
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

	public:
		using Replicas::Replicas;
		std::vector<spk::Message> sent;
		bool blocked = false, throwSend = false;
		std::function<void()> onSend;
	};

	Protocol protocol{42};
	ID peer = ID::generate(), object = ID::generate(), session = ID::generate();
	Clock::time_point now{};
	void SetUp() override
	{
		Object::reads = 0;
	}
	Update update(int value, std::uint64_t revision = 1, std::uint64_t tracking = 1)
	{
		return {session, object, tracking, revision, Edit::Set, payload(value)};
	}
};
