#pragma once

#include <iterator>
#include <memory>
#include <vector>
#include <functional>

#include "input/spk_input.hpp"

namespace spk
{
    /**
     * @brief An InputMapping maps inputs (keyboard, mouse, controller) to an event code.
     *
     * @see Input, Keyboard, Mouse, InputDecoder
     */
    class InputMapping
    {
    public:
        /**
         * @brief A mapping with that value as its code() is not valid.
         *
         * This value is used by the default initializer.
         */
        static constexpr size_t kDummyMappingCode = std::numeric_limits<size_t>::max();

        /// Default name if a debug name was not provided.
        static constexpr char KUnnamedMapping[] = "";

        /**
         * @brief Default constructor. The resulting mapping is incomplete, and thus not valid.
         *
         * Used to initialize fixed-sized arrays.
         */
        InputMapping() : _name(""), _code(kDummyMappingCode) {}

        virtual ~InputMapping() = default;

        /**
         * @brief Constructs an InputMapping to map an Input to an EventType.
         *
         * @param p_expects The Input of this mapping.
         * @param p_code The event code triggered by this mapping.
         * @param p_debugName The optional debug name.
         */
        template <typename EventType>
        InputMapping(const Input &p_expects, EventType p_code, const std::string &p_debugName = std::string(KUnnamedMapping))
            : _name(p_debugName), _code(static_cast<size_t>(p_code)), _expects(p_expects)
        {
        }

        /// Returns true if the mapping is valid. Mapping created with the default constructor will be incomplete.
        bool valid() const { return _code != kDummyMappingCode; }

        /// Getter for the mapping's name.
        const std::string &name() const { return _name; }

        /// Getter for the Input expected by this mapping.
        const Input &expects() const { return _expects; }

        /// Getter for the event code that this mapping triggers.
        size_t code() const { return _code; }

        /// Getter for the event code that this mapping triggers.
        template <typename EventType>
        EventType code() const { static_cast<EventType>(_code); }

        /// Compares two mappings. True if other has the same input and event code.
        bool operator==(const InputMapping &other) const
        {
            return (code() == other.code()) && (expects() == other.expects());
        }

        /**
         * @brief Sets all members to their default values. The resulting object is not active and cannot be used anymore.
         */
        void clear()
        {
            _name.clear();
            _expects = Input();
            _code = kDummyMappingCode;
        }

    private:
        std::string _name;
        Input _expects;
        size_t _code;
    };
}
