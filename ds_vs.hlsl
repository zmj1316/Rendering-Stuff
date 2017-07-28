////////////////////////////////////////////////////////////////////////////////
// Filename: deferred.vs
////////////////////////////////////////////////////////////////////////////////


//////////////////////
// CONSTANT BUFFERS //
//////////////////////
cbuffer MatrixBuffer
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projMatrix;
};


//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 instancedOffset : TEXCOORD1;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float4 depthPosition : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};


////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType DeferredVertexShader(VertexInputType input)
{
	PixelInputType output;
	float4 worldPosition;

	// Change the position vector to be 4 units for proper matrix calculations.
	input.position.w = 1.0f;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	worldPosition = mul(input.position, worldMatrix);
	worldPosition.x += input.instancedOffset.x;
	worldPosition.y += input.instancedOffset.y;
	worldPosition.z += input.instancedOffset.z;
	output.position = mul(worldPosition, viewMatrix);
	output.depthPosition = worldPosition;
	output.position = mul(output.position, projMatrix);
	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;

	// Calculate the normal vector against the world matrix only.
	output.normal = mul(input.normal, (float3x3)worldMatrix);

	// Normalize the normal vector.
	output.normal = normalize(output.normal);

	return output;
}