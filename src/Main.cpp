#define STB_IMAGE_IMPLEMENTATION
#include "Rasterizer.hpp"
#include "fmt/format.h"

int main()
{
	std::string_view objectName = "sponza";

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
	Scene scene(camera);
	scene.LoadObject(fmt::format("../assets/{0}.obj", objectName));
	Rasterizer rasterizer(std::move(scene));
	rasterizer.TransformScene();
	auto file = fmt::format("../../render_{0}_{1}x{2}.png", objectName, rasterizer.DEFAULT_WIDTH, rasterizer.DEFAULT_HEIGHT);
	rasterizer.RenderToPng(file);
}