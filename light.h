#pragma once
#include <d3dx9.h>
#include <d3d11.h>
#include "DXUT.h"

namespace light
{
	struct MatrixBufferType
	{
		D3DXMATRIX world;
		D3DXMATRIX view;
		D3DXMATRIX projection;
	};

	struct LightBufferType
	{
		D3DXVECTOR3 lightDirection;
		float padding;
	};

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;
	ID3D11SamplerState* m_sampleState;
	ID3D11Buffer* m_matrixBuffer;
	ID3D11Buffer* m_lightBuffer;

	void Render(ID3D11DeviceContext* deviceContext, int indexCount, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix,
		D3DXMATRIX projectionMatrix, ID3D11ShaderResourceView* colorTexture, ID3D11ShaderResourceView* normalTexture,
		D3DXVECTOR3 lightDirection)
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

		// Lock the light constant buffer so it can be written to.
		V(deviceContext->Map(m_lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

		// Get a pointer to the data in the constant buffer.
		dataPtr2 = (LightBufferType*)mappedResource.pData;

		// Copy the lighting variables into the constant buffer.
		dataPtr2->lightDirection = lightDirection;
		dataPtr2->padding = 0.0f;

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

		// Render the geometry.
		deviceContext->DrawIndexed(indexCount, 0, 0);

		ID3D11ShaderResourceView* t[1] = { nullptr};
		deviceContext->PSSetShaderResources(0, 1, t);
		deviceContext->PSSetShaderResources(1, 1, t);
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
	}
}
