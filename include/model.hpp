#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "container/cached_data.hpp"
#include "math/matrix.hpp"
#include "voxel/volume.hpp"

namespace spk
{
	class Model
	{
		friend class Skeleton;

	public:
		class Skeleton;
		class View;

		struct Version
		{
			std::uint64_t transform = 0;
			std::uint64_t pose = 0;
			std::uint64_t topology = 0;
			std::uint64_t geometry = 0;

			[[nodiscard]] bool operator==(const Version &) const noexcept = default;
		};

	private:
		std::unique_ptr<Skeleton> _skeleton;
		spk::Matrix4x4 _transform = spk::Matrix4x4::identity();
		std::uint64_t _transformVersion = 0;
		mutable spk::CachedData<spk::Matrix4x4> _globalMatrix{[this] { return _calculateGlobalMatrix(); }};

		[[nodiscard]] spk::Matrix4x4 _calculateGlobalMatrix() const;
		void _invalidateGlobalMatrices() noexcept;

	public:
		Model();
		explicit Model(Skeleton skeleton);
		Model(const Model &other);
		Model(Model &&other) noexcept;
		Model &operator=(const Model &other);
		Model &operator=(Model &&other) noexcept;
		~Model();

		[[nodiscard]] Skeleton &skeleton() noexcept;
		[[nodiscard]] const Skeleton &skeleton() const noexcept;
		void setTransform(spk::Matrix4x4 transform);
		[[nodiscard]] const spk::Matrix4x4 &transform() const noexcept;
		[[nodiscard]] const spk::Matrix4x4 &globalMatrix() const;
		[[nodiscard]] Version version() const noexcept;
	};

	class Model::Skeleton
	{
	public:
		class Part;
		class Joint;

		using Path = std::vector<std::string>;

		struct Revision
		{
			std::uint64_t pose = 0;
			std::uint64_t topology = 0;
			std::uint64_t geometry = 0;
		};

	private:
		std::unique_ptr<Part> _root;
		std::shared_ptr<Revision> _revision;
		Model *_ownerModel = nullptr;
		Joint *_parentJoint = nullptr;

		void _bindOwner(Model *owner) noexcept;
		void _bindParentJoint(Joint *parent) noexcept;

	public:
		Skeleton();
		Skeleton(const Skeleton &other);
		Skeleton(Skeleton &&other) noexcept;
		Skeleton &operator=(const Skeleton &other);
		Skeleton &operator=(Skeleton &&other) noexcept;
		~Skeleton();

		[[nodiscard]] Part &root() noexcept;
		[[nodiscard]] const Part &root() const noexcept;
		[[nodiscard]] Joint &joint(const Path &path);
		[[nodiscard]] const Joint &joint(const Path &path) const;
		[[nodiscard]] Joint &joint(std::string_view uniqueName);
		[[nodiscard]] const Joint &joint(std::string_view uniqueName) const;
		[[nodiscard]] Revision revision() const noexcept;
	};

	class Model::Skeleton::Joint
	{
		friend class Part;

	private:
		spk::Matrix4x4 _transform = spk::Matrix4x4::identity();
		spk::Quaternion _rotation = spk::Quaternion::identity();
		std::string _attachment;
		std::unique_ptr<Part> _child;
		std::unique_ptr<Model> _model;
		std::shared_ptr<Revision> _revision;
		Part *_parentPart = nullptr;
		std::uint64_t _attachmentVersion = 0;
		mutable spk::CachedData<spk::Matrix4x4> _globalMatrix{[this] { return _calculateGlobalMatrix(); }};

		[[nodiscard]] spk::Matrix4x4 _calculateGlobalMatrix() const;
		void _bindHierarchy(Part *parent) noexcept;
		void _invalidateGlobalMatrices() noexcept;

		void _bindRevision(std::shared_ptr<Revision> revision);
		void _changed() noexcept;

	public:
		explicit Joint(spk::Matrix4x4 transform = spk::Matrix4x4::identity());
		Joint(const Joint &other);
		Joint(Joint &&other) noexcept;
		Joint &operator=(const Joint &other);
		Joint &operator=(Joint &&other) noexcept;
		~Joint();

		void setTransform(spk::Matrix4x4 transform);
		[[nodiscard]] const spk::Matrix4x4 &transform() const noexcept;
		void setRotation(spk::Quaternion rotation);
		[[nodiscard]] spk::Quaternion rotation() const noexcept;
		[[nodiscard]] const spk::Matrix4x4 &globalMatrix() const;
		void attach(Part child, std::string attachment = "");
		void attach(Model model, std::string attachment = "");
		void detach() noexcept;
		[[nodiscard]] Part *child() noexcept;
		[[nodiscard]] const Part *child() const noexcept;
		[[nodiscard]] Model *model() noexcept;
		[[nodiscard]] const Model *model() const noexcept;
		[[nodiscard]] const std::string &attachment() const noexcept;
		[[nodiscard]] std::uint64_t attachmentVersion() const noexcept;
	};

	class Model::Skeleton::Part
	{
		friend class Model;
		friend class Joint;
		friend class Skeleton;

	public:
		class View;

	private:
		std::optional<spk::Voxel::Volume> _volume;
		std::map<std::string, Joint, std::less<>> _joints;
		std::shared_ptr<Revision> _revision;
		Joint *_parentJoint = nullptr;
		Model *_ownerModel = nullptr;
		std::uint64_t _volumeVersion = 0;
		mutable spk::CachedData<spk::Matrix4x4> _globalMatrix{[this] { return _calculateGlobalMatrix(); }};

		[[nodiscard]] spk::Matrix4x4 _calculateGlobalMatrix() const;
		void _bindHierarchy(Joint *parent, Model *owner) noexcept;
		void _invalidateGlobalMatrices() noexcept;
		void _anchorChanged(Joint *joint) noexcept;

		void _bindRevision(std::shared_ptr<Revision> revision);

	public:
		Part() = default;
		explicit Part(spk::Voxel::Volume volume);
		Part(const Part &other);
		Part(Part &&other) noexcept;
		Part &operator=(const Part &other);
		Part &operator=(Part &&other) noexcept;
		~Part();

		void setVolume(spk::Voxel::Volume volume);
		void clearVolume() noexcept;
		[[nodiscard]] const spk::Voxel::Volume *volume() const noexcept;
		[[nodiscard]] std::uint64_t volumeVersion() const noexcept;
		[[nodiscard]] const spk::Matrix4x4 &globalMatrix() const;
		void addJoint(std::string name, spk::Matrix4x4 transform = spk::Matrix4x4::identity());
		[[nodiscard]] Joint &joint(std::string_view name);
		[[nodiscard]] const Joint &joint(std::string_view name) const;
		[[nodiscard]] const std::map<std::string, Joint, std::less<>> &joints() const noexcept;
	};
}
