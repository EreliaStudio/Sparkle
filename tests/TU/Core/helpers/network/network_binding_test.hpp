#pragma once

#include "network/network.hpp"
#include "network_test_utils.hpp"
#include <gtest/gtest.h>
#include <map>
#include <thread>

class NetworkBindingTest : public testing::Test
{
protected:
	using ID = spk::UUID;
	using Clock = spk::Network::Clock;
	class Server : public spk::Server
	{
	public:
		~Server()
		{
			stop();
		}
	};
	class Client : public spk::Client
	{
	public:
		~Client()
		{
			disconnect();
		}
	};
	struct State
	{
		int value = 0;
	};
	struct Codec
	{
		static void encode(spk::Message::Writer &writer, const State &state)
		{
			writer << state.value;
		}
		static State decode(const spk::Message::Reader &reader)
		{
			return {reader.get<int>()};
		}
	};
	using Protocol = spk::Network::Protocol<State, Codec>;
	class Object : public spk::Network::PublishableTrait<State>, public spk::Network::ReplicableTrait<State>
	{
		State _buildNetworkState() const override
		{
			return {value};
		}
		void _applyNetworkState(const State &state) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			value = state.value;
			++applications;
		}

	public:
		const std::thread::id owner = std::this_thread::get_id();
		int value = 0, applications = 0;
		void change(int next)
		{
			value = next;
			invalidate();
		}
	};
	class Source : public spk::Network::PublicationSourceTrait<State, Codec>
	{
		const std::thread::id _owner = std::this_thread::get_id();
		void _requestObject(ID peer, const spk::Network::Request &request) override
		{
			EXPECT_EQ(std::this_thread::get_id(), _owner);
			PublicationSourceTrait::_requestObject(peer, request);
		}

	public:
		explicit Source(spk::Message::Type type = 42) :
			PublicationSourceTrait(type, {.interval = Clock::duration::zero()})
		{
		}
	};
	class Replicas : public spk::Network::ReplicaCollectionTrait<State, Codec>
	{
		spk::Network::ReplicableTrait<State> *_findReplica(ID id) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			auto found = objects.find(id);
			return found == objects.end() ? nullptr : &found->second;
		}
		spk::Network::ReplicableTrait<State> &_createReplica(ID id) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			++creations;
			return objects[id];
		}
		void _removeReplica(ID id) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			objects.erase(id);
			++removals;
		}

	public:
		explicit Replicas(spk::Message::Type type = 42) :
			ReplicaCollectionTrait(type)
		{
		}
		const std::thread::id owner = std::this_thread::get_id();
		std::map<ID, Object> objects;
		int creations = 0, removals = 0;
	};
	ID object = ID::generate();
};
