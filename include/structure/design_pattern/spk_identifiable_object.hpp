#pragma once

#include <set>
#include <unordered_map>
#include <type_traits>
#include <mutex>
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
    template <typename TType>
    class IdentifiedObject
    {
        static_assert(std::is_base_of<IdentifiedObject<TType>, TType>::value,
                      "TType must inherit from IdentifiedObject<TType>");
                      
    public:
        using ID = long;
        static inline const ID InvalidID = -1;

    private:
        ID _id;

        static inline ID _nextId = 0;
        static inline std::set<ID> _releasedIds;
        static inline std::unordered_map<ID, spk::SafePointer<IdentifiedObject<TType>>> _registry;
        static inline std::mutex _mutex;
    
        void assignNewId()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_releasedIds.empty())
            {
                auto it = _releasedIds.begin();
                _id = *it;
                _releasedIds.erase(it);
            }
            else
            {
                _id = _nextId++;
            }
            _registry[_id] = this;
        }

    public:
        IdentifiedObject()
        {
            assignNewId();
        }
        
        IdentifiedObject(const IdentifiedObject& other)
        {
            assignNewId();
        }

        IdentifiedObject& operator=(const IdentifiedObject& other)
        {
            // No need to recreate anything as the ID is an intrinsic property of the object
            return *this;
        }

        IdentifiedObject(IdentifiedObject&& other) noexcept
            : _id(other._id)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _registry[_id] = this;
            other._id = InvalidID;
        }

        IdentifiedObject& operator=(IdentifiedObject&& other) noexcept
        {
            if (this != &other)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (_id != InvalidID)
                {
                    _registry.erase(_id);
                    _releasedIds.insert(_id);
                }

                _id = other._id;
                _registry[_id] = this;
                other._id = InvalidID;
            }
            return *this;
        }

        ~IdentifiedObject()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _registry.erase(_id);
            _releasedIds.insert(_id);
        }

        ID id() const
        {
            return _id;
        }

        static spk::SafePointer<TType> GetObjectByID(const ID& id)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _registry.find(id);
            return (it != _registry.end()) ? static_cast<TType*>(it->second.get()) : nullptr;
        }
    };
}
