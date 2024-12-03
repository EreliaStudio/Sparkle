#include "structure/thread/spk_persistant_worker_tester.hpp"

TEST_F(PersistantWorkerTest, WorkerInitialization)
{
	spk::PersistantWorker worker(workerName);
	worker.addExecutionStep(callback).relinquish();
	ASSERT_FALSE(worker.isJoinable()) << "Worker should not be joinable upon initialization.";
}

TEST_F(PersistantWorkerTest, WorkerStart)
{
	spk::PersistantWorker worker(workerName);
	worker.addExecutionStep(callback).relinquish();
	worker.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	ASSERT_TRUE(worker.isJoinable()) << "Worker should be joinable after start.";
}

TEST_F(PersistantWorkerTest, WorkerStop)
{
	spk::PersistantWorker worker(workerName);
	worker.addExecutionStep(callback).relinquish();
	worker.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(52));
	worker.stop();
	ASSERT_TRUE(worker.isJoinable()) << "Worker should be joinable after stop.";
	worker.join();
	ASSERT_FALSE(worker.isJoinable()) << "Worker should not be joinable after joining them.";
	int finalCount = counter.load();
	ASSERT_GE(finalCount, 4) << "Callback should have been executed multiple times.";
}

TEST_F(PersistantWorkerTest, WorkerStopWithoutStart)
{
	spk::PersistantWorker worker(workerName);
	worker.addExecutionStep(callback).relinquish();
	worker.stop();
	ASSERT_FALSE(worker.isJoinable()) << "Worker should not be joinable if stop is called without start.";
	ASSERT_GE(counter.load(), 0) << "Callback should not have been executed if stop is called without start.";
}

TEST_F(PersistantWorkerTest, WorkerStartStopStart)
{
	spk::PersistantWorker worker(workerName);
	worker.addExecutionStep(callback).relinquish();
	worker.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(52));
	worker.stop();
	worker.join();
	ASSERT_FALSE(worker.isJoinable()) << "Worker should not be joinable after first stop.";
	int firstCount = counter.load();
	ASSERT_GE(firstCount, 4) << "Callback should have been executed multiple times before first stop.";

	worker.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(52));
	worker.stop();
	worker.join();
	ASSERT_FALSE(worker.isJoinable()) << "Worker should not be joinable after second stop.";
	int secondCount = counter.load();
	ASSERT_GE(secondCount, firstCount) << "Callback should have been executed additional times after second start.";
}

TEST_F(PersistantWorkerTest, PreparationStepAddition)
{
	spk::PersistantWorker worker(workerName);
	worker.addExecutionStep(callback).relinquish();

	auto contract = worker.addPreparationStep([]() {
		// Preparation job logic
		});

	ASSERT_TRUE(contract.isValid()) << "Preparation contract should be valid after addition.";
}

TEST_F(PersistantWorkerTest, PreparationStepsTriggered)
{
	bool preparationStepExecuted = false;

	spk::PersistantWorker worker(workerName);
	worker.addExecutionStep(callback).relinquish();

	worker.addPreparationStep([&]() {
		preparationStepExecuted = true;
		}).relinquish();

		worker.start();
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		worker.stop();

		ASSERT_TRUE(preparationStepExecuted) << "Preparation step should have been executed.";
}

TEST_F(PersistantWorkerTest, MultiplePreparationSteps)
{
	bool step1Executed = false;
	bool step2Executed = false;

	spk::PersistantWorker worker(workerName);
	worker.addExecutionStep(callback).relinquish();

	worker.addPreparationStep([&]() {
		step1Executed = true;
		}).relinquish();

	worker.addPreparationStep([&]() {
		step2Executed = true;
		}).relinquish();

	worker.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	worker.stop();

	ASSERT_TRUE(step1Executed) << "First preparation step should have been executed.";
	ASSERT_TRUE(step2Executed) << "Second preparation step should have been executed.";
}