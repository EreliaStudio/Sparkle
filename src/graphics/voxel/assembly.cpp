#include "voxel/assembly.hpp"

#include <cmath>
#include <limits>
#include <utility>

#include "exception.hpp"
#include "voxel/palette.hpp"

namespace spk::Voxel
{
	Assembly::Assembly()
	{
		invalidate();
	}

	Assembly::Assembly(const Assembly &other) :
		_parts(other._parts),
		_paletteInputs(other._paletteInputs),
		_palette(other._palette),
		_nextID(other._nextID)
	{
		invalidate();
	}

	Assembly &Assembly::operator=(const Assembly &other)
	{
		if (this != &other)
		{
			auto replacement = other._parts;
			auto inputs = other._paletteInputs;
			auto palette = other._palette;
			_commit(replacement, inputs, std::move(palette), other._nextID);
			resetAfterCopy();
		}
		return *this;
	}

	const spk::Palette &Assembly::palette() const noexcept
	{
		return _palette;
	}

	std::size_t Assembly::size() const noexcept
	{
		return _parts.size();
	}

	bool Assembly::empty() const noexcept
	{
		return _parts.empty();
	}

	bool Assembly::contains(PartID id) const noexcept
	{
		return _parts.contains(id);
	}

	const Assembly::Part &Assembly::part(PartID id) const
	{
		const auto it = _parts.find(id);
		if (it == _parts.end())
		{
			throw spk::Exception("Voxel Assembly: unknown PartID");
		}
		return it->second;
	}

	Assembly::Part &Assembly::_editablePart(PartID id)
	{
		return const_cast<Part &>(std::as_const(*this).part(id));
	}

	PartID Assembly::_allocate(PartID &next)
	{
		if (next == 0)
		{
			throw spk::Exception("Voxel Assembly: PartID space exhausted");
		}
		return std::exchange(next, next == std::numeric_limits<PartID>::max() ? 0 : next + 1);
	}

	void Assembly::_validateTransform(const spk::Matrix4x4 &transform)
	{
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				if (!std::isfinite(transform[column][row]))
				{
					throw spk::Exception("Voxel Assembly: non-finite transform");
				}
			}
		}
	}

	bool Assembly::_sameMaterial(const spk::Material &left, const spk::Material &right)
	{
		if (left.textures().size() != right.textures().size())
		{
			return false;
		}
		for (const auto &[name, binding] : left.textures())
		{
			const auto *other = right.find(name);
			if (other == nullptr || binding.texture.identifier() != other->texture.identifier() ||
				binding.section != other->section)
			{
				return false;
			}
		}
		return true;
	}

	spk::Palette Assembly::_mergePalette(
		std::map<PartID, Part> &parts,
		const std::map<PartID, spk::Palette> &inputs)
	{
		spk::Voxel::Palette::Builder builder;
		spk::Palette::Materials seen;
		for (const auto &[id, source] : inputs)
		{
			for (const auto &[name, material] : source.materials())
			{
				const auto existing = seen.find(name);
				if (existing != seen.end())
				{
					if (!_sameMaterial(existing->second, material))
					{
						throw spk::Exception("Voxel Assembly: conflicting material slot '" + name + "'");
					}
					continue;
				}
				builder.setMaterial(name, material);
				seen.emplace(name, material);
			}
		}
		auto merged = builder.build();
		for (auto &[id, part] : parts)
		{
			if (part.model.has_value())
			{
				part.model->bind(merged);
			}
		}
		return merged;
	}

	void Assembly::_commit(
		std::map<PartID, Part> &parts,
		std::map<PartID, spk::Palette> &inputs,
		spk::Palette palette,
		PartID next) noexcept
	{
		_parts.swap(parts);
		_paletteInputs.swap(inputs);
		_palette = std::move(palette);
		_nextID = next;
	}

	std::vector<PartID> Assembly::roots() const
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

	std::vector<PartID> Assembly::children(PartID id) const
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

	void Assembly::_appendSubtree(PartID id, std::vector<PartID> &result) const
	{
		result.push_back(id);
		for (const PartID child : children(id))
		{
			_appendSubtree(child, result);
		}
	}

	std::vector<PartID> Assembly::partIDs() const
	{
		std::vector<PartID> result;
		result.reserve(_parts.size());
		for (const PartID root : roots())
		{
			_appendSubtree(root, result);
		}
		return result;
	}

	spk::Matrix4x4 Assembly::accumulatedTransform(PartID id) const
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

	PartID Assembly::addPart(
		std::string name,
		std::optional<PartID> parent,
		std::optional<Model> model,
		spk::Matrix4x4 localTransform)
	{
		if (parent.has_value())
		{
			(void)part(*parent);
		}
		_validateTransform(localTransform);
		if (model.has_value() && model->palette() == nullptr)
		{
			throw spk::Exception("Voxel Assembly: Model must have a bound palette");
		}
		PartID next = _nextID;
		const PartID id = _allocate(next);
		auto prepared = _parts;
		auto inputs = _paletteInputs;
		prepared.emplace(id, Part{id, std::move(name), parent, localTransform, std::move(model)});
		if (prepared.at(id).model.has_value())
		{
			inputs.emplace(id, *prepared.at(id).model->palette());
		}
		auto merged = _mergePalette(prepared, inputs);
		_commit(prepared, inputs, std::move(merged), next);
		return id;
	}

	void Assembly::setName(PartID id, std::string name)
	{
		_editablePart(id).name.swap(name);
	}

	void Assembly::setModel(PartID id, std::optional<Model> model)
	{
		(void)part(id);
		if (model.has_value() && model->palette() == nullptr)
		{
			throw spk::Exception("Voxel Assembly: Model must have a bound palette");
		}
		auto prepared = _parts;
		auto inputs = _paletteInputs;
		prepared.at(id).model = std::move(model);
		if (prepared.at(id).model.has_value())
		{
			inputs.insert_or_assign(id, *prepared.at(id).model->palette());
		}
		else
		{
			inputs.erase(id);
		}
		auto merged = _mergePalette(prepared, inputs);
		_commit(prepared, inputs, std::move(merged), _nextID);
	}

	void Assembly::setLocalTransform(PartID id, spk::Matrix4x4 transform)
	{
		Part &current = _editablePart(id);
		_validateTransform(transform);
		current.localTransform = transform;
	}

	void Assembly::setParent(PartID id, std::optional<PartID> parent)
	{
		Part &current = _editablePart(id);
		for (auto cursor = parent; cursor.has_value(); cursor = part(*cursor).parent)
		{
			if (*cursor == id)
			{
				throw spk::Exception("Voxel Assembly: cyclic parent relationship");
			}
		}
		current.parent = parent;
	}

	void Assembly::removePart(PartID id)
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
		auto prepared = _parts;
		auto inputs = _paletteInputs;
		for (const PartID candidate : removed)
		{
			prepared.erase(candidate);
			inputs.erase(candidate);
		}
		auto merged = _mergePalette(prepared, inputs);
		_commit(prepared, inputs, std::move(merged), _nextID);
	}

	std::vector<PartID> Assembly::_validatedInsertOrder(
		std::optional<PartID> parent, const Assembly &source, const spk::Matrix4x4 &transform) const
	{
		if (this == &source)
		{
			throw spk::Exception("Voxel Assembly: cannot insert itself");
		}
		if (parent.has_value())
		{
			(void)part(*parent);
		}
		_validateTransform(transform);
		auto order = source.partIDs();
		if (order.size() != source.size())
		{
			throw spk::Exception("Voxel Assembly: invalid source hierarchy");
		}
		for (const PartID id : order)
		{
			_validateTransform(source.part(id).localTransform);
		}
		return order;
	}

	void Assembly::_stageImportedParts(
		std::map<PartID, Part> &prepared,
		std::map<PartID, spk::Palette> &inputs,
		PartID &next,
		const Assembly &source,
		const std::vector<PartID> &order,
		InsertResult &result)
	{
		for (const PartID id : order)
		{
			const Part &part = source.part(id);
			const PartID newID = _allocate(next);
			const PartID newParent = part.parent.has_value() ? result.partIDs.at(*part.parent) : result.anchor;
			prepared.emplace(newID, Part{newID, part.name, newParent, part.localTransform, part.model});
			if (part.model.has_value())
			{
				inputs.emplace(newID, source._paletteInputs.at(id));
			}
			result.partIDs.emplace(id, newID);
		}
	}

	Assembly::InsertResult Assembly::insertAssembly(
		std::optional<PartID> parent,
		const Assembly &source,
		spk::Matrix4x4 attachmentTransform,
		std::string anchorName)
	{
		const auto order = _validatedInsertOrder(parent, source, attachmentTransform);
		auto prepared = _parts;
		auto inputs = _paletteInputs;
		PartID next = _nextID;
		InsertResult result{_allocate(next), {}};
		prepared.emplace(result.anchor, Part{result.anchor, std::move(anchorName), parent, attachmentTransform, std::nullopt});
		_stageImportedParts(prepared, inputs, next, source, order, result);
		auto merged = _mergePalette(prepared, inputs);
		_commit(prepared, inputs, std::move(merged), next);
		return result;
	}
}
