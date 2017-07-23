#pragma once
#include <d3d11.h>
#include "DXUT.h"

namespace deferred
{
	constexpr int BUFFER_COUNT = 2;
	int m_textureWidth, m_textureHeight;
	ID3D11Texture2D* m_renderTargetTextureArray[BUFFER_COUNT];
	ID3D11RenderTargetView* m_renderTargetViewArray[BUFFER_COUNT];
	ID3D11ShaderResourceView* m_shaderResourceViewArray[BUFFER_COUNT];
	ID3D11Texture2D* m_depthStencilBuffer;
	ID3D11DepthStencilView* m_depthStencilView;
	D3D11_VIEWPORT m_viewport;

	struct MatrixBufferType
	{
		D3DXMATRIX world;
		D3DXMATRIX view;
		D3DXMATRIX projection;
	};

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;
	ID3D11SamplerState* m_sampleStateWrap;
	ID3D11Buffer* m_matrixBuffer;

	void SetRenderTargets(ID3D11DeviceContext* deviceContext)
	{
		// Bind the render target view array and depth stencil buffer to the output render pipeline.
		deviceContext->OMSetRenderTargets(BUFFER_COUNT, m_renderTargetViewArray, m_depthStencilView);

		// Set the viewport.
		deviceContext->RSSetViewports(1, &m_viewport);
	}
	void ClearRenderTargets(ID3D11DeviceContext* deviceContext, float red, float green, float blue, float alpha)
	{
		float color[4];
		int i;


		// Setup the color to clear the buffer to.
		color[0] = red;
		color[1] = green;
		color[2] = blue;
		color[3] = alpha;

		// Clear the render target buffers.
		for (i = 0; i<BUFFER_COUNT; i++)
		{
			deviceContext->ClearRenderTargetView(m_renderTargetViewArray[i], color);
		}

		// Clear the depth buffer.
		deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	void Render(ID3D11DeviceContext* deviceContext, int indexCount, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix,
		D3DXMATRIX projectionMatrix, ID3D11ShaderResourceView* texture)
	{
		HRESULT hr;
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		unsigned int bufferNumber;
		MatrixBufferType* dataPtr;


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

		// Set shader texture resource in the pixel shader.
		deviceContext->PSSetShaderResources(0, 1, &texture);


		// Set the vertex input layout.
		deviceContext->IASetInputLayout(m_layout);

		// Set the vertex and pixel shaders that will be used to render.
		deviceContext->VSSetShader(m_vertexShader, NULL, 0);
		deviceContext->PSSetShader(m_pixelShader, NULL, 0);

		// Set the sampler states in the pixel shader.
		deviceContext->PSSetSamplers(0, 1, &m_sampleStateWrap);

		// Render the geometry.
		deviceContext->DrawIndexed(indexCount, 0, 0);
	}

	void ReleaseDefferred();

	bool InitDeferredBuffers(ID3D11Device* device, int textureWidth, int textureHeight, float screenDepth, float screenNear)
	{
		D3D11_TEXTURE2D_DESC textureDesc;
		HRESULT hr;
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		D3D11_TEXTURE2D_DESC depthBufferDesc;
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		int i;

		ReleaseDefferred();
		// Store the width and height of the render texture.
		m_textureWidth = textureWidth;
		m_textureHeight = textureHeight;

		// Initialize the render target texture description.
		ZeroMemory(&textureDesc, sizeof(textureDesc));

		// Setup the render target texture description.
		textureDesc.Width = textureWidth;
		textureDesc.Height = textureHeight;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		// Create the render target textures.
		for (i = 0; i < BUFFER_COUNT; i++)
		{
			V(device->CreateTexture2D(&textureDesc, NULL, &m_renderTargetTextureArray[i]));
		}

		// Setup the description of the render target view.
		renderTargetViewDesc.Format = textureDesc.Format;
		renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.Texture2D.MipSlice = 0;

		// Create the render target views.
		for (i = 0; i < BUFFER_COUNT; i++)
		{
			V( device->CreateRenderTargetView(m_renderTargetTextureArray[i], &renderTargetViewDesc, &m_renderTargetViewArray[i]));
		}

		// Setup the description of the shader resource view.
		shaderResourceViewDesc.Format = textureDesc.Format;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;

		// Create the shader resource views.
		for (i = 0; i < BUFFER_COUNT; i++)
		{
			V(device->CreateShaderResourceView(m_renderTargetTextureArray[i], &shaderResourceViewDesc, &m_shaderResourceViewArray[i]));
		}

		// Initialize the description of the depth buffer.
		ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

		// Set up the description of the depth buffer.
		depthBufferDesc.Width = textureWidth;
		depthBufferDesc.Height = textureHeight;
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.ArraySize = 1;
		depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthBufferDesc.SampleDesc.Count = 1;
		depthBufferDesc.SampleDesc.Quality = 0;
		depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthBufferDesc.CPUAccessFlags = 0;
		depthBufferDesc.MiscFlags = 0;

		// Create the texture for the depth buffer using the filled out description.
		V( device->CreateTexture2D(&depthBufferDesc, NULL, &m_depthStencilBuffer));

		// Initailze the depth stencil view description.
		ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

		// Set up the depth stencil view description.
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		// Create the depth stencil view.
		V(device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView));

		// Setup the viewport for rendering.
		m_viewport.Width = (float)textureWidth;
		m_viewport.Height = (float)textureHeight;
		m_viewport.MinDepth = 0.0f;
		m_viewport.MaxDepth = 1.0f;
		m_viewport.TopLeftX = 0.0f;
		m_viewport.TopLeftY = 0.0f;

		ID3D10Blob* errorMessage;
		ID3D10Blob* vertexShaderBuffer;
		ID3D10Blob* pixelShaderBuffer;
		D3D11_INPUT_ELEMENT_DESC polygonLayout[3];
		unsigned int numElements;
		D3D11_SAMPLER_DESC samplerDesc;
		D3D11_BUFFER_DESC matrixBufferDesc;


		// Initialize the pointers this function will use to null.
		errorMessage = 0;
		vertexShaderBuffer = 0;
		pixelShaderBuffer = 0;

		// Compile the vertex shader code.
		V(D3DX11CompileFromFile(L"ds_vs.hlsl", NULL, NULL, "DeferredVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL,
			&vertexShaderBuffer, &errorMessage, NULL));

		// Compile the pixel shader code.
		V(D3DX11CompileFromFile(L"ds_ps.hlsl", NULL, NULL, "DeferredPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL,
			&pixelShaderBuffer, &errorMessage, NULL));

		// Create the vertex shader from the buffer.
		V( device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader));

