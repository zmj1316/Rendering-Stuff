#pragma once
#include <d3dx9.h>
#include <d3d11.h>
#include "DXUT.h"
#include <random>
#include "shadow.h"

namespace light
{
	struct MatrixBufferType
	{
		D3DXMATRIX world;
		D3DXMATRIX view;
		D3DXMATRIX projection;
		D3DXMATRIX invView;
	};

	struct LightBufferType
	{
		D3DXVECTOR4 lightDirection;
		D3DXVECTOR4 lightPosition;
		D3DXVECTOR4 lightColor;
		D3DXVECTOR4 viewPosition;
		D3DXMATRIX  invView;
		D3DXMATRIX lightView;
		D3DXMATRIX lightProjection;
	};

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;
	ID3D11SamplerState* m_sampleState;
	ID3D11Buffer* m_matrixBuffer;
	ID3D11Buffer* m_lightBuffer;

	ID3D11DepthStencilState* LVState0 = nullptr;
	ID3D11DepthStencilState* LVState1 = nullptr;
	ID3D11DepthStencilState* LVState2 = nullptr;

	ID3D11RasterizerState* front_state = nullptr;
	ID3D11RasterizerState* back_state = nullptr;

	ID3D11SamplerState* m_sampleStateWrap;
	ID3D11SamplerState* m_sampleStateClamp;

	void LV_pass(ID3D11DeviceContext* deviceContext, int indexCount, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix,
		D3DXMATRIX projectionMatrix, ID3D11ShaderResourceView* colorTexture, ID3D11ShaderResourceView* normalTexture, ID3D11ShaderResourceView* positionTexture,
		D3DXVECTOR3 lightDirection, D3DXVECTOR3 lightPosition, D3DXVECTOR3 lightColor, D3DXVECTOR3 viewPosition, D3DXMATRIX	invViewProj)

	{
		HRESULT hr;
		deviceContext->VSSetShader(m_vertexShader, NULL, 0);
		deviceContext->PSSetShader(nullptr, NULL, 0);
		D3DXMATRIX scale,translation;
		D3DXMatrixScaling(&scale, 40, 40, 40);
		D3DXMatrixTranslation(&translation, lightPosition.x, lightPosition.y, lightPosition.z);
		worldMatrix = scale * translation * worldMatrix;
		// Transpose the matrices to prepare them for the shader.
		D3DXMatrixTranspose(&worldMatrix, &worldMatrix);
		D3DXMatrixTranspose(&viewMatrix, &viewMatrix);
		D3DXMatrixTranspose(&projectionMatrix, &projectionMatrix);
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// Lock the constant buffer so it can be written to.
		V(deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

		unsigned int bufferNumber;

		MatrixBufferType* dataPtr;

		// Get a pointer to the data in the constant buffer.
		dataPtr = (MatrixBufferType*)mappedResource.pData;

//		// Copy the matrices into the constant buffer.
//		D3DXMatrixTranslation(&dataPtr->world, lightPosition.x, lightPosition.y, lightPosition.z);
		dataPtr->world = worldMatrix;
		dataPtr->view = viewMatrix;
		dataPtr->projection = projectionMatrix;

		// Unlock the constant buffer.
		deviceContext->Unmap(m_matrixBuffer, 0);

		// Set the position of the constant buffer in the vertex shader.
		bufferNumber = 0;

		// Now set the constant buffer in the vertex shader with the updated values.
		deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);

		deviceContext->RSSetState(back_state);
		deviceContext->OMSetDepthStencilState(LVState0, 0xFF);
		deviceContext->DrawIndexed(indexCount, 0, 0);
		deviceContext->RSSetState(front_state);
		deviceContext->OMSetDepthStencilState(LVState1, 0);
		deviceContext->DrawIndexed(indexCount, 0, 0);
//		deviceContext->RSSetState(front_state);
	}


