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
	float4 lightDirection;
	float4 lightPosition;
	float4 lightColor;
	float4 viewPosition;
	matrix invView;
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
	
	float3 position;
	
	// Sample the colors from the color render texture using the point sampler at this texture coordinate location.
	colors = colorTexture.Sample(SampleTypePoint, input.tex);
	
	// Sample the normals from the normal render texture using the point sampler at this texture coordinate location.
	normals = normalTexture.Sample(SampleTypePoint, input.tex);
	normal = normals;
	position = positionTexture.Sample(SampleTypePoint, input.tex);
	outputColor = float4(0, 0, 0, 0);
	//float depth = normals.x * 500;
	////outputColor = float4(depth, depth, depth,1);
	//position = input.view_dir * (depth / input.view_dir.z);

	//position = mul(position, invView);
	//outputColor = float4(position / 500,1);
	//return outputColor;

		float3 lightDir = lightPosition - position; //3D position in space of the surface
		float distance = length(lightDir);
		//if (distance < 300)
		{

			lightDir = lightDir / distance; // = normalize( lightDir );
			distance = distance * distance; //This line may be optimised using Inverse square root
			
			float NdotL = dot(normal, lightDir);
			float intensity = saturate(NdotL);

			float4 diffuse;
			diffuse = intensity / distance * colors / 4;
			float3 H = normalize(lightDir + normalize(viewPosition - position));

			float NdotH = dot(normal, H);
			intensity = pow(saturate(NdotH), 20);

			float4 specularColor = lightColor;
			float4 specular = intensity* 4 * specularColor / distance;
		
		
			// Determine the final amount of diffuse color based on the color of the pixel combined with the light intensity.
			//outputColor = saturate(colors * lightIntensity);
			outputColor += (diffuse + specular);
		}

	return outputColor;
}
