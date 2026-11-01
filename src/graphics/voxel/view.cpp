#include "voxel/view.hpp"

#include <cmath>
#include <iterator>
#include <limits>
#include <utility>

#include "exception.hpp"
#include "voxel/palette.hpp"

namespace spk::Voxel
{
	View::View()
	{
		invalidate();
	}

	View::View(Model model) :
		View()
	{
		(void)addPart("model", std::nullopt, {std::move(model)});
	}

	View::View(Model model, spk::Palette palette) :
		View(std::move(model))
	{
		bind(std::move(palette));
	}

	View::View(const View &other) :
		_parts(other._parts),
		_palette(other._palette),
		_nextID(other._nextID)
	{
		invalidate();
	}

	View &View::operator=(const View &other)
	{
		if (this != &other)
		{
			auto prepared = other._parts;
			_parts.swap(prepared);
			_palette = other._palette;
			_nextID = other._nextID;
			resetAfterCopy();
		}
		return *this;
	}

	const spk::Palette *View::palette() const noexcept
	{
		return _palette ? &*_palette : nullptr;
	}

	void View::bind(spk::Palette palette)
	{
		(void)spk::Voxel::Palette::Builder(palette);
		std::string missing;
		for (const auto &[id, part] : _parts)
		{
			for (const Model &model : part.models)
			{
				for (const auto &slot : model.materialSlots())
				{
					if (palette.find(slot.name) == nullptr && missing.find("'" + slot.name + "'") == std::string::npos)
					{
						if (!missing.empty())
						{
							missing += ", ";
						}
						missing += "'" + slot.name + "'";
					}
				}
			}
		}
		if (!missing.empty())
		{
			throw spk::Exception("Voxel View: palette is missing material slots: " + missing);
		}
		_palette = std::move(palette);
	}

	std::vector<std::uint32_t> View::materialIndices(PartID id, std::size_t modelIndex) const
	{
		const auto &models = part(id).models;
		if (modelIndex >= models.size())
		{
			throw spk::Exception("Voxel View: unknown Model index");
		}
		if (!_palette)
		{
			throw spk::Exception("Voxel View: no palette is bound");
		}
		std::vector<std::uint32_t> result;
		result.reserve(models[modelIndex].materialSlots().size());
		const auto &materials = _palette->materials();
		for (const auto &slot : models[modelIndex].materialSlots())
		{
			const auto it = materials.find(slot.name);
			if (it == materials.end())
			{
				throw spk::Exception("Voxel View: material slot not found: " + slot.name);
			}
			const auto index = std::distance(materials.begin(), it);
			if (static_cast<std::uint64_t>(index) > std::numeric_limits<std::uint32_t>::max())
			{
				throw spk::Exception("Voxel View: material index overflow");
			}
			result.push_back(static_cast<std::uint32_t>(index));
		}
		return result;
	}

	std::size_t View::size() const noexcept
	{
		return _parts.size();
	}

	bool View::empty() const noexcept
	{
		return _parts.empty();
	}

	bool View::contains(PartID id) const noexcept
	{
		return _parts.contains(id);
	}

	const View::Part &View::part(PartID id) const
	{
		const auto it = _parts.find(id);
		if (it == _parts.end())
		{
			throw spk::Exception("Voxel View: unknown PartID");
		}
		return it->second;
	}

	View::Part &View::_editablePart(PartID id)
	{
		return const_cast<Part &>(std::as_const(*this).part(id));
	}

	PartID View::_allocate(PartID &next)
	{
		if (next == 0)
		{
			throw spk::Exception("Voxel View: PartID space exhausted");
		}
		return std::exchange(next, next == std::numeric_limits<PartID>::max() ? 0 : next + 1);
	}

