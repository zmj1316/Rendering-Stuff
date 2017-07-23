////////////////////////////////////////////////////////////////////////////////
// Filename: light.ps
////////////////////////////////////////////////////////////////////////////////


/////////////
// GLOBALS //
/////////////
Texture2D colorTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D positionTexture : register(t2);


///////////////////
// SAMPLE STATES //
///////////////////
SamplerState SampleTypePoint : register(s0);


//////////////////////
// CONSTANT BUFFERS //
//////////////////////
cbuffer LightBuffer
{
	float3 lightDirection;
	float3 lightPosition;
	float3 lightColor;
	float3 viewPosition;
};


//////////////
// TYPEDEFS //
//////////////
struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};


////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 LightPixelShader(PixelInputType input) : SV_TARGET
{
	float4 colors;
	float4 normals;
	float3 lightDir;
	float lightIntensity;
	float4 outputColor;
	
	float3 position;
	float3 pointDir;
	
	// Sample the colors from the color render texture using the point sampler at this texture coordinate location.
	colors = colorTexture.Sample(SampleTypePoint, input.tex);
	
	// Sample the normals from the normal render texture using the point sampler at this texture coordinate location.
	normals = normalTexture.Sample(SampleTypePoint, input.tex);
	
	position = positionTexture.Sample(SampleTypePoint, input.tex)*float3(1000,1000,1000);
	pointDir = position - lightPosition;
	// Invert the light direction for calculations.
	lightDir = -lightDirection;
	
	// Calculate the amount of light on this pixel.
	lightIntensity = saturate(dot(normals.xyz, lightDir));
	
	float4 diffuse = max(0, dot(normals, lightPosition - position));
	diffuse = diffuse * colors;
	
	float3 H = normalize(viewPosition - position + lightPosition - position);
	float4 specularColor = float4(0.1, 0.7, 0.1, 1);
	float4 specular = specularColor * pow(max(0, dot(H, normals)), 2);
	
	
	// Determine the final amount of diffuse color based on the color of the pixel combined with the light intensity.
	outputColor = saturate(colors * lightIntensity);
	outputColor += specular;

	return outputColor;
}
