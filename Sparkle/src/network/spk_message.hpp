#pragma once

#include <cstdint>
#include <cstring>
#include "data_structure/spk_data_buffer.hpp"

namespace spk
{
	/**
	 * @class Message
	 * @brief Represents a message that consists of a header and a data buffer.
	 * 
	 * This class encapsulates the details of a message, which includes a header containing metadata
	 * and a buffer for the actual message data. The Message class provides various methods to manipulate
	 * the buffer and access header information.
	 */
	class Message
	{
	public:
		/**
		 * @class Header
		 * @brief Represents the header of a message, containing metadata such as type and emitter ID.
		 * 
		 * The Header class encapsulates metadata for a message, including the type of message, the ID
		 * of the client that sent the message, and a reserved field for future use. It also keeps track
		 * of the message length internally.
		 */
		class Header
		{
		public:
			using ClientID = size_t;  ///< Type alias for Client ID.
			using Type = int32_t;	 ///< Type alias for Message Type.

		private:
			friend class Message;
			size_t _length;  ///< Length of the message data.

		public:
			int32_t type;		///< Type of the message.
			ClientID emitterID;  ///< ID of the client that sent the message.
			uint8_t reserved[16]; ///< Reserved bytes for future use.

			/**
			 * @brief Default constructor for Header. Initializes type and emitterID to 0 and reserves memory.
			 */
			Header() :
				type(0),
				emitterID(0),
				_length(0)
			{
				std::memset(reserved, 0, sizeof(reserved));
			}

			/**
			 * @brief Constructor for Header with message type.
			 * @param p_type The type of the message.
			 */
			Header(Type p_type) :
				type(p_type),
				emitterID(0),
				_length(0)
			{
				std::memset(reserved, 0, sizeof(reserved));
			}

			/**
			 * @brief Constructor for Header with client ID and message type.
			 * @param p_emitterID The ID of the client that sent the message.
			 * @param p_type The type of the message.
			 */
			Header(const ClientID& p_emitterID, const Type& p_type) :
				type(p_type),
				emitterID(p_emitterID),
				_length(0)
			{
				std::memset(reserved, 0, sizeof(reserved));
			}
			
			/**
			 * @brief Gets the length of the message data.
			 * @return The length of the message data.
			 */
			size_t length() const
			{
				return (_length);
			}
		};

	private:
		Header _header;   ///< Header of the message.
		DataBuffer _buffer; ///< Data buffer of the message.

	public:
		/**
		 * @brief Default constructor for Message.
		 */
		Message() = default;

		/**
		 * @brief Constructor for Message with message type.
		 * @param p_messageType The type of the message.
		 */
		Message(const Header::Type& p_messageType) :
			_header(p_messageType)
		{
		}

		/**
		 * @brief Constructor for Message with client ID and message type.
		 * @param p_clientToRedirectMessage The ID of the client to redirect the message to.
		 * @param p_messageType The type of the message.
		 */
		Message(const Header::ClientID& p_clientToRedirectMessage, const Header::Type& p_messageType) :
			_header(p_clientToRedirectMessage, p_messageType)
		{
		}

		/**
		 * @brief Gets the type of the message.
		 * @return The type of the message.
		 */
		const Header::Type& type() const
		{
			return (_header.type);
		}

		/**
		 * @brief Gets the header of the message.
		 * @return Reference to the header of the message.
		 */
		Header& header()
		{
			return (_header);
		}

		/**
		 * @brief Gets the header of the message (const version).
		 * @return Const reference to the header of the message.
		 */
		const Header& header() const
		{
			return (_header);
		}

		/**
		 * @brief Gets the data buffer of the message.
		 * @return Reference to the data buffer.
		 */
		DataBuffer& buffer()
		{
			return (_buffer);
		}

		/**
		 * @brief Gets the data buffer of the message (const version).
		 * @return Const reference to the data buffer.
		 */
		const DataBuffer& buffer() const
		{
			return (_buffer);
		}

		/**
		 * @brief Gets the raw data of the message.
		 * @return Pointer to the raw data of the message.
		 */
		const uint8_t* data() const
		{
			return (_buffer.data());
		}

		/**
		 * @brief Gets the size of the message data.
		 * @return The size of the message data.
		 */
		size_t size() const
		{
			return (_buffer.size());
		}

		/**
		 * @brief Resizes the message buffer.
		 * @param p_newSize The new size for the message buffer.
		 */
		void resize(const size_t& p_newSize)
		{
			_buffer.resize(p_newSize);
			_header._length = _buffer.size();
		}

		/**
		 * @brief Clears the message buffer.
		 */
		void clear()
		{
			_buffer.clear();
			_header._length = _buffer.size();
		}

		/**
		 * @brief Resets the message buffer.
		 */
		void reset()
		{
			_buffer.reset();
			_header._length = _buffer.size();
		}

		/**
		 * @brief Skips a specified number of bytes in the buffer.
		 * @param p_number The number of bytes to skip.
		 */
		void skip(const size_t& p_number)
		{
			_buffer.skip(p_number);
		}

		/**
		 * @brief Edits the buffer at a specified offset with a given input.
		 * @tparam InputType The type of the input data.
		 * @param p_offset The offset to start editing.
		 * @param p_input The input data to write.
		 */
		template <typename InputType>
		void edit(const size_t& p_offset, const InputType& p_input)
		{
			_buffer.edit(p_offset, p_input);
		}

		/**
		 * @brief Edits the buffer at a specified offset with raw data.
		 * @param p_offset The offset to start editing.
		 * @param p_data Pointer to the raw data.
		 * @param p_dataSize The size of the raw data.
		 */
		void edit(const size_t& p_offset, const void* p_data, const size_t& p_dataSize)
		{
			_buffer.edit(p_offset, p_data, p_dataSize);
		}

		/**
		 * @brief Appends raw data to the buffer.
		 * @param p_data Pointer to the raw data.
		 * @param p_dataSize The size of the raw data.
		 */
		void append(const void* p_data, const size_t& p_dataSize)
		{
			_buffer.append(p_data, p_dataSize);
			_header._length = _buffer.size();
		}

		/**
		 * @brief Gets data from the buffer.
		 * @tparam OutputType The type of the data to get.
		 * @return The data from the buffer.
		 */
		template <typename OutputType>
		OutputType get() const
		{
			return (_buffer.get<OutputType>());	
		}

		/**
		 * @brief Inserts data into the message buffer.
		 * @tparam InputType The type of the input data.
		 * @param p_input The input data to insert.
		 * @return Reference to the Message object.
		 */
		template <typename InputType>
		Message& operator<<(const InputType& p_input)
		{
			_buffer << p_input;
			_header._length = _buffer.size();
			return *this;
		}

		/**
		 * @brief Extracts data from the message buffer.
		 * @tparam OutputType The type of the output data.
		 * @param p_output The output data to extract into.
		 * @return Const reference to the Message object.
		 */
		template <typename OutputType>
		const Message& operator>>(OutputType& p_output) const
		{
			_buffer >> p_output;
			return *this;
		}
	};
}
