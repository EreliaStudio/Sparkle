#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <stdexcept>
#include <mutex>

#include "miscellaneous/spk_is_container.hpp"

namespace spk
{
    /**
     * @class DataBuffer
     * @brief A buffer for handling binary data with support for serialization and deserialization.
     * 
     * The DataBuffer class provides methods to store, modify, and retrieve binary data. It supports
     * serialization and deserialization of standard layout types and containers. The buffer can be resized,
     * cleared, and reset, and it maintains a bookmark for tracking read positions.
     */
    class DataBuffer
    {
    private:
        std::vector<uint8_t> _data; ///< Vector storing the binary data.
        mutable size_t _bookmark; ///< Bookmark for tracking read position.

    public:
        /**
         * @brief Default constructor for DataBuffer.
         */
        DataBuffer();

        /**
         * @brief Constructor for DataBuffer with a specified size.
         * @param p_dataSize Initial size of the buffer.
         */
        DataBuffer(size_t p_dataSize);

        /**
         * @brief Gets a pointer to the raw data.
         * @return Pointer to the raw data.
         */
        uint8_t* data()
        {
            return (_data.data());
        }

        /**
         * @brief Gets a pointer to the raw data (const version).
         * @return Pointer to the raw data.
         */
        const uint8_t* data() const
        {
            return (_data.data());
        }

        /**
         * @brief Gets the size of the buffer.
         * @return The size of the buffer.
         */
        inline size_t size() const { return _data.size(); }

        /**
         * @brief Gets the current bookmark position.
         * @return The current bookmark position.
         */
        inline size_t bookmark() const { return _bookmark; }

        /**
         * @brief Gets the amount of data left after the bookmark.
         * @return The amount of data left after the bookmark.
         */
        inline size_t leftover() const { return size() - bookmark(); }

        /**
         * @brief Checks if the buffer is empty after the bookmark.
         * @return True if the buffer is empty after the bookmark, false otherwise.
         */
        inline bool empty() const { return leftover() == 0; }

        /**
         * @brief Resizes the buffer to a new size.
         * @param p_newSize The new size for the buffer.
         */
        void resize(const size_t &p_newSize);

        /**
         * @brief Skips a specified number of bytes in the buffer.
         * @param p_number The number of bytes to skip.
         */
        void skip(const size_t &p_number);

        /**
         * @brief Clears the buffer, removing all data.
         */
        void clear();

        /**
         * @brief Resets the buffer, clearing data and resetting the bookmark.
         */
        void reset();

        /**
         * @brief Retrieves a value from the buffer.
         * @tparam OutputType The type of the value to retrieve.
         * @return The retrieved value.
         */
        template <typename OutputType>
        OutputType get() const
        {
            OutputType result;
            *this >> result;
            return (result);
        }

        /**
         * @brief Edits the buffer at a specified offset with a given input value.
         * @tparam InputType The type of the input value.
         * @param p_offset The offset at which to edit the buffer.
         * @param p_input The input value to write to the buffer.
         * @throw std::runtime_error If the offset is out of bounds.
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
         * @brief Edits the buffer at a specified offset with raw data.
         * @param p_offset The offset at which to edit the buffer.
         * @param p_data Pointer to the raw data.
         * @param p_dataSize The size of the raw data.
         * @throw std::runtime_error If the offset is out of bounds.
         */
        void edit(const size_t& p_offset, const void* p_data, const size_t& p_dataSize)
        {
            if (p_offset + p_dataSize > size())
                throw std::runtime_error("Unable to edit, offset is out of bound.");
            memcpy(_data.data() + p_offset, p_data, p_dataSize);
        }

        /**
         * @brief Inserts a value into the buffer.
         * @tparam InputType The type of the input value.
         * @param p_input The input value to insert.
         * @return Reference to the DataBuffer object.
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
         * @brief Retrieves a value from the buffer.
         * @tparam OutputType The type of the value to retrieve.
         * @param p_output The output value to retrieve.
         * @return Const reference to the DataBuffer object.
         * @throw std::runtime_error If there is not enough data left in the buffer.
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
         * @brief Inserts a container into the buffer.
         * @tparam InputType The type of the container.
         * @param p_input The container to insert.
         * @return Reference to the DataBuffer object.
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
         * @brief Retrieves a container from the buffer.
         * @tparam OutputType The type of the container.
         * @param p_output The container to retrieve.
         * @return Const reference to the DataBuffer object.
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

        /**
         * @brief Appends raw data to the buffer.
         * @param p_data Pointer to the raw data.
         * @param p_dataSize The size of the raw data.
         */
        void append(const void* p_data, const size_t& p_dataSize)
        {
            size_t oldSize = size();
            resize(size() + p_dataSize);
            edit(oldSize, p_data, p_dataSize);
        }
    };
}
