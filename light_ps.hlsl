////////////////////////////////////////////////////////////////////////////////
// Filename: light.ps
////////////////////////////////////////////////////////////////////////////////


/////////////
// GLOBALS //
/////////////
Texture2D colorTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D positionTexture : register(t2);
Texture2D shadowTexture : register(t3);


///////////////////
// SAMPLE STATES //
///////////////////
SamplerState SampleTypePoint : register(s0);
SamplerState SampleTypeClamp : register(s1);
SamplerState SampleTypeWrap : register(s2);


//////////////////////
// CONSTANT BUFFERS //
//////////////////////
cbuffer LightBuffer
{
	float4 lightDirection;
	float4 lightPosition;
	float4 lightColor;
	float4 viewPosition;
	matrix invView;
	matrix lightViewMatrix;
	matrix lightProjectionMatrix;
};


//////////////
// TYPEDEFS //
//////////////
struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 view_dir  : POSITION;
	float2 tex : TEXCOORD0;
};


////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 LightPixelShader(PixelInputType input) : SV_TARGET
{
	//return (1,1,1,1);
	float4 colors;
	float4 normals;
	float3 normal;
	float lightIntensity;
	float4 outputColor;
	
	float4 position;
	
	// Sample the colors from the color render texture using the point sampler at this texture coordinate location.
	colors = colorTexture.Sample(SampleTypePoint, input.tex);
	
	// Sample the normals from the normal render texture using the point sampler at this texture coordinate location.
	normals = normalTexture.Sample(SampleTypePoint, input.tex);
	normal = normals;
	position = positionTexture.Sample(SampleTypePoint, input.tex);
	outputColor = float4(0, 0, 0, 0);
	//return float4(position,1);
	//float4 position__ = float4(position, 1);
	float4 lightViewPosition = mul(position, lightViewMatrix);
	float4 lightProjViewPosition = mul(lightViewPosition, lightProjectionMatrix);
	// Calculate the projected texture coordinates.
	float2 projectTexCoord;

	projectTexCoord.x = lightProjViewPosition.x / lightProjViewPosition.w / 2.0f + 0.5f;
	projectTexCoord.y = -lightProjViewPosition.y / lightProjViewPosition.w / 2.0f + 0.5f;
	//float depth = normals.x * 500;
	////outputColor = float4(depth, depth, depth,1);
	//position = input.view_dir * (depth / input.view_dir.z);

	//position = mul(position, invView);
	//outputColor = float4(position / 500,1);
	//return outputColor;
	//return float4(projectTexCoord, 0, 1);
	if ((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
	{
		// Sample the shadow map depth value from the depth texture using the sampler at the projected texture coordinate location.
		float depthValue = shadowTexture.Sample(SampleTypeClamp, projectTexCoord).r;

		// Calculate the depth of the light.
		float lightDepthValue = lightViewPosition.z / 500;
		//return float4(depthValue, depthValue, 0, 1);
		// Subtract the bias from the lightDepthValue.
		lightDepthValue = lightDepthValue - 0.001f;

		// Compare the depth of the shadow map value and the depth of the light to determine whether to shadow or to light this pixel.
		// If the light is in front of the object then light the pixel, if not then shadow this pixel since an object (occluder) is casting a shadow on it.
		if (lightDepthValue < depthValue)
		{
			float3 lightDir = lightPosition - position; //3D position in space of the surface
			float distance = length(lightDir);
			//if (distance < 300)
				lightDir = lightDir / distance; // = normalize( lightDir );
				distance = distance * distance; //This line may be optimised using Inverse square root

				float NdotL = dot(normal, lightDir);
				float intensity = saturate(NdotL);

				float4 diffuse;
				diffuse = (intensity / distance * 5) * colors;
				float3 H = normalize(lightDir + normalize(viewPosition - position));

				float NdotH = dot(normal, H);
				intensity = pow(saturate(NdotH), 20);

				float4 specularColor = lightColor;
				float4 specular = intensity * 4 * specularColor / distance;


			// Determine the final amount of diffuse color based on the color of the pixel combined with the light intensity.
				outputColor += (diffuse + specular);
		}

	}
	return outputColor;
}