		// Create the pixel shader from the buffer.
		V( device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader));

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

		polygonLayout[2].SemanticName = "NORMAL";
		polygonLayout[2].SemanticIndex = 0;
		polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		polygonLayout[2].InputSlot = 0;
		polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[2].InstanceDataStepRate = 0;

		// Get a count of the elements in the layout.
		numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

		// Create the vertex input layout.
		V( device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(),
			&m_layout));

		// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
		vertexShaderBuffer->Release();
		vertexShaderBuffer = 0;

		pixelShaderBuffer->Release();
		pixelShaderBuffer = 0;

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

		// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
		matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
		matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		matrixBufferDesc.MiscFlags = 0;
		matrixBufferDesc.StructureByteStride = 0;

		// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
		V( device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer));

		return true;
	}


	void ReleaseDefferred()
	{
		int i;


		if (m_depthStencilView)
		{
			m_depthStencilView->Release();
			m_depthStencilView = 0;
		}

		if (m_depthStencilBuffer)
		{
			m_depthStencilBuffer->Release();
			m_depthStencilBuffer = 0;
		}

		for (i = 0; i < BUFFER_COUNT; i++)
		{
			if (m_shaderResourceViewArray[i])
			{
				m_shaderResourceViewArray[i]->Release();
				m_shaderResourceViewArray[i] = 0;
			}

			if (m_renderTargetViewArray[i])
			{
				m_renderTargetViewArray[i]->Release();
				m_renderTargetViewArray[i] = 0;
			}

			if (m_renderTargetTextureArray[i])
			{
				m_renderTargetTextureArray[i]->Release();
				m_renderTargetTextureArray[i] = 0;
			}
		}

		if (m_matrixBuffer)
		{
			m_matrixBuffer->Release();
			m_matrixBuffer = 0;
		}

		// Release the sampler state.
		if (m_sampleStateWrap)
		{
			m_sampleStateWrap->Release();
			m_sampleStateWrap = 0;
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
