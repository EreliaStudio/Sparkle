#include "network/network_binding_test.hpp"
#include <array>
#include <atomic>
#include <future>

TEST_F(NetworkBindingTest, ServerAndThreeClientOwnerThreadsKeepReplicasIsolatedAcrossReconnect)
{
	using namespace std::chrono_literals;
	const std::array ids{ID::generate(), ID::generate(), ID::generate()};
	std::atomic<int> acquired = 0, updated = 0, completed = 0;
	NetworkTestUtils::ThreadFailure failures;
	std::promise<std::uint16_t> portPromise;
	auto portFuture = portPromise.get_future();
	std::jthread serverThread([&](std::stop_token stop) {
		failures.run([&] {
			Server server;
			Source source;
			std::array<Object, 3> objects;
			for (std::size_t i = 0; i < ids.size(); ++i)
			{
				objects[i].change(static_cast<int>(i));
				source.registerObject(ids[i], objects[i]);
			}
			source.bind(server);
			server.start(0);
			portPromise.set_value(server.port());
			bool changed = false, destroyed = false;
			while (!stop.stop_requested())
			{
				server.treatMessages();
				if (!changed && acquired == 3)
				{
					for (std::size_t i = 0; i < ids.size(); ++i)
					{
						objects[i].change(100 + static_cast<int>(i));
					}
					changed = true;
				}
				if (!destroyed && updated == 3)
				{
					for (auto id : ids)
					{
						source.destroyObject(id);
					}
					destroyed = true;
				}
				source.dispatch(Clock::now());
				std::this_thread::sleep_for(1ms);
			}
		});
	});
	ASSERT_EQ(portFuture.wait_for(5s), std::future_status::ready);
	const auto port = portFuture.get();
	std::vector<std::jthread> clients;
	for (std::size_t index = 0; index < ids.size(); ++index)
	{
		clients.emplace_back([&, index] {
			failures.run([&] {
				Client client;
				Replicas replicas;
				replicas.bind(client);
				auto wait = [&](auto predicate) {
					if (!NetworkTestUtils::waitUntil([&] {
							client.treatMessages();
							return predicate();
						},
													 5s))
					{
						throw spk::Exception("Threaded binding timed out");
					}
				};
				client.connect("127.0.0.1", port);
				wait([&] {
					return replicas.isSynchronized();
				});
				EXPECT_TRUE(replicas.requestObject(ids[index]));
				wait([&] {
					return replicas.objects.contains(ids[index]);
				});
				EXPECT_EQ(replicas.objects.size(), 1u);
				EXPECT_EQ(replicas.objects.at(ids[index]).value, static_cast<int>(index));
				++acquired;
				wait([&] {
					return replicas.objects.at(ids[index]).value == 100 + static_cast<int>(index);
				});
				client.disconnect();
				client.connect("127.0.0.1", port);
				client.treatMessages();
				EXPECT_TRUE(replicas.objects.empty());
				wait([&] {
					return replicas.isSynchronized();
				});
				EXPECT_TRUE(replicas.requestObject(ids[index]));
				wait([&] {
					return replicas.objects.contains(ids[index]);
				});
				EXPECT_EQ(replicas.objects.size(), 1u);
				EXPECT_EQ(replicas.objects.at(ids[index]).value, 100 + static_cast<int>(index));
				++updated;
				wait([&] {
					return replicas.objects.empty();
				});
				EXPECT_FALSE(replicas.requestStatus(ids[index]));
				client.disconnect();
				client.treatMessages();
				EXPECT_FALSE(replicas.isSynchronized());
				++completed;
			});
		});
	}
	for (auto &client : clients)
	{
		client.join();
	}
	serverThread.request_stop();
	serverThread.join();
	EXPECT_NO_THROW(failures.rethrow());
	EXPECT_EQ(completed, 3);
}
