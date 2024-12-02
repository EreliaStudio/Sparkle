#pragma once

#include "structure/spk_iostream.hpp"

#include <unordered_map>
#include <vector>
#include <functional>
#include <thread>
#include <memory>

namespace spk
{
	/**
	 * @class Thread
	 * @brief Represents a simple wrapper for managing threads with named prefixes and callbacks.
	 *
	 * The `Thread` class provides an abstraction over the standard `std::thread`, allowing for named threads
	 * with easy management of thread lifecycle and execution of user-defined callbacks.
	 *
	 * ### Example
	 * @code
	 * spk::Thread myThread(L"MyThread", []() {
	 *	 // Perform some threaded work.
	 *	 std::this_thread::sleep_for(std::chrono::seconds(1));
	 * });
	 * myThread.start();
	 * myThread.join();
	 * @endcode
	 */
	class Thread
	{
	private:
		std::wstring _name;
		std::function<void()> _innerCallback;
		std::unique_ptr<std::thread> _handle = nullptr;

	public:
		/**
		 * @brief Constructs a `Thread` object.
		 * @param p_name Name of the thread.
		 * @param p_callback Callback function to execute in the thread.
		 */
		Thread(const std::wstring& p_name, const std::function<void()>& p_callback);

		/**
		 * @brief Destructor for the `Thread` class.
		 *
		 * Ensures that the thread is joined if it is still running.
		 */
		~Thread();

		/**
		 * @brief Starts the thread, executing the callback function.
		 *
		 * If the thread is already running, it will be joined before restarting.
		 */
		virtual void start();

		/**
		 * @brief Joins the thread, waiting for its completion.
		 *
		 * Ensures the thread is properly joined and resets the thread handle.
		 */
		virtual void join();

		/**
		 * @brief Checks if the thread is joinable.
		 * @return True if the thread is joinable; otherwise, false.
		 */
		bool isJoinable() const;
	};
}