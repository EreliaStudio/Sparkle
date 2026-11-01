#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "design_pattern/trait/versioned_trait.hpp"
#include "math/matrix.hpp"
#include "voxel/model.hpp"

namespace spk::Voxel
{
	using PartID = std::uint64_t;

	class Assembly final : public spk::VersionedTrait
	{
	public:
		struct Part
		{
			PartID id;
			std::string name;
			std::optional<PartID> parent;
			spk::Matrix4x4 localTransform;
			std::optional<Model> model;
		};

		struct InsertResult
		{
			PartID anchor;
			std::map<PartID, PartID> partIDs;
		};

	private:
		friend struct AssemblyTestAccess;

		std::map<PartID, Part> _parts;
		std::map<PartID, spk::Palette> _paletteInputs;
		spk::Palette _palette;
		PartID _nextID = 1;
		bool _paletteExplicitlyBound = false;

		[[nodiscard]] Part &_editablePart(PartID id);
		[[nodiscard]] static PartID _allocate(PartID &next);
		static void _validateTransform(const spk::Matrix4x4 &transform);
		void _appendSubtree(PartID id, std::vector<PartID> &result) const;
		[[nodiscard]] static bool _sameMaterial(const spk::Material &left, const spk::Material &right);
		static void _bindModels(std::map<PartID, Part> &parts, const spk::Palette &palette);
		[[nodiscard]] spk::Palette _mergePalette(
			std::map<PartID, Part> &parts,
			const std::map<PartID, spk::Palette> &inputs);
		void _commit(
			std::map<PartID, Part> &parts,
			std::map<PartID, spk::Palette> &inputs,
			spk::Palette palette,
			PartID next) noexcept;
		[[nodiscard]] std::vector<PartID> _validatedInsertOrder(
			std::optional<PartID> parent, const Assembly &source, const spk::Matrix4x4 &transform) const;
		void _stageImportedParts(
			std::map<PartID, Part> &prepared,
			std::map<PartID, spk::Palette> &inputs,
			PartID &next,
			const Assembly &source,
			const std::vector<PartID> &order,
			InsertResult &result);

	public:
		Assembly();
		Assembly(const Assembly &other);
		Assembly(Assembly &&other) noexcept = default;
		Assembly &operator=(const Assembly &other);

		[[nodiscard]] const spk::Palette &palette() const noexcept;
		void bind(spk::Palette palette);
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] bool contains(PartID id) const noexcept;
		[[nodiscard]] const Part &part(PartID id) const;
		[[nodiscard]] std::vector<PartID> roots() const;
		[[nodiscard]] std::vector<PartID> children(PartID id) const;
		[[nodiscard]] std::vector<PartID> partIDs() const;
		[[nodiscard]] spk::Matrix4x4 accumulatedTransform(PartID id) const;

		[[nodiscard]] PartID addPart(
			std::string name,
			std::optional<PartID> parent = std::nullopt,
			std::optional<Model> model = std::nullopt,
			spk::Matrix4x4 localTransform = spk::Matrix4x4::identity());
		void setName(PartID id, std::string name);
		void setModel(PartID id, std::optional<Model> model);
		void setLocalTransform(PartID id, spk::Matrix4x4 transform);
		void setParent(PartID id, std::optional<PartID> parent);
		void removePart(PartID id);

		[[nodiscard]] InsertResult insertAssembly(
			std::optional<PartID> parent,
			const Assembly &source,
			spk::Matrix4x4 attachmentTransform = spk::Matrix4x4::identity(),
			std::string anchorName = "attachment");
	};
}
