#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <stdexcept>
#include <mutex>

#include "spk_sfinae.hpp"

namespace spk
{
	/**
	 * @class DataBuffer
	 * @brief A utility class for managing and manipulating binary data buffers.
	 * 
	 * The `DataBuffer` class provides functionalities for reading, writing, and editing binary data.
	 * It supports standard layout types and containers, allowing serialization and deserialization of structured data.
	 * 
	 * Example usage:
	 * @code
	 * spk::DataBuffer buffer;
	 * int value = 42;
	 * std::string text = "This is a test";
	 * buffer << value; // Write to buffer
	 * buffer << text; // Write to buffer
	 * 
	 * int retrieved;
	 * std::string retrievedText;
	 * buffer >> retrieved; // Read from buffer
	 * buffer >> retrievedText; // Read from buffer
	 * @endcode
	 */
	class DataBuffer
	{
	private:
		std::vector<uint8_t> _data; /**< @brief Internal storage for binary data. */
		mutable size_t _bookmark; /**< @brief Current read position within the buffer. */

	public:
		/**
		 * @brief Default constructor. Initializes an empty buffer.
		 */
		DataBuffer();

		/**
		 * @brief Constructor that initializes the buffer with a specific size.
		 * 
		 * @param p_dataSize The initial size of the buffer.
		 */
		DataBuffer(size_t p_dataSize);

		/**
		 * @brief Returns a pointer to the internal data.
		 * 
		 * @return A non-const pointer to the data.
		 */
		uint8_t* data();

		/**
		 * @brief Returns a const pointer to the internal data.
		 * 
		 * @return A const pointer to the data.
		 */
		const uint8_t* data() const;

		/**
		 * @brief Retrieves the size of the buffer.
		 * 
		 * @return The size of the buffer in bytes.
		 */
		size_t size() const;

		/**
		 * @brief Retrieves the current bookmark position.
		 * 
		 * @return The current read position within the buffer.
		 */
		size_t bookmark() const;

		/**
		 * @brief Calculates the remaining unread bytes in the buffer.
		 * 
		 * @return The number of unread bytes.
		 */
		size_t leftover() const;

		/**
		 * @brief Checks if the buffer is empty (i.e., no unread bytes).
		 * 
		 * @return `true` if empty, otherwise `false`.
		 */
		bool empty() const;

		/**
		 * @brief Resizes the buffer.
		 * 
		 * @param p_newSize The new size of the buffer.
		 */
		void resize(const size_t& p_newSize);

		/**
		 * @brief Skips a specified number of bytes.
		 * 
		 * @param p_number The number of bytes to skip.
		 * @throws std::runtime_error If the number of bytes to skip exceeds the leftover bytes.
		 */
		void skip(const size_t& p_number);

		/**
		 * @brief Clears the buffer and resets the bookmark.
		 */
		void clear();

		/**
		 * @brief Resets the bookmark to the beginning of the buffer.
		 */
		void reset();

		/**
		 * @brief Reads a value of type `OutputType` from the buffer.
		 * 
		 * @tparam OutputType The type of data to read.
		 * @return The value read from the buffer.
		 */
		template <typename OutputType>
		OutputType get() const
		{
			OutputType result;
			*this >> result;
			return (result);
		}

		/**
		 * @brief Edits a portion of the buffer at a specific offset with a value of type `InputType`.
		 * 
		 * @tparam InputType The type of data to write.
		 * @param p_offset The offset at which to edit the buffer.
		 * @param p_input The value to write.
		 * @throws std::runtime_error If the offset exceeds the buffer size.
		 */
		template <typename InputType>
		void edit(const size_t& p_offset, const InputType& p_input)
		{
			static_assert(std::is_standard_layout<InputType>::value, "Unable to handle this type.");
			if (p_offset + sizeof(InputType) > size())
				throw std::runtime_error("Unable to edit, offset is out of bound.");
			memcpy(_data.data() + p_offset, &p_input, sizeof(InputType));
		}

		/**
		 * @brief Edits a portion of the buffer at a specific offset with raw data.
		 * 
		 * @param p_offset The offset at which to edit the buffer.
		 * @param p_data Pointer to the raw data.
		 * @param p_dataSize Size of the raw data.
		 * @throws std::runtime_error If the offset exceeds the buffer size.
		 */
		void edit(const size_t& p_offset, const void* p_data, const size_t& p_dataSize);

		/**
		 * @brief Appends raw data to the buffer.
		 * 
		 * @param p_data Pointer to the raw data.
		 * @param p_dataSize Size of the raw data to append.
		 */
		void append(const void* p_data, const size_t& p_dataSize);

		/**
		 * @brief Writes a value or container to the buffer.
		 * 
		 * @tparam InputType The type of data to write.
		 * @param p_input The value or container to write.
		 * @return A reference to the buffer.
		 */
		template <typename InputType, typename std::enable_if_t<!spk::IsContainer<InputType>::value>* = nullptr>
		DataBuffer& operator<<(const InputType& p_input)
		{
			static_assert(std::is_standard_layout<InputType>::value, "Unable to handle this type.");

			size_t previous_size(_data.size());

			_data.resize(_data.size() + sizeof(InputType));
			std::memcpy(_data.data() + previous_size, &p_input, sizeof(InputType));

			return *this;
		}

		/**
		 * @brief Reads a value or container from the buffer.
		 * 
		 * @tparam OutputType The type of data to read.
		 * @param p_output The variable to store the read value.
		 * @return A reference to the buffer.
		 */
		template <typename OutputType, typename std::enable_if_t<!spk::IsContainer<OutputType>::value>* = nullptr>
		const DataBuffer& operator>>(OutputType& p_output) const
		{
			static_assert(std::is_standard_layout<OutputType>::value, "Unable to handle this type.");
			if (leftover() < sizeof(OutputType))
				throw std::runtime_error("Unable to retrieve data buffer content.");
			std::memcpy(&p_output, _data.data() + bookmark(), sizeof(OutputType));
			_bookmark += sizeof(OutputType);
			return *this;
		}

		/**
		 * @brief Writes a container to the buffer.
		 * 
		 * @tparam InputType The type of container to write.
		 * @param p_input The container to write.
		 * @return A reference to the buffer.
		 */
		template <typename InputType, typename std::enable_if_t<spk::IsContainer<InputType>::value>* = nullptr>
		DataBuffer& operator<<(const InputType& p_input)
		{
			*this << p_input.size();
			for (auto it = p_input.begin(); it != p_input.end(); ++it)
			{
				*this << *it;
			}
			return *this;
		}

		/**
		 * @brief Reads a container from the buffer.
		 * 
		 * @tparam OutputType The type of container to read.
		 * @param p_output The container to store the read data.
		 * @return A reference to the buffer.
		 */
		template <typename OutputType, typename std::enable_if_t<spk::IsContainer<OutputType>::value>* = nullptr>
		const DataBuffer& operator>>(OutputType& p_output) const
		{
			p_output.resize(this->get<size_t>());
			for (auto it = p_output.begin(); it != p_output.end(); ++it)
			{
				*this >> *it;
			}
			return *this;
		}
	};
}
