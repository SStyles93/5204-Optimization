#define STB_IMAGE_IMPLEMENTATION
#include "Rasterizer.hpp"
//#include "oldScene.hpp"

int main()
{
	Scene scene;
	scene.LoadObject("../assets/sponza.obj");
	Rasterizer rasterizer(std::move(scene));
	rasterizer.TransformScene();
	rasterizer.RenderToPng("../../render_scene.png");

	//Scene();
}