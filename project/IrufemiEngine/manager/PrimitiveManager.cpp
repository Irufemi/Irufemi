#include "PrimitiveManager.h"

void PrimitiveManager::GetPrimitive(const PrimitiveType& primitiveType, std::vector<VertexData>& vertexDataList, std::vector<uint32_t>& indexDataList) {

	switch (primitiveType)
	{
		case PrimitiveType::Triangle:
			break;
		case PrimitiveType::Plane:

			// ローカルXY平面上の4頂点(-Z向き)
			//  v3(-0.5,0.5,0)----v2(0.5,0.5,0)
			//      |                |
			//      |                |
			//  v0(-0.5,-0.5,0)--v1(0.5,-0.5,0)
			vertexDataList.push_back({ { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }); // v0
			vertexDataList.push_back({ {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }); // v1
			vertexDataList.push_back({ {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }); // v2
			vertexDataList.push_back({ { -0.5f,  0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }); // v3

			// 2トライアングル (-Z向き)
			indexDataList.push_back(0);
			indexDataList.push_back(2);
			indexDataList.push_back(1);
			indexDataList.push_back(0);
			indexDataList.push_back(3);
			indexDataList.push_back(2);
			break;
		case PrimitiveType::Cube:
			break;
		case PrimitiveType::Cylinder:
			break;
		case PrimitiveType::Sphere:
			break;
		case PrimitiveType::Tetra:
			break;
		case PrimitiveType::Skybox:
			break;

	default:
		break;
	}

}