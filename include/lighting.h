#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include <raylib.h>

struct LightSource {
	int x;
	int y;
	int radius;
};

struct LightingMap {
	int width = 0;
	int height = 0;
	std::vector<std::vector<uint8_t>> obstacles;

	inline LightingMap() = default;

	inline LightingMap(int width, int height)
		: width(width), height(height), obstacles(height, std::vector<uint8_t>(width, 0)) {}

	inline void setObstacle(int x, int y, uint8_t val) {
		if (x >= 0 && x < width && y >= 0 && y < height) {
			obstacles[y][x] = val;
		}
	}

	inline void calcAndDrawLights(
		const std::vector<LightSource>& sources,
		int scrX, int scrY, int scrWid, int scrHeight
	) const {
		constexpr int rayCount = 128;
		constexpr int lightBands = 8;
		constexpr float bandStepDistance = 4;
		constexpr float rayStep = 0.5f;
		const Color lightColor{246, 154, 84, 18};

		for (const LightSource& source : sources) {
			if (source.radius <= 0) {
				continue;
			}

			std::vector<Vector2> endpoints;
			endpoints.reserve(rayCount);

			for (int ray = 0; ray < rayCount; ++ray) {
				const float angle = 2.0f * PI * ray / rayCount;
				const float dirX = std::cos(angle);
				const float dirY = std::sin(angle);
				float distance = static_cast<float>(source.radius);

				for (float sample = 0.0f; sample <= source.radius; sample += rayStep) {
					const int worldX = static_cast<int>(std::floor(source.x + dirX * sample));
					const int worldY = static_cast<int>(std::floor(source.y + dirY * sample));
					const int screenX = worldX - scrX;
					const int screenY = worldY - scrY;

					const bool outsideMap = worldX < 0 || worldX >= width || worldY < 0 || worldY >= height;
					const bool outsideView = screenX < 0 || screenX >= scrWid || screenY < 0 || screenY >= scrHeight;
					const bool blocked = outsideMap || outsideView || obstacles[worldY][worldX] != 0;

					if (blocked) {
						distance = sample;
						break;
					}
				}

				endpoints.push_back(Vector2{
					source.x - scrX + dirX * distance,
					source.y - scrY + dirY * distance
				});
			}

			const Vector2 center{
				static_cast<float>(source.x - scrX),
				static_cast<float>(source.y - scrY)
			};

			for (int ray = 0; ray < rayCount; ++ray) {
				for (int band = lightBands; band > 0; --band) {
					const float scale = band / static_cast<float>(lightBands);

					const Vector2 first{
						center.x + (endpoints[ray].x - center.x) * scale,
						center.y + (endpoints[ray].y - center.y) * scale
					};
					const Vector2& nextEndpoint = endpoints[(ray + 1) % rayCount];

					const Vector2 second{
						center.x + (nextEndpoint.x - center.x) * scale,
						center.y + (nextEndpoint.y - center.y) * scale
					};

					Vector2 endpointAvg = {
						(endpoints[ray].x + nextEndpoint.x) / 2.0f,
						(endpoints[ray].y + nextEndpoint.y) / 2.0f
					};

					Vector2 endpointAvgDist = {
						endpointAvg.x - center.x,
						endpointAvg.y - center.y
					};

					float endpointDist = sqrt(
						endpointAvgDist.x * endpointAvgDist.x +
						endpointAvgDist.y * endpointAvgDist.y
					);

					int bandMax = endpointDist / bandStepDistance;
					if (bandMax > band) {
						DrawTriangle(center, second, first, lightColor);
					}
				}
			}
		}
	}
};
