#pragma once

#include <cstdint>
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
	class Skeleton::Part::View
	{
		friend class Model::View;

	private:
		Model *_model = nullptr;
		Part *_part = nullptr;
		const Part *_root = nullptr;
		mutable std::uint64_t _topologyVersion = 0;
		mutable std::optional<spk::Voxel::Geometry> _geometry;
		mutable std::uint64_t _bodyVersion = 0;
		mutable std::map<std::string, std::unique_ptr<View>, std::less<>> _children;
		mutable std::map<std::string, std::unique_ptr<Model::View>, std::less<>> _models;
		mutable std::map<std::string, std::uint64_t, std::less<>> _attachmentVersions;

		[[nodiscard]] bool _attachmentChanged(const std::string &name, std::uint64_t version) const;
		void _dropOutdatedAttachments() const;
		void _addMissingAttachments() const;
		void _sync() const;
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
		[[nodiscard]] View &child(std::string_view jointName);
		[[nodiscard]] const View &child(std::string_view jointName) const;
		[[nodiscard]] Model::View &attachedModel(std::string_view jointName);
		[[nodiscard]] const Model::View &attachedModel(std::string_view jointName) const;
	};

	class Model::View
	{
		friend class Skeleton::Part::View;

	public:
		using Path = Skeleton::Path;

	private:
		Model *_model;
		mutable Skeleton::Part::View _root;
		std::optional<spk::Palette> _palette;

		void _sync() const;

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
		void bind(spk::Palette palette);
		[[nodiscard]] const spk::Palette *palette() const noexcept;
		[[nodiscard]] bool isRenderable() const;
	};
}
