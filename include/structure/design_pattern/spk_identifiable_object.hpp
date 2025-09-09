#pragma once

#include "structure/spk_safe_pointer.hpp"
#include <mutex>
#include <set>
#include <type_traits>
#include <unordered_map>

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
		static std::set<ID> &_releasedIds()
		{
			static std::set<ID> releasedIds;
			return releasedIds;
		}
		static std::unordered_map<ID, spk::SafePointer<IdentifiedObject<TType>>> &_registry()
		{
			static std::unordered_map<ID, spk::SafePointer<IdentifiedObject<TType>>> registry;
			return registry;
		}
		static std::mutex &_mutex()
		{
			static std::mutex mutex;
			return mutex;
		}

		void _assignNewId()
		{
			std::lock_guard<std::mutex> lock(_mutex());
			if (_releasedIds().empty() == false)
			{
				auto it = _releasedIds().begin();
				_id = *it;
				_releasedIds().erase(it);
			}
			else
			{
				_id = _nextId;
				while (_registry().contains(_id) == true)
				{
					_id++;
				}
			}
			_registry()[_id] = this;
			_nextId = _id + 1;
		}

	public:
		IdentifiedObject()
		{
			static_assert(std::is_base_of<IdentifiedObject<TType>, TType>::value, "TType must inherit from IdentifiedObject<TType>");
			_assignNewId();
		}

		IdentifiedObject(const ID &p_id)
		{
			std::lock_guard<std::mutex> lock(_mutex());
			_id = p_id;
			_registry()[_id] = this;
		}

		IdentifiedObject(const IdentifiedObject &p_other)
		{
			_assignNewId();
		}

		IdentifiedObject &operator=(const IdentifiedObject &p_other)
		{
			// No need to recreate anything as the ID is an intrinsic property of the object
			return *this;
		}

		IdentifiedObject(IdentifiedObject &&p_other) noexcept :
			_id(p_other._id)
		{
			std::lock_guard<std::mutex> lock(_mutex());
			_registry()[_id] = this;
			p_other._id = InvalidID;
		}

		IdentifiedObject &operator=(IdentifiedObject &&p_other) noexcept
		{
			if (this != &p_other)
			{
				std::lock_guard<std::mutex> lock(_mutex());
				if (_id != InvalidID)
				{
					_registry().erase(_id);
					_releasedIds().insert(_id);
				}

				_id = p_other._id;
				_registry()[_id] = this;
				p_other._id = InvalidID;
			}
			return *this;
		}

		~IdentifiedObject()
		{
			std::lock_guard<std::mutex> lock(_mutex());
			_registry().erase(_id);
			_releasedIds().insert(_id);
		}

		ID id() const
		{
			return _id;
		}

		void changeID(const ID &p_newId)
		{
			std::lock_guard<std::mutex> lock(_mutex());

			if (p_newId == _id)
			{
				return;
			}

			auto it = _registry().find(p_newId);
			if (it == _registry().end())
			{
				_registry().erase(_id);

				_releasedIds().erase(_id);
				_releasedIds().erase(p_newId);

				_id = p_newId;

				_registry()[_id] = this;
			}
			else
			{
				spk::SafePointer<IdentifiedObject<TType>> otherPtr = it->second;
				IdentifiedObject<TType> *otherObj = otherPtr.get();

				if (otherObj == this)
				{
					return;
				}

				ID oldId = _id;

				_registry().erase(oldId);
				_registry().erase(p_newId);

				otherObj->_id = oldId;
				this->_id = p_newId;

				_registry()[oldId] = otherPtr;
				_registry()[p_newId] = this;

				_releasedIds().erase(oldId);
				_releasedIds().erase(p_newId);
			}
		}

		static spk::SafePointer<TType> containObjectId(const ID &p_id)
		{
			std::lock_guard<std::mutex> lock(_mutex());
			auto it = _registry().find(p_id);
			return (it != _registry().end()) ? true : false;
		}

		static spk::SafePointer<TType> getObjectById(const ID &p_id)
		{
			if (containObjectId(p_id) == false)
			{
				return (nullptr);
			}

			std::lock_guard<std::mutex> lock(_mutex());
			return (static_cast<TType *>(_registry()[p_id]));
		}
	};
}
