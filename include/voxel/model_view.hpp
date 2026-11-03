#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "model.hpp"
#include "voxel/geometry.hpp"
#include "voxel/palette.hpp"

namespace spk
{
	class Model::Skeleton::Part::View
	{
		friend class Model::View;

	public:
		enum class PalettePropagation
		{
			Self,
			Recursive
		};

	private:
		Model *_model = nullptr;
		Part *_part = nullptr;
		const Part *_root = nullptr;
		mutable std::uint64_t _topologyVersion = 0;
		mutable std::optional<spk::Voxel::Geometry> _geometry;
		mutable std::uint64_t _volumeVersion = 0;
		std::optional<spk::Palette> _palette;
		mutable std::map<std::string, std::unique_ptr<View>, std::less<>> _children;
		mutable std::map<std::string, std::unique_ptr<Model::View>, std::less<>> _models;
		mutable std::map<std::string, std::uint64_t, std::less<>> _attachmentVersions;

		[[nodiscard]] bool _attachmentChanged(std::string_view name, std::uint64_t version) const;
		void _dropOutdatedAttachments() const;
		void _addMissingAttachments() const;
		void _sync() const;
		void _validatePalette(const spk::Palette &palette, PalettePropagation propagation) const;
		void _applyPalette(const spk::Palette &palette, PalettePropagation propagation);
		using DrawCallback = std::function<void(const Part &, const spk::Voxel::Geometry &, const spk::Palette &, const spk::Matrix4x4 &)>;
		void _visitJoint(std::string_view name, const Joint &joint, const DrawCallback &callback, spk::Matrix4x4 transform) const;
		void _visit(const DrawCallback &callback, spk::Matrix4x4 transform) const;
		void _collectMatches(std::string_view name, std::vector<View *> &matches);

	public:
		View(Model &model, Part &part);
		View(const View &other);
		View(View &&other) noexcept;
		View &operator=(const View &other);
		View &operator=(View &&other) noexcept;
		~View();

		[[nodiscard]] const Part &part() const;
		[[nodiscard]] const spk::Voxel::Geometry *geometry() const;
		[[nodiscard]] const spk::Palette *palette() const noexcept;
		void bind(spk::Palette palette, PalettePropagation propagation = PalettePropagation::Self);
		[[nodiscard]] View &child(std::string_view jointName);
		[[nodiscard]] const View &child(std::string_view jointName) const;
		[[nodiscard]] Model::View &attachedModel(std::string_view jointName);
		[[nodiscard]] const Model::View &attachedModel(std::string_view jointName) const;
	};

	class Model::View
	{
		friend class Skeleton::Part::View;

	public:
		using PalettePropagation = Skeleton::Part::View::PalettePropagation;
		using Path = Skeleton::Path;
		using DrawCallback = std::function<void(
			const Skeleton::Part &, const spk::Voxel::Geometry &, const spk::Palette &, const spk::Matrix4x4 &)>;

	private:
		Model *_model;
		mutable Skeleton::Part::View _root;

		void _sync() const;
		void _visit(const DrawCallback &callback, spk::Matrix4x4 parent) const;

	public:
		explicit View(Model &model);
		View(const View &other) = default;
		View(View &&other) noexcept = default;
		View &operator=(const View &other) = default;
		View &operator=(View &&other) noexcept = default;
		~View() = default;

		[[nodiscard]] const Model &model() const noexcept;
		[[nodiscard]] Skeleton::Part::View &root();
		[[nodiscard]] const Skeleton::Part::View &root() const;
		[[nodiscard]] Skeleton::Part::View &part(const Path &path);
		[[nodiscard]] const Skeleton::Part::View &part(const Path &path) const;
		[[nodiscard]] Skeleton::Part::View &part(std::string_view uniqueJointName);
		[[nodiscard]] const Skeleton::Part::View &part(std::string_view uniqueJointName) const;
		void bind(spk::Palette palette, PalettePropagation propagation = PalettePropagation::Self);
		void harmonize();
		[[nodiscard]] bool isRenderable() const;
		void visit(const DrawCallback &callback) const;
	};
}
