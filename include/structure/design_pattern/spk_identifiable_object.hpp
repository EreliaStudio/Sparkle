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
                _id = _nextId;
				while (_registry.contains(_id) == true)
					_id++;
            }
            _registry[_id] = this;
			_nextId = _id + 1;
        }

    public:
        IdentifiedObject()
        {
        	static_assert(std::is_base_of<IdentifiedObject<TType>, TType>::value,
                      "TType must inherit from IdentifiedObject<TType>");
            assignNewId();
        }

        IdentifiedObject(const ID& p_id)
        {
            std::lock_guard<std::mutex> lock(_mutex);
			_id = p_id;
            _registry[_id] = this;
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

		void changeID(const ID& newId)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			if (newId == _id)
				return;

			auto it = _registry.find(newId);
			if (it == _registry.end())
			{
				_registry.erase(_id);

				_releasedIds.erase(_id);
				_releasedIds.erase(newId);

				_id = newId;

				_registry[_id] = this;
			}
			else
			{
				spk::SafePointer<IdentifiedObject<TType>> otherPtr = it->second;
				IdentifiedObject<TType>* otherObj = otherPtr.get();

				if (otherObj == this)
					return;

				ID oldId = _id;

				_registry.erase(oldId);
				_registry.erase(newId);

				otherObj->_id = oldId;
				this->_id = newId;

				_registry[oldId] = otherPtr;
				_registry[newId] = this;

				_releasedIds.erase(oldId);
				_releasedIds.erase(newId);
			}
		}

		static spk::SafePointer<TType> ContainObjectID(const ID& id)
		{
			std::lock_guard<std::mutex> lock(_mutex);
            auto it = _registry.find(id);
            return (it != _registry.end()) ? true : false;
		}

        static spk::SafePointer<TType> GetObjectByID(const ID& id)
        {
			if (ContainObjectID(id) == false)
			{
				return (nullptr);
			}

            std::lock_guard<std::mutex> lock(_mutex);
            return (static_cast<TType*>(_registry[id]));
        }
    };
}
