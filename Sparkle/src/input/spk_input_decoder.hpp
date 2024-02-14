#pragma once

#include <mutex>

#include "input/spk_input.hpp"
#include "input/spk_input_mapping.hpp"
#include "design_pattern/spk_notifier.hpp"

namespace spk
{
    /**
     * @brief InputDecoder maps Inputs to event codes and notifies of said events.
     *
     * This class allocates a vectors which size is equal to the highest event code in your
     * mappings. Keep that value close to the actual number of event codes unless you want a big
     * bad bloat.
     *
     * InputDecoder is thread safe.
     *
     * @code
     * InputDecoder inputDecoder;
     *
     * // Set some mappings
     * inputDecoder.set_mappings({
     *     InputMapping{Input{Mouse::Left, InputState::Pressed}, MyEventsEnum::First},
     *     InputMapping{Input{Keyboard::Key::A, InputState::Released}, MyEventsEnum::Second},
     * });
     *
     * // Subscribe
     * Notifier::Callback callback = [](){ std::cout << "Triggered and event" << std::endl; };
     * std::unique_ptr<Notifier::Contract> contract = inputDecoder.subscribe(MyEventsEnum::First, callback);
     *
     * // Trigger event
     * inputDecoder.add(Input{Mouse::Left, InputState::Pressed});
     *
     * // Cancel subscription
     * contract->cancel();
     * @endcode
     *
     * @see Input, InputMapping, Notifier
     */
    class InputDecoder
    {
    public:
        /**
         * @brief Default constructor.
         *
         * All public methods are safe to be called right after construction.
         */
        InputDecoder() = default;

        /**
         * @brief Replaces all mappings.
         *
         * Replacing mappings preserves all existing event subscriptions .
         */
        void set_mappings(const std::vector<InputMapping> &p_mappings);

        /**
         * @brief Updates a single mapping.
         *
         * If the event code was already in use, replaces the previous mapping.
         * Replacing a mapping preserves all existing subscriptions to the associated event.
         */
        void update(const InputMapping &p_mapping);

        /**
         * @brief Add an input to be decoded.
         *
         * If this call triggers an event, all subscribed callbacks will be called by that method.
         */
        void add(const Input &p_input);

        /**
         * @brief Subscribes a callback that will be called when the event is triggered.
         *
         * @param p_eventCode The event code to which you subscribe.
         * @param p_callback The code to execute when the event occurs.
         *
         * @returns a Notifier::Contract to hold that subscription.
         * @see Notifier
         */
        template <typename EventCodeType>
        std::unique_ptr<Notifier::Contract> subscribe(EventCodeType p_eventCode, const Notifier::Callback &p_callback)
        {
            return subscribe(static_cast<size_t>(p_eventCode), p_callback);
        }

        /**
         * @brief Subscribes a callback that will be called when the event is triggered.
         *
         * @param p_eventCode The event code to which you subscribe.
         * @param p_callback The code to execute when the event occurs.
         *
         * @returns a Notifier::Contract to hold that subscription.
         * @see Notifier
         */
        std::unique_ptr<Notifier::Contract> subscribe(size_t p_eventCode, const Notifier::Callback &p_callback);

        /**
         * @brief Get the current mapping for an event.
         */
        template <typename EventCodeType>
        const InputMapping &mapping(EventCodeType p_mappingCode)
        {
            return _mappings[static_cast<int>(p_mappingCode)];
        }

    private:
        std::vector<std::unique_ptr<Notifier>> _notifiers; // TODO initialize

        std::vector<int> _inputToMappings[Input::max_code];

        std::vector<InputMapping> _mappings;

        std::mutex _mu;
    };

}