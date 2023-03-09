#define STB_IMAGE_IMPLEMENTATION
#include "Rasterizer.hpp"
#include "fmt/format.h"

int main()
{
	/*Scene scene;
	scene.LoadObject("../assets/sponza.obj");
	Rasterizer rasterizer(std::move(scene));
	rasterizer.TransformScene();
	auto file = fmt::format("../../render_scene_{0}x{1}.png", rasterizer.DEFAULT_WIDTH, rasterizer.DEFAULT_HEIGHT);
	rasterizer.RenderToPng(file);*/


	//Cube render
	Camera camera;
	camera.SetNearPlane(0.1f);
	camera.SetFarPlane(100.f);
	camera.SetEyePosition(glm::vec3(0, 3.75, 6.5));
	camera.SetLookDirection(glm::vec3(0, 0, 0));
	camera.SetupCamera();
	Scene scene(camera);
	scene.LoadObject("../assets/backpack.obj");
	Rasterizer rasterizer(std::move(scene));
	rasterizer.TransformScene();
	auto file = fmt::format("../../render_backpack_{0}x{1}.png", rasterizer.DEFAULT_WIDTH, rasterizer.DEFAULT_HEIGHT);
	rasterizer.RenderToPng(file);
}

//#include "oldScene.hpp"
//
//int main()
//{
//	
//	Scene();
//}