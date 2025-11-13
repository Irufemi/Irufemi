#include "GeometryShaderHeader.hlsli"
#include "Object3D.hlsli"

[maxvertexcount(3)]
void main(
	triangle VertexShaderOutput input[3], 
	inout TriangleStream< GeometryShaderOutput > output
)
{
    [unroll]
	for (uint i = 0; i < 3; i++)
	{
		GeometryShaderOutput element;
		element.svpos = input[i].position;
		element.normal = input[i].normal;
		element.uv = input[i].texcoord;
		element.worldPosition = input[i].worldPosition;
		output.Append(element);
	}
}