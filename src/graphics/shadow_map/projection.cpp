#include "shadow_map/projection.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

#include "exception.hpp"
#include "math/vector4.hpp"

namespace spk::ShadowMap
{
	namespace
	{
		[[nodiscard]] Vector3 normalizedDirection(const Vector3 &direction)
		{
			const double squared = direction.dot(direction);
			if (!std::isfinite(squared) || squared < 1.0e-12)
			{
				throw spk::Exception("Shadow projection requires a finite nonzero light direction");
			}
			return direction.normalized();
		}

		[[nodiscard]] Vector3 upFor(const Vector3 &direction)
		{
			return std::abs(direction.y) < 0.95f ? Vector3{0, 1, 0} : Vector3{1, 0, 0};
		}

		[[nodiscard]] std::array<Vector3, 8> truncatedFrustum(const Camera3D &camera, float distance, float minimumDepth = 0.0f)
		{
			if (!std::isfinite(distance) || distance <= 0.0f)
			{
				throw spk::Exception("Directional shadow distance must be finite and positive");
			}
			const Matrix4x4 &projection = camera.projectionMatrix();
			const bool perspective = camera.projectionMode() == Camera3D::ProjectionMode::Perspective;
			float nearPlane;
			if (perspective)
			{
				nearPlane = projection[3][2] / (projection[2][2] - 1.0f);
			}
			else
			{
				nearPlane = (projection[3][2] + 1.0f) / projection[2][2];
			}
			if (!std::isfinite(nearPlane) || !std::isfinite(projection[0][0]) ||
				!std::isfinite(projection[1][1]) || projection[0][0] == 0.0f ||
				projection[1][1] == 0.0f)
			{
				throw spk::Exception("Invalid camera projection for directional shadows");
			}
			const float farPlane = std::max(nearPlane + 0.001f, distance);
			const Matrix4x4 inverseView =
				Matrix4x4::translation(camera.position()) * Matrix4x4::rotation(camera.rotation());
			std::array<Vector3, 8> corners{};
			for (std::size_t i = 0; i < corners.size(); ++i)
			{
				const float x = (i & 1) ? 1.0f : -1.0f;
				const float y = (i & 2) ? 1.0f : -1.0f;
				const float depth = (i & 4) ? farPlane : std::max(nearPlane, minimumDepth);
				const float vx = perspective ? x * depth / projection[0][0] : (x - projection[3][0]) / projection[0][0];
				const float vy = perspective ? y * depth / projection[1][1] : (y - projection[3][1]) / projection[1][1];
				const Vector4 world = inverseView * Vector4{vx, vy, -depth, 1.0f};
				corners[i] = {world.x, world.y, world.z};
			}
			return corners;
		}
	}

	std::array<Projection, 6> Projection::point(const Light::Selection::Entry &light)
	{
		if (light.type != Light::Selection::Type::Point || !std::isfinite(light.range) ||
			light.range <= 0.0f || !std::isfinite(light.position.x) ||
			!std::isfinite(light.position.y) || !std::isfinite(light.position.z))
		{
			throw spk::Exception("Invalid Point shadow projection parameters");
		}
		const float nearPlane = light.range * 0.001f;
		if (!std::isfinite(nearPlane) || nearPlane <= 0.0f ||
			!(nearPlane < light.range))
		{
			throw spk::Exception("Point shadow near/far planes cannot be represented");
		}
		constexpr std::array<Vector3, 6> directions{{{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
		constexpr std::array<Vector3, 6> ups{{{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}}};
		std::array<Projection, 6> result{};
		for (std::size_t face = 0; face < result.size(); ++face)
		{
			Camera3D camera;
			camera.setPosition(light.position);
			camera.lookAt(light.position + directions[face], ups[face]);
			camera.setPerspective(1.57079632679f, 1.0f, nearPlane, light.range);
			result[face] = {camera.viewProjectionMatrix(), nearPlane, light.range};
		}
		return result;
	}

	Projection Projection::spot(const Light::Selection::Entry &light)
	{
		if (light.type != Light::Selection::Type::Spot ||
			!std::isfinite(light.range) || light.range <= 0.0f ||
			!std::isfinite(light.angle) || light.angle <= 0.0f ||
			light.angle >= 1.57079632679f)
		{
			throw spk::Exception("Invalid Spot shadow projection parameters");
		}
		const Vector3 direction = normalizedDirection(light.direction);
		const float nearPlane = std::min(0.05f, light.range * 0.01f);
		Camera3D camera;
		camera.setPosition(light.position);
		camera.lookAt(light.position + direction, upFor(direction));
		camera.setPerspective(2.0f * light.angle, 1.0f, nearPlane, light.range);
		return {camera.viewProjectionMatrix(), nearPlane, light.range};
	}

	Projection Projection::directional(const Light::Selection::Entry &light, const Camera3D &camera)
	{
		if (light.type != Light::Selection::Type::Directional)
		{
			throw spk::Exception("Directional shadow projection requires a Directional light");
		}
		const Vector3 direction = normalizedDirection(light.direction);
		const auto corners = truncatedFrustum(camera, light.shadowDistance);
		Vector3 center{};
		for (const Vector3 &point : corners)
		{
			center += point;
		}
		center = center / Vector3{8, 8, 8};
		float radius = 0.0f;
		for (const Vector3 &point : corners)
		{
			radius = std::max(radius, static_cast<float>((point - center).length()));
		}
		radius = std::max(radius, 0.01f);
		Camera3D shadowCamera;
		shadowCamera.setPosition(center - direction * Vector3{2.0f * radius, 2.0f * radius, 2.0f * radius});
		shadowCamera.lookAt(center, upFor(direction));
		const Matrix4x4 view = shadowCamera.viewMatrix();
		float minX = std::numeric_limits<float>::max(), minY = minX, minZ = minX;
		float maxX = std::numeric_limits<float>::lowest(), maxY = maxX, maxZ = maxX;
		for (const Vector3 &point : corners)
		{
			const Vector4 projected = view * Vector4{point.x, point.y, point.z, 1.0f};
			minX = std::min(minX, projected.x);
			minY = std::min(minY, projected.y);
			minZ = std::min(minZ, projected.z);
			maxX = std::max(maxX, projected.x);
			maxY = std::max(maxY, projected.y);
			maxZ = std::max(maxZ, projected.z);
		}
		const float padding = std::max(0.01f, radius * 0.01f);
		const float nearPlane = std::max(0.001f, -maxZ - padding);
		const float farPlane = std::max(nearPlane + 0.01f, -minZ + padding);
		const Matrix4x4 orthographic = Matrix4x4::ortho(minX - padding, maxX + padding, minY - padding, maxY + padding, nearPlane, farPlane);
		return {orthographic * view, nearPlane, farPlane};
	}

	std::array<Projection, Projection::CascadeCount> Projection::directionalCascades(
		const Light::Selection::Entry &light, const Camera3D &camera, std::span<const CasterBounds> casters)
	{
		if (light.type != Light::Selection::Type::Directional || light.shadowResolution < 8)
		{
			throw spk::Exception("Invalid directional cascade parameters");
		}
		const auto direction = normalizedDirection(light.direction);
		const auto fullFrustum = truncatedFrustum(camera, light.shadowDistance);
		const auto cameraView = camera.viewMatrix();
		const float nearDepth = -(cameraView * Vector4(fullFrustum.front(), 1.0f)).z;
		const float farDepth = -(cameraView * Vector4(fullFrustum.back(), 1.0f)).z;
		Camera3D lightCamera;
		lightCamera.lookAt(direction, upFor(direction));
		const auto view = lightCamera.viewMatrix();

		const auto projectBounds = [](const CasterBounds &box, const Matrix4x4 &matrix) {
			CasterBounds result{{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
								{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()}};
			for (std::size_t i = 0; i < 8; ++i)
			{
				const auto p = matrix * Vector4{(i & 1) ? box.maximum.x : box.minimum.x,
												(i & 2) ? box.maximum.y : box.minimum.y,
												(i & 4) ? box.maximum.z : box.minimum.z, 1};
				result.minimum = {std::min(result.minimum.x, p.x), std::min(result.minimum.y, p.y), std::min(result.minimum.z, p.z)};
				result.maximum = {std::max(result.maximum.x, p.x), std::max(result.maximum.y, p.y), std::max(result.maximum.z, p.z)};
			}
			return result;
		};
		std::vector<CasterBounds> lightBounds;
		lightBounds.reserve(casters.size());
		for (const auto &caster : casters)
		{
			lightBounds.push_back(projectBounds(caster, view));
		}

		// Fractions of the receiver depth interval; overlap the preceding slice
		// by ten percent of its width for the shader's transition.
		constexpr std::array<float, CascadeCount> fractions{0.125f, 0.28125f, 0.53125f, 1.0f};
		std::array<Projection, CascadeCount> result{};
		float previous = nearDepth;
		float previousWidth = 0.0f;
		for (std::size_t i = 0; i < result.size(); ++i)
		{
			const float split = nearDepth + (farDepth - nearDepth) * fractions[i];
			const auto corners = truncatedFrustum(camera, split, previous - previousWidth * 0.1f);
			Vector3 minimum{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
			Vector3 maximum{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
			for (const auto &corner : corners)
			{
				const auto p = view * Vector4(corner, 1.0f);
				minimum = {std::min(minimum.x, p.x), std::min(minimum.y, p.y), std::min(minimum.z, p.z)};
				maximum = {std::max(maximum.x, p.x), std::max(maximum.y, p.y), std::max(maximum.z, p.z)};
			}
			const float extent = std::max(maximum.x - minimum.x, maximum.y - minimum.y);
			const float half = std::ceil(std::max(0.01f, extent * 0.51f) * 64.0f) / 64.0f;
			const float resolution = static_cast<float>(light.shadowResolution);
			const float paddedHalf = half / (1.0f - 4.0f / resolution);
			const float texel = 2.0f * paddedHalf / resolution;
			const float x = std::round((minimum.x + maximum.x) * 0.5f / texel) * texel;
			const float y = std::round((minimum.y + maximum.y) * 0.5f / texel) * texel;
			const float left = x - paddedHalf, right = x + paddedHalf;
			const float bottom = y - paddedHalf, top = y + paddedHalf;
			// Only extend towards the light for casters overlapping the receiver
			// footprint. Off-camera blockers remain in the shadow map.
			for (const auto &box : lightBounds)
			{
				if (box.maximum.x >= left && box.minimum.x <= right &&
					box.maximum.y >= bottom && box.minimum.y <= top && box.maximum.z >= minimum.z)
				{
					maximum.z = std::max(maximum.z, box.maximum.z);
				}
			}
			const float nearPlane = std::floor((-maximum.z - texel) / texel) * texel;
			const float farPlane = std::ceil((-minimum.z + texel) / texel) * texel;
			result[i] = {Matrix4x4::ortho(left, right, bottom, top, nearPlane, farPlane) * view,
						 nearPlane, farPlane, split, texel};
			previousWidth = split - previous;
			previous = split;
		}
		return result;
	}

	bool Projection::intersects(const CasterBounds &bounds) const
	{
		std::array<bool, 6> outside{true, true, true, true, true, true};
		for (std::size_t i = 0; i < 8; ++i)
		{
			const auto p = viewProjection * Vector4{(i & 1) ? bounds.maximum.x : bounds.minimum.x,
													(i & 2) ? bounds.maximum.y : bounds.minimum.y,
													(i & 4) ? bounds.maximum.z : bounds.minimum.z, 1};
			outside[0] &= p.x < -p.w;
			outside[1] &= p.x > p.w;
			outside[2] &= p.y < -p.w;
			outside[3] &= p.y > p.w;
			outside[4] &= p.z < -p.w;
			outside[5] &= p.z > p.w;
		}
		return std::none_of(outside.begin(), outside.end(), [](bool value) { return value; });
	}
}
