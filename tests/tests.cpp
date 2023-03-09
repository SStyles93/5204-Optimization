#define STB_IMAGE_IMPLEMENTATION
#include <gtest/gtest.h>
#include "Rasterizer.hpp"

TEST(SceneTests, TextureStruct)
{
	Texture texture;
	EXPECT_EQ(texture.data, nullptr);
	EXPECT_EQ(texture.width, -1);
	EXPECT_EQ(texture.height, -1);
	EXPECT_EQ(texture.numChannels, -1);
}

//TEST(SceneTests, VertexInputStruct)
//{
//	VertexInput vInput;
//	EXPECT_EQ(vInput.pos., nullptr);
//	EXPECT_EQ(vInput.normal, nullptr);
//	EXPECT_EQ(vInput.texCoords, nullptr);
//}

//TEST(SceneTests, FragmentInputStruct)
//{
//	FragmentInput fInput;
//	EXPECT_EQ(fInput.normal, nullptr);
//	EXPECT_EQ(fInput.texCoords, nullptr);
//}
TEST(SceneTests, MeshStruct)
{
	Mesh mesh;
	EXPECT_EQ(mesh.idxOffset, 0u);
	EXPECT_EQ(mesh.idxCount, 0u);
	EXPECT_EQ(mesh.diffuseTexName, "");
}

TEST(SceneTests, IndexedPrimitiveStruct)
{
	IndexedPrimitive primitive;
	EXPECT_EQ(primitive.posIdx, 0u);
	EXPECT_EQ(primitive.normalIdx, 0u);
	EXPECT_EQ(primitive.uvIdx, 0u);
}
//
//TEST(SceneTests, IndexedPrimitiveStructOperator)
//{
//	IndexedPrimitive primitive;
//	
//}

TEST(RasterizerTests, Constructors)
{
	Scene scene;
	Rasterizer rasterizer(std::move(scene));
	EXPECT_EQ(rasterizer.GetScreenHeight(), SCREEN_HEIGHT);
	EXPECT_EQ(rasterizer.GetScreenWidth(), SCREEN_WIDTH);
}
TEST(RasterizerTests, RasterizerInit)
{
	Scene scene;
	Rasterizer rasterizer(std::move(scene));
	EXPECT_EQ(rasterizer.GetDepthBuffer().at(0), SCREEN_WIDTH * SCREEN_HEIGHT);
	EXPECT_EQ(rasterizer.GetFrameBuffer().at(0).x, 0);
	EXPECT_EQ(rasterizer.GetFrameBuffer().at(0).y, 0);
	EXPECT_EQ(rasterizer.GetFrameBuffer().at(0).z, 0);
}
