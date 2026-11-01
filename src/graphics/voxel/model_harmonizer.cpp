#include "voxel/model.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <utility>

#include "exception.hpp"
#include "voxel/palette.hpp"

namespace spk::Voxel
{
	namespace
	{
		[[nodiscard]] bool sameMaterial(const spk::Material &first, const spk::Material &second)
		{
			if (first.textures().size() != second.textures().size())
			{
				return false;
			}
			for (const auto &[role, binding] : first.textures())
			{
				const auto *other = second.find(role);
				if (!other || binding.texture.identifier() != other->texture.identifier() ||
					binding.section != other->section)
				{
					return false;
				}
			}
			return true;
		}

		void reserveNames(const Model &model, std::vector<std::string> &names)
		{
			for (const auto &slot : model.materialSlots())
			{
				names.push_back(slot.name);
			}
			for (const auto &[name, anchor] : model.anchors())
			{
				if (anchor.model())
				{
					reserveNames(*anchor.model(), names);
				}
			}
		}

		[[nodiscard]] std::string availableName(
			const std::string &name,
			const std::vector<std::string> &reserved)
		{
			std::size_t suffix = 1;
			std::string candidate;
			do
			{
				candidate = name + "#" + std::to_string(suffix++);
			} while (std::find(reserved.begin(), reserved.end(), candidate) != reserved.end());
			return candidate;
		}
	}

	void Model::Harmonizer::_collect(
		const View &view,
		const Model &model,
		Path &path,
		Context &context)
	{
		const auto it = view._palettes.find(path);
		if (it == view._palettes.end() && !model.materialSlots().empty())
		{
			throw spk::Exception("Voxel Harmonizer: missing source palette");
		}
		auto &mapping = context.remappings[path];
		for (const auto &slot : model.materialSlots())
		{
			const auto *material = it->second.find(slot.name);
			std::size_t index = 0;
			for (; index < context.names.size(); ++index)
			{
				if (context.origins[index] == slot.name && sameMaterial(context.materials[index], *material))
				{
					break;
				}
			}
			if (index == context.names.size())
			{
				const bool conflict = std::find(context.names.begin(), context.names.end(), slot.name) != context.names.end();
				auto name = conflict ? availableName(slot.name, context.reservedNames) : slot.name;
				context.reservedNames.push_back(name);
				context.names.push_back(std::move(name));
				context.origins.push_back(slot.name);
				context.materials.push_back(*material);
			}
			if (index > std::numeric_limits<std::uint32_t>::max())
			{
				throw spk::Exception("Voxel Harmonizer: too many materials");
			}
			mapping.push_back(static_cast<std::uint32_t>(index));
		}
		for (const auto &[name, anchor] : model.anchors())
		{
			if (anchor.model())
			{
				path.push_back(name);
				_collect(view, *anchor.model(), path, context);
				path.pop_back();
			}
		}
	}

	void Model::Harmonizer::_rewrite(Model &model, Path &path, const Context &context)
	{
		std::vector<Definition::Slot> slots;
		slots.reserve(context.names.size());
		for (const auto &name : context.names)
		{
			slots.push_back({name, 1.0f});
		}
		const auto &mapping = context.remappings.at(path);
		for (std::size_t i = 0; i < mapping.size(); ++i)
		{
			slots[mapping[i]].opacity = model.materialSlots()[i].opacity;
		}
		model._mesh = model._mesh.remap(mapping, std::move(slots));
		for (const auto &[name, anchor] : model.anchors())
		{
			if (model.anchor(name).model())
			{
				path.push_back(name);
				_rewrite(*model.anchor(name).model(), path, context);
				path.pop_back();
			}
		}
	}

	Model::Harmonizer::Result Model::Harmonizer::process(const View &view) const
	{
		Context context;
		reserveNames(view.model(), context.reservedNames);
		Path path;
		_collect(view, view.model(), path, context);
		std::vector<std::size_t> order(context.names.size());
		std::iota(order.begin(), order.end(), 0);
		std::sort(order.begin(), order.end(), [&](auto a, auto b) {
			return context.names[a] < context.names[b];
		});
		std::vector<std::uint32_t> oldToNew(order.size());
		std::vector<std::string> sortedNames;
		std::vector<spk::Material> sortedMaterials;
		for (std::size_t i = 0; i < order.size(); ++i)
		{
			oldToNew[order[i]] = static_cast<std::uint32_t>(i);
			sortedNames.push_back(context.names[order[i]]);
			sortedMaterials.push_back(context.materials[order[i]]);
		}
		context.names = std::move(sortedNames);
		context.materials = std::move(sortedMaterials);
		for (auto &[path, mapping] : context.remappings)
		{
			for (auto &index : mapping)
			{
				index = oldToNew[index];
			}
		}
		spk::Voxel::Palette::Builder builder;
		for (std::size_t i = 0; i < context.names.size(); ++i)
		{
			builder.setMaterial(context.names[i], context.materials[i]);
		}
		Model copy = view.model();
		_rewrite(copy, path, context);
		return {std::move(copy), builder.build(), MaterialLayout(std::move(context.names))};
	}
}
