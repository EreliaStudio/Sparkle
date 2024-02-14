#include "input/spk_input_decoder.hpp"

namespace spk
{
    void InputDecoder::set_mappings(const std::vector<InputMapping>& p_mappings)
    {
        std::unique_lock<std::mutex> lk(_mu);

        // Reset everything but subscriptions.
        for (auto& mapping : _mappings)
        {
            mapping.clear();
        }
        for (size_t i = 0; i < Input::max_code; i++)
        {
            _inputToMappings[i].clear();
        }

        for (const InputMapping& mapping : p_mappings)
        {
            // Skip mappings that have not been fully initialized.
            if (!mapping.valid())
            {
                continue;
            }
            // Extends mappings if needed.
            while (mapping.code() > _notifiers.size())
            {
                _notifiers.push_back(std::make_unique<Notifier>());
            }
            while (mapping.code() > _mappings.size())
            {
                _mappings.push_back(InputMapping());
            }

            _inputToMappings[mapping.expects().code()].push_back(mapping.code());
            _mappings[mapping.code()] = mapping;
        }
    }

    void InputDecoder::update(const InputMapping& p_mapping)
    {
        if (!p_mapping.valid())
        {
            return;
        }
        std::unique_lock<std::mutex> lk(_mu);

        // Extends mappings if needed.
        while (p_mapping.code() > _notifiers.size())
        {
            _notifiers.push_back(std::make_unique<Notifier>());
        }
        while (p_mapping.code() > _mappings.size())
        {
            _mappings.push_back(InputMapping());
        }

        // Cleanup old mapping from the input map.
        InputMapping& old = _mappings[p_mapping.code()];
        if (old.valid())
        {
            std::vector<int>& oldMappings = _inputToMappings[old.expects().code()];
            auto codeIt = std::find(oldMappings.begin(), oldMappings.end(), p_mapping.code());
            oldMappings.erase(codeIt);
        }

        // Add new mapping to the input map.
        _inputToMappings[p_mapping.expects().code()].push_back(p_mapping.code());
        _mappings[p_mapping.code()] = p_mapping;
    }

    void InputDecoder::add(const Input& p_input)
    {
        std::unique_lock<std::mutex> lk(_mu);
        for (int mappingCode : _inputToMappings[p_input.code()])
        {
            _notifiers[mappingCode]->notify_all();
        }
    }

    std::unique_ptr<Notifier::Contract> InputDecoder::subscribe(size_t p_mappingCode, const Notifier::Callback& p_callback)
    {
        std::unique_lock<std::mutex> lk(_mu);
        // Extend notifiers if needed
        while (p_mappingCode > _notifiers.size())
        {
            _notifiers.push_back(std::make_unique<Notifier>());
        }
        return _notifiers[p_mappingCode]->subscribe(p_callback);
    }
}