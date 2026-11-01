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

	class View final : public spk::VersionedTrait
	{
	public:
		struct Part
		{
			PartID id;
			std::string name;
			std::optional<PartID> parent;
			spk::Matrix4x4 localTransform;
			std::vector<Model> models;
		};

		struct InsertResult
		{
			PartID anchor;
			std::map<PartID, PartID> partIDs;
		};

	private:
		friend struct ViewTestAccess;

		std::map<PartID, Part> _parts;
		std::optional<spk::Palette> _palette;
		PartID _nextID = 1;

		[[nodiscard]] Part &_editablePart(PartID id);
		[[nodiscard]] static PartID _allocate(PartID &next);
		static void _validateTransform(const spk::Matrix4x4 &transform);
		void _validateModels(const std::vector<Model> &models) const;
		void _appendSubtree(PartID id, std::vector<PartID> &result) const;
		[[nodiscard]] std::vector<PartID> _validatedInsertOrder(
			std::optional<PartID> parent, const View &source, const spk::Matrix4x4 &transform) const;
		static void _stageImportedParts(
			std::map<PartID, Part> &prepared,
			PartID &next,
			const View &source,
			const std::vector<PartID> &order,
			InsertResult &result);

	public:
		View();
		explicit View(Model model);
		View(Model model, spk::Palette palette);
		View(const View &other);
		View(View &&other) noexcept = default;
		View &operator=(const View &other);

		[[nodiscard]] const spk::Palette *palette() const noexcept;
		void bind(spk::Palette palette);
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] bool contains(PartID id) const noexcept;
		[[nodiscard]] const Part &part(PartID id) const;
		[[nodiscard]] std::vector<PartID> roots() const;
		[[nodiscard]] std::vector<PartID> children(PartID id) const;
		[[nodiscard]] std::vector<PartID> partIDs() const;
		[[nodiscard]] spk::Matrix4x4 accumulatedTransform(PartID id) const;
		[[nodiscard]] std::vector<std::uint32_t> materialIndices(PartID id, std::size_t modelIndex) const;

		[[nodiscard]] PartID addPart(
			std::string name,
			std::optional<PartID> parent = std::nullopt,
			std::vector<Model> models = {},
			spk::Matrix4x4 localTransform = spk::Matrix4x4::identity());
		void addModel(PartID id, Model model);
		void setModel(PartID id, std::size_t index, Model model);
		void removeModel(PartID id, std::size_t index);
		void setName(PartID id, std::string name);
		void setLocalTransform(PartID id, spk::Matrix4x4 transform);
		void setParent(PartID id, std::optional<PartID> parent);
		void removePart(PartID id);

		[[nodiscard]] InsertResult insertView(
			std::optional<PartID> parent,
			const View &source,
			spk::Matrix4x4 attachmentTransform = spk::Matrix4x4::identity(),
			std::string anchorName = "attachment");
	};
}
