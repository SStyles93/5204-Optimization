#define STB_IMAGE_IMPLEMENTATION
#include <benchmark/benchmark.h>
#include "Rasterizer.hpp"
#include "fmt/format.h"

const uint32_t widths[] = { 1024u, 1920u, 3840u, 7680u };
const uint32_t heights[] = { 768u, 1080u, 2160u, 4320u };
const int fromRange = 0;
const int toRange = 3;

static void BM_Transform(benchmark::State& state, std::string_view objectName)
{
	Camera camera;

	if (objectName != "sponza")
	{
		camera.SetNearPlane(0.1f);
		camera.SetFarPlane(100.f);
		camera.SetEyePosition(glm::vec3(0, 5, 10));
		camera.SetLookDirection(glm::vec3(0, 0, 0));
		camera.SetViewAngle(45.0f);
		camera.SetupCamera();
	}
	camera.SetupCamera();
	Scene scene(camera);
	scene.LoadObject(fmt::format("../assets/{0}.obj", objectName));

	for (auto _ : state)
	{
		Rasterizer rasterizer(std::move(scene), widths[state.range(0)], heights[state.range(0)]);
		rasterizer.TransformScene();
	}
}
BENCHMARK_CAPTURE(BM_Transform, TransformCube, "cube")
->DenseRange(fromRange, toRange, 1)
->Unit(benchmark::kMillisecond)
->MinTime(10.0);
BENCHMARK_CAPTURE(BM_Transform, TransformBackpack, "backpack")
->DenseRange(fromRange, toRange, 1)
->Unit(benchmark::kMillisecond)
->MinTime(10.0);
BENCHMARK_CAPTURE(BM_Transform, TransformScene, "sponza")
->DenseRange(fromRange, toRange, 1)
->Unit(benchmark::kMillisecond)
->MinTime(10.0);
BENCHMARK_MAIN();