	void Render(ID3D11DeviceContext* deviceContext, int indexCount, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix,
		D3DXMATRIX projectionMatrix, ID3D11ShaderResourceView* colorTexture, ID3D11ShaderResourceView* normalTexture, ID3D11ShaderResourceView* positionTexture,
		D3DXVECTOR3 lightDirection, D3DXVECTOR3 lightPosition, D3DXVECTOR3 lightColor, D3DXVECTOR3 viewPosition, D3DXMATRIX	invProj,D3DXMATRIX	gView)
	{
		HRESULT hr;
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		unsigned int bufferNumber;
		MatrixBufferType* dataPtr;
		LightBufferType* dataPtr2;


		// Transpose the matrices to prepare them for the shader.
		D3DXMatrixTranspose(&worldMatrix, &worldMatrix);
		D3DXMatrixTranspose(&viewMatrix, &viewMatrix);
		D3DXMatrixTranspose(&projectionMatrix, &projectionMatrix);

		// Lock the constant buffer so it can be written to.
		V( deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

		// Get a pointer to the data in the constant buffer.
		dataPtr = (MatrixBufferType*)mappedResource.pData;

		// Copy the matrices into the constant buffer.
		dataPtr->world = worldMatrix;
		dataPtr->view = viewMatrix;
		dataPtr->projection = projectionMatrix;

		// Unlock the constant buffer.
		deviceContext->Unmap(m_matrixBuffer, 0);

		// Set the position of the constant buffer in the vertex shader.
		bufferNumber = 0;

		// Now set the constant buffer in the vertex shader with the updated values.
		deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);

		// Set shader texture resources in the pixel shader.
		deviceContext->PSSetShaderResources(0, 1, &colorTexture);
		deviceContext->PSSetShaderResources(1, 1, &normalTexture);
		deviceContext->PSSetShaderResources(2, 1, &positionTexture);
		deviceContext->PSSetShaderResources(3, 1, &shadow::m_shaderResourceView);

		// Lock the light constant buffer so it can be written to.
		V(deviceContext->Map(m_lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

		// Get a pointer to the data in the constant buffer.
		dataPtr2 = (LightBufferType*)mappedResource.pData;

		// Copy the lighting variables into the constant buffer.
		D3DXMatrixTranspose(&invProj, &invProj);
		dataPtr2->lightDirection = D3DXVECTOR4(lightDirection,1);
		dataPtr2->lightPosition = D3DXVECTOR4(lightPosition, 1);
		dataPtr2->lightColor = D3DXVECTOR4(lightColor,1);
		D3DXMatrixInverse(&gView, nullptr, &gView);
		D3DXMatrixTranspose(&gView, &gView);
		dataPtr2->lightView = shadow::GetViewMatrix();
		D3DXMatrixTranspose(&dataPtr2->lightView, &dataPtr2->lightView);

		dataPtr2->lightProjection = shadow::GetProjMatrix();
		D3DXMatrixTranspose(&dataPtr2->lightProjection, &dataPtr2->lightProjection);

		dataPtr->invView = gView;
		dataPtr2->invView = gView;
//		for (int i = 0; i < 32; ++i)
//		{
//			srand(i);
//			for (int j = 0; j < 32; ++j)
//			{
//				dataPtr2->lightPosition[i*32 + j] = D3DXVECTOR4((i-16)*5,(j-16)*5,-4,1);
//				dataPtr2->lightColor[i * 32 + j] = D3DXVECTOR4(0.5*rand()/RAND_MAX, 0.5*rand() / RAND_MAX, 0.5*rand() / RAND_MAX,1);
//			}
//		}
		dataPtr2->viewPosition = D3DXVECTOR4(viewPosition,1);
		// Unlock the constant buffer.
		deviceContext->Unmap(m_lightBuffer, 0);

		// Set the position of the light constant buffer in the pixel shader.
		bufferNumber = 0;

		// Finally set the light constant buffer in the pixel shader with the updated values.
		deviceContext->PSSetConstantBuffers(bufferNumber, 1, &m_lightBuffer);

		// Set the vertex input layout.
		deviceContext->IASetInputLayout(m_layout);

		// Set the vertex and pixel shaders that will be used to render.
		deviceContext->VSSetShader(m_vertexShader, NULL, 0);
		deviceContext->PSSetShader(m_pixelShader, NULL, 0);

		// Set the sampler state in the pixel shader.
		deviceContext->PSSetSamplers(0, 1, &m_sampleState);
		// Set the sampler states in the pixel shader.
		deviceContext->PSSetSamplers(1, 1, &m_sampleStateClamp);
		deviceContext->PSSetSamplers(2, 1, &m_sampleStateWrap);
//		deviceContext->OMSetDepthStencilState(LVState2, 0);

		// Render the geometry.
		deviceContext->DrawIndexed(indexCount, 0, 0);

	}


	void ReleaseLight();

	void InitLight(ID3D11Device* device)
	{
		HRESULT hr;
		ID3D10Blob* errorMessage;
		ID3D10Blob* vertexShaderBuffer;
		ID3D10Blob* pixelShaderBuffer;
		D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
		unsigned int numElements;
		D3D11_SAMPLER_DESC samplerDesc;
		D3D11_BUFFER_DESC matrixBufferDesc;
		D3D11_BUFFER_DESC lightBufferDesc;


		// Initialize the pointers this function will use to null.
		errorMessage = 0;
		vertexShaderBuffer = 0;
		pixelShaderBuffer = 0;

		// Compile the vertex shader code.
		V(D3DX11CompileFromFile(L"light_vs.hlsl", NULL, NULL, "LightVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL,
			&vertexShaderBuffer, &errorMessage, NULL));

		// Compile the pixel shader code.
		V(D3DX11CompileFromFile(L"light_ps.hlsl", NULL, NULL, "LightPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL,
			&pixelShaderBuffer, &errorMessage, NULL));

		// Create the vertex shader from the buffer.
		V(device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader));

		// Create the pixel shader from the buffer.
		V(device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader));

		// Create the vertex input layout description.
		polygonLayout[0].SemanticName = "POSITION";
		polygonLayout[0].SemanticIndex = 0;
		polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		polygonLayout[0].InputSlot = 0;
		polygonLayout[0].AlignedByteOffset = 0;
		polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[0].InstanceDataStepRate = 0;

		polygonLayout[1].SemanticName = "TEXCOORD";
		polygonLayout[1].SemanticIndex = 0;
		polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		polygonLayout[1].InputSlot = 0;
		polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[1].InstanceDataStepRate = 0;

		// Get a count of the elements in the layout.
		numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

		// Create the vertex input layout.
		V(device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(),
			&m_layout));

		// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
		vertexShaderBuffer->Release();
		vertexShaderBuffer = 0;

		pixelShaderBuffer->Release();
		pixelShaderBuffer = 0;

		// Create a texture sampler state description.
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		// Create the texture sampler state.
		V(device->CreateSamplerState(&samplerDesc, &m_sampleState));

		// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
		matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
		matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		matrixBufferDesc.MiscFlags = 0;
		matrixBufferDesc.StructureByteStride = 0;

		// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
		V(device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer));

		// Setup the description of the light dynamic constant buffer that is in the pixel shader.
		lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		lightBufferDesc.ByteWidth = sizeof(LightBufferType);
		lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		lightBufferDesc.MiscFlags = 0;
		lightBufferDesc.StructureByteStride = 0;

		// Create the constant buffer pointer so we can access the pixel shader constant buffer from within this class.
		V(device->CreateBuffer(&lightBufferDesc, NULL, &m_lightBuffer));

		D3D11_DEPTH_STENCIL_DESC LVdesc;
		LVdesc.DepthEnable = true;
		LVdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		LVdesc.DepthFunc = D3D11_COMPARISON_LESS;
		LVdesc.StencilEnable = true;
		LVdesc.StencilReadMask = 0xFF;
		LVdesc.StencilWriteMask = 0xFF;
		LVdesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
		LVdesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
		LVdesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		LVdesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		LVdesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		LVdesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		LVdesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		LVdesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		V(device->CreateDepthStencilState(&LVdesc, &LVState0));

		LVdesc.DepthEnable = true;
		LVdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		LVdesc.DepthFunc = D3D11_COMPARISON_LESS;
		LVdesc.StencilEnable = true;
		LVdesc.StencilReadMask = 0xFF;
		LVdesc.StencilWriteMask = 0xFF;
		LVdesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INVERT;
		LVdesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		LVdesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		LVdesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
//		LVdesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
//		LVdesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR_SAT;
//		LVdesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
//		LVdesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
		V(device->CreateDepthStencilState(&LVdesc, &LVState1));

		LVdesc.DepthEnable = false;
		LVdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		LVdesc.DepthFunc = D3D11_COMPARISON_LESS;
		LVdesc.StencilEnable = true;
		LVdesc.StencilReadMask = 0xFF;
		LVdesc.StencilWriteMask = 0xFF;
		LVdesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		LVdesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
		LVdesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
		LVdesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
		LVdesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		LVdesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
		LVdesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		LVdesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
		V(device->CreateDepthStencilState(&LVdesc, &LVState2));

		D3D11_RASTERIZER_DESC rasterDesc;
		rasterDesc.AntialiasedLineEnable = false;
		rasterDesc.CullMode = D3D11_CULL_BACK;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 0.0f;
		rasterDesc.DepthClipEnable = true;
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.FrontCounterClockwise = false;
		rasterDesc.MultisampleEnable = false;
		rasterDesc.ScissorEnable = false;
		rasterDesc.SlopeScaledDepthBias = 0.0f;
		V(device->CreateRasterizerState(&rasterDesc, &front_state));
		rasterDesc.FrontCounterClockwise = true;
		V(device->CreateRasterizerState(&rasterDesc, &back_state));



		// Create a wrap texture sampler state description.
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		// Create the texture sampler state.
		V( device->CreateSamplerState(&samplerDesc, &m_sampleStateWrap));

		// Create a clamp texture sampler state description.
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

		// Create the texture sampler state.
		V( device->CreateSamplerState(&samplerDesc, &m_sampleStateClamp));
	}

	void ReleaseLight()
	{
		// Release the light constant buffer.
		if (m_lightBuffer)
		{
			m_lightBuffer->Release();
			m_lightBuffer = 0;
		}

		// Release the matrix constant buffer.
		if (m_matrixBuffer)
		{
			m_matrixBuffer->Release();
			m_matrixBuffer = 0;
		}

		// Release the sampler state.
		if (m_sampleState)
		{
			m_sampleState->Release();
			m_sampleState = 0;
		}

		// Release the layout.
		if (m_layout)
		{
			m_layout->Release();
			m_layout = 0;
		}

		// Release the pixel shader.
		if (m_pixelShader)
		{
			m_pixelShader->Release();
			m_pixelShader = 0;
		}

		// Release the vertex shader.
		if (m_vertexShader)
		{
			m_vertexShader->Release();
			m_vertexShader = 0;
		}
		SAFE_RELEASE(LVState0);
		SAFE_RELEASE(LVState1);
		SAFE_RELEASE(LVState2);
		SAFE_RELEASE(front_state);
		SAFE_RELEASE(back_state);
		// Release the sampler states.
		if (m_sampleStateWrap)
		{
			m_sampleStateWrap->Release();
			m_sampleStateWrap = 0;
		}

		if (m_sampleStateClamp)
		{
			m_sampleStateClamp->Release();
			m_sampleStateClamp = 0;
		}
	}
}