	void View::_validateTransform(const spk::Matrix4x4 &transform)
	{
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				if (!std::isfinite(transform[column][row]))
				{
					throw spk::Exception("Voxel View: non-finite transform");
				}
			}
		}
	}

	void View::_validateModels(const std::vector<Model> &models) const
	{
		if (_palette.has_value())
		{
			for (const auto &model : models)
			{
				if (!model.isCompatible(*_palette))
				{
					throw spk::Exception("Voxel View: Model requires slots absent from the View palette");
				}
			}
		}
	}

	std::vector<PartID> View::roots() const
	{
		std::vector<PartID> result;
		for (const auto &[id, current] : _parts)
		{
			if (!current.parent.has_value())
			{
				result.push_back(id);
			}
		}
		return result;
	}

	std::vector<PartID> View::children(PartID id) const
	{
		(void)part(id);
		std::vector<PartID> result;
		for (const auto &[childID, current] : _parts)
		{
			if (current.parent == id)
			{
				result.push_back(childID);
			}
		}
		return result;
	}

	void View::_appendSubtree(PartID id, std::vector<PartID> &result) const
	{
		result.push_back(id);
		for (const PartID child : children(id))
		{
			_appendSubtree(child, result);
		}
	}

	std::vector<PartID> View::partIDs() const
	{
		std::vector<PartID> result;
		result.reserve(_parts.size());
		for (const PartID root : roots())
		{
			_appendSubtree(root, result);
		}
		return result;
	}

	spk::Matrix4x4 View::accumulatedTransform(PartID id) const
	{
		std::vector<PartID> ancestry;
		for (std::optional<PartID> current = id; current.has_value(); current = part(*current).parent)
		{
			ancestry.push_back(*current);
		}
		auto result = spk::Matrix4x4::identity();
		for (auto it = ancestry.rbegin(); it != ancestry.rend(); ++it)
		{
			result = result * part(*it).localTransform;
		}
		return result;
	}

	PartID View::addPart(
		std::string name,
		std::optional<PartID> parent,
		std::vector<Model> models,
		spk::Matrix4x4 localTransform)
	{
		if (parent.has_value())
		{
			(void)part(*parent);
		}
		_validateTransform(localTransform);
		_validateModels(models);
		PartID next = _nextID;
		const PartID id = _allocate(next);
		_parts.emplace(id, Part{id, std::move(name), parent, localTransform, std::move(models)});
		_nextID = next;
		return id;
	}

	void View::addModel(PartID id, Model model)
	{
		_validateModels({model});
		_editablePart(id).models.push_back(std::move(model));
	}

	void View::setModel(PartID id, std::size_t index, Model model)
	{
		_validateModels({model});
		auto &models = _editablePart(id).models;
		if (index >= models.size())
		{
			throw spk::Exception("Voxel View: unknown Model index");
		}
		models[index] = std::move(model);
	}

	void View::removeModel(PartID id, std::size_t index)
	{
		auto &models = _editablePart(id).models;
		if (index >= models.size())
		{
			throw spk::Exception("Voxel View: unknown Model index");
		}
		models.erase(models.begin() + static_cast<std::ptrdiff_t>(index));
	}

	void View::setName(PartID id, std::string name)
	{
		_editablePart(id).name.swap(name);
	}

	void View::setLocalTransform(PartID id, spk::Matrix4x4 transform)
	{
		Part &current = _editablePart(id);
		_validateTransform(transform);
		current.localTransform = transform;
	}

	void View::setParent(PartID id, std::optional<PartID> parent)
	{
		Part &current = _editablePart(id);
		for (auto cursor = parent; cursor.has_value(); cursor = part(*cursor).parent)
		{
			if (*cursor == id)
			{
				throw spk::Exception("Voxel View: cyclic parent relationship");
			}
		}
		current.parent = parent;
	}

	void View::removePart(PartID id)
	{
		(void)part(id);
		std::vector<PartID> removed;
		for (const auto &[candidate, current] : _parts)
		{
			for (auto cursor = std::optional<PartID>(candidate); cursor.has_value(); cursor = part(*cursor).parent)
			{
				if (*cursor == id)
				{
					removed.push_back(candidate);
					break;
				}
			}
		}
		for (const PartID candidate : removed)
		{
			_parts.erase(candidate);
		}
	}

	std::vector<PartID> View::_validatedInsertOrder(
		std::optional<PartID> parent, const View &source, const spk::Matrix4x4 &transform) const
	{
		if (this == &source)
		{
			throw spk::Exception("Voxel View: cannot insert itself");
		}
		if (parent.has_value())
		{
			(void)part(*parent);
		}
		_validateTransform(transform);
		auto order = source.partIDs();
		if (order.size() != source.size())
		{
			throw spk::Exception("Voxel View: invalid source hierarchy");
		}
		for (const PartID id : order)
		{
			_validateTransform(source.part(id).localTransform);
			_validateModels(source.part(id).models);
		}
		return order;
	}

	void View::_stageImportedParts(
		std::map<PartID, Part> &prepared,
		PartID &next,
		const View &source,
		const std::vector<PartID> &order,
		InsertResult &result)
	{
		for (const PartID id : order)
		{
			const Part &part = source.part(id);
			const PartID newID = _allocate(next);
			const PartID newParent = part.parent.has_value() ? result.partIDs.at(*part.parent) : result.anchor;
			prepared.emplace(newID, Part{newID, part.name, newParent, part.localTransform, part.models});
			result.partIDs.emplace(id, newID);
		}
	}

	View::InsertResult View::insertView(
		std::optional<PartID> parent,
		const View &source,
		spk::Matrix4x4 attachmentTransform,
		std::string anchorName)
	{
		const auto order = _validatedInsertOrder(parent, source, attachmentTransform);
		auto prepared = _parts;
		PartID next = _nextID;
		InsertResult result{_allocate(next), {}};
		prepared.emplace(result.anchor, Part{result.anchor, std::move(anchorName), parent, attachmentTransform, {}});
		_stageImportedParts(prepared, next, source, order, result);
		_parts.swap(prepared);
		_nextID = next;
		return result;
	}
}
