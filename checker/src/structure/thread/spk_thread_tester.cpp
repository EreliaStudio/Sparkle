#include "structure/thread/spk_thread_tester.hpp"

TEST_F(ThreadTest, ThreadInitialization)
{
	spk::Thread thread(threadName, callback);

	ASSERT_FALSE(thread.isJoinable()) << "Thread should not be joinable upon initialization.";
}

TEST_F(ThreadTest, ThreadStart)
{
	spk::Thread thread(threadName, callback);

	thread.start();

	ASSERT_TRUE(thread.isJoinable()) << "Thread should be joinable after start.";
}

TEST_F(ThreadTest, ThreadJoin)
{
	spk::Thread thread(threadName, callback);

	thread.start();
	thread.join();

	ASSERT_FALSE(thread.isJoinable()) << "Thread should not be joinable after join.";
	ASSERT_TRUE(callbackExecuted) << "Callback should have been executed.";
}

TEST_F(ThreadTest, MultipleStartJoin)
{
	spk::Thread thread(threadName, callback);

	thread.start();
	thread.join();

	ASSERT_FALSE(thread.isJoinable()) << "Thread should not be joinable after first join.";
	ASSERT_TRUE(callbackExecuted) << "Callback should have been executed after first join.";

	callbackExecuted = false;

	thread.start();
	thread.join();

	ASSERT_FALSE(thread.isJoinable()) << "Thread should not be joinable after second join.";
	ASSERT_TRUE(callbackExecuted) << "Callback should have been executed after second join.";
}

TEST_F(ThreadTest, StartWhileRunning)
{
	spk::Thread thread(threadName, callback);

	thread.start();

	ASSERT_TRUE(thread.isJoinable()) << "Thread should be joinable after start.";

	thread.start();

	ASSERT_TRUE(thread.isJoinable()) << "Thread should be joinable after restarting.";

	thread.join();

	ASSERT_TRUE(callbackExecuted) << "Callback should have been executed after restarting.";
}

TEST_F(ThreadTest, JoinWithoutStart)
{
	spk::Thread thread(threadName, callback);

	thread.join();

	ASSERT_FALSE(thread.isJoinable()) << "Thread should not be joinable if join is called without start.";
	ASSERT_FALSE(callbackExecuted) << "Callback should not have been executed if join is called without start.";
}

TEST_F(ThreadTest, RelaunchAfterCompletion)
{
	spk::Thread thread(threadName, callback);
	thread.start();
	thread.join();
	ASSERT_FALSE(thread.isJoinable()) << "Thread should not be joinable after first join.";
	ASSERT_TRUE(callbackExecuted) << "Callback should have been executed after first join.";

	callbackExecuted = false;
	thread.start();
	ASSERT_TRUE(thread.isJoinable()) << "Thread should be joinable after restarting.";
	thread.join();
	ASSERT_FALSE(thread.isJoinable()) << "Thread should not be joinable after second join.";
	ASSERT_TRUE(callbackExecuted) << "Callback should have been executed after second join.";
}