#include "GeometryShaderHeader.hlsli"
#include "Object3D.hlsli"

/*ジオメトリシェーダの導入*/

[maxvertexcount(3)]
void main(
	triangle VertexShaderOutput input[3],
	inout TriangleStream<GeometryShaderOutput> output
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

/*ジオメトリの加工*/

/// Append

//[maxvertexcount(1)]
//void main(
//triangle VertexShaderOutput input[3],
//inout PointStream<GeometryShaderOutput> output
//)
//{
//	GeometryShaderOutput element;
//	element.svpos = input[0].position;
//	element.normal = input[0].normal;
//	element.uv = input[0].texcoord;
//	element.worldPosition = input[0].worldPosition;
//	output.Append(element);
//}

/// 複数頂点の出力

//[maxvertexcount(3)]
//void main(
//triangle VertexShaderOutput input[3],
//inout PointStream<GeometryShaderOutput> output
//)
//{
//	for (uint i = 0; i < 3; i++)
//	{
//		GeometryShaderOutput element;
//		element.svpos = input[i].position;
//		element.normal = input[i].normal;
//		element.uv = input[i].texcoord;
//		element.worldPosition = input[i].worldPosition;
//		output.Append(element);
//	}
//}


/// Lineの出力

//[maxvertexcount(2)]
//void main(
//triangle VertexShaderOutput input[3],
//inout LineStream<GeometryShaderOutput> output
//)
//{
//	GeometryShaderOutput element;
//	// 線分の始点
//	element.svpos = input[0].position;
//	element.normal = input[0].normal;
//	element.uv = input[0].texcoord;
//	element.worldPosition = input[0].worldPosition;
//	output.Append(element);
//	// 線分の終点
//	element.svpos = input[1].position;
//	element.normal = input[1].normal;
//	element.uv = input[1].texcoord;
//	element.worldPosition = input[1].worldPosition;
//	output.Append(element);
//}

/// RestartStrip

// 三角形の入力から、線分を３つ出力するサンプル
//[maxvertexcount(6)]
//void main(
//triangle VertexShaderOutput input[3],
//inout LineStream<GeometryShaderOutput> output
//)
//{
//	GeometryShaderOutput element;
	
//	// 三角形の頂点1点ずつ扱う
//	for (uint i = 0; i < 3; i++)
//	{
//		// 線分の始点
//		element.svpos = input[i].position;
//		element.normal = input[i].normal;
//		element.uv = input[i].texcoord;
//		element.worldPosition = input[i].worldPosition;
//		output.Append(element);
//		// 線分の終点
//		if (i == 2)
//		{
//			// +1すると溢れるので、最初に戻る
//			element.svpos = input[0].position;
//			element.normal = input[0].normal;
//			element.uv = input[0].texcoord;
//			element.worldPosition = input[0].worldPosition;
//		}
//		else
//		{
//			element.svpos = input[i + 1].position;
//			element.normal = input[i + 1].normal;
//			element.uv = input[i + 1].texcoord;
//			element.worldPosition = input[i + 1].worldPosition;
//		}
//		output.Append(element);
//		// 現在のストリップを終了し、次のストリップを開始
//		output.RestartStrip();
//	}
//}


/// LineStrip

//// 三角形の入力から、線分を３つ出力するサンプル
//[maxvertexcount(6)]
//void main(
//triangle VertexShaderOutput input[3],
//inout LineStream<GeometryShaderOutput> output
//)
//{
//	GeometryShaderOutput element;
//	// 三角形の頂点1点ずつ扱う
//	for (uint i = 0; i < 3; i++)
//	{
//		// 線分の始点
//		element.svpos = input[i].position;
//		element.normal = input[i].normal;
//		element.uv = input[i].texcoord;
//		element.worldPosition = input[i].worldPosition;
//		// 頂点を１つ追加
//		output.Append(element);
//	}
//	// 最初の点をもう一度追加
//	element.svpos = input[0].position;
//	element.normal = input[0].normal;
//	element.uv = input[0].texcoord;
//	element.worldPosition = input[0].worldPosition;
//	output.Append(element);
//}

/// 数値の加工

//[maxvertexcount(3)]
//void main(
//	triangle VertexShaderOutput input[3],
//	inout TriangleStream<GeometryShaderOutput> output
//)
//{
//    [unroll]
//	for (uint i = 0; i < 3; i++)
//	{
//		GeometryShaderOutput element;
//		element.svpos = input[i].position;
//		element.normal = input[i].normal;
//		// UVを2倍に
//		element.uv = input[i].texcoord * 2.0f;
//		element.worldPosition = input[i].worldPosition;
//		output.Append(element);
//	}
//}

/// プリミティブの複製

// 三角形の入力から、三角形を2つ出力
//[maxvertexcount(6)]
//void main(
//	triangle VertexShaderOutput input[3],
//	inout TriangleStream<GeometryShaderOutput> output
//)
//{
//	// 1つ目の三角形
//	for (uint i = 0; i < 3; i++)
//	{
//		GeometryShaderOutput element;
//		element.svpos = input[i].position;
//		element.normal = input[i].normal;
//		element.uv = input[i].texcoord;
//		element.worldPosition = input[i].worldPosition;
//		output.Append(element);
//	}
//	// 現在のストリップを終了
//	output.RestartStrip();
//	// 2つ目の三角形
//	for (uint j = 0; j < 3; j++)
//	{
//		GeometryShaderOutput element;
//		// X方向に20ずらす
//		element.svpos = input[j].position + float4(20.0f, 0.0f, 0.0f, 0.0f);
//		element.normal = input[j].normal;
//		// UVを5倍に
//		element.uv = input[j].texcoord * 5.0f;
//		element.worldPosition = input[j].worldPosition;
//		output.Append(element);
//	}
//}