#pragma once
#include <d3d11.h>
#include <d3dx11async.h>
#include "DXUT.h"
#include "deferred.h"

namespace shadow
{
	ID3D11VertexShader *m_vertexShader;
	ID3D11PixelShader *m_pixelShader;
	ID3D11InputLayout *m_layout;
	ID3D11Buffer *m_matrixBuffer;

	int m_textureWidth = 2048;
	int m_textureHeight = 2048;
	ID3D11Texture2D *m_renderTargetTexture;
	ID3D11RenderTargetView *m_renderTargetView;
	ID3D11ShaderResourceView *m_shaderResourceView;
	D3D11_VIEWPORT m_viewport;

	D3DXVECTOR3 lightPosition;
	D3DXVECTOR3 lookat = D3DXVECTOR3(0, 0, 0);
	D3DXVECTOR3 up = D3DXVECTOR3(0, 1, 0);

	D3DXMATRIX GetViewMatrix()
	{
		D3DXMATRIX viewMatrix;
		D3DXMatrixLookAtLH(&viewMatrix, &lightPosition, &lookat, &up);
		return viewMatrix;
	}

	D3DXMATRIX GetProjMatrix(float screenDepth = 500, float screenNear = 0.005f)
	{
		float fieldOfView, screenAspect;
		D3DXMATRIX projectionMatrix;


		// Setup field of view and screen aspect for a square light source.
		fieldOfView = (float)D3DX_PI / 2.0f;
		screenAspect = 1.0f;

		// Create the projection matrix for the light.
		D3DXMatrixPerspectiveFovLH(&projectionMatrix, fieldOfView, screenAspect, screenNear, screenDepth);
		return projectionMatrix;
	}

	struct MatrixBufferType
	{
		D3DXMATRIX world;
		D3DXMATRIX view;
		D3DXMATRIX projection;
	};

	void SetRenderTargets(ID3D11DeviceContext *deviceContext)
	{
		// Bind the render target view array and depth stencil buffer to the output render pipeline.
		deviceContext->OMSetRenderTargets(1, &m_renderTargetView, DXUTGetD3D11DepthStencilView());

		// Set the viewport.
		deviceContext->RSSetViewports(1, &m_viewport);
	}

	void ClearRenderTargets(ID3D11DeviceContext *deviceContext, float red, float green, float blue, float alpha)
	{
		float color[4];
		int i;


		// Setup the color to clear the buffer to.
		color[0] = red;
		color[1] = green;
		color[2] = blue;
		color[3] = alpha;

		// Clear the render target buffers.
		deviceContext->ClearRenderTargetView(m_renderTargetView, color);

		// Clear the depth buffer.
		deviceContext->ClearDepthStencilView(DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		deviceContext->ClearDepthStencilView(DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	void Render(ID3D11DeviceContext *deviceContext, int indexCount, D3DXMATRIX worldMatrix)
	{
		SetRenderTargets(deviceContext);
		ClearRenderTargets(deviceContext, 0, 0, 0, 1);
		HRESULT hr;
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		unsigned int bufferNumber;
		MatrixBufferType *dataPtr;
		D3DXMATRIX viewMatrix = GetViewMatrix();
		D3DXMATRIX projectionMatrix = GetProjMatrix();

		// Transpose the matrices to prepare them for the shader.
		D3DXMatrixTranspose(&worldMatrix, &worldMatrix);
		D3DXMatrixTranspose(&viewMatrix, &viewMatrix);
		D3DXMatrixTranspose(&projectionMatrix, &projectionMatrix);

		// Lock the constant buffer so it can be written to.
		V(deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

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
		//		deviceContext->PSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);


		// Set the vertex input layout.
		deviceContext->IASetInputLayout(m_layout);

		// Set the vertex and pixel shaders that will be used to render.
		deviceContext->VSSetShader(m_vertexShader, NULL, 0);
		deviceContext->PSSetShader(m_pixelShader, NULL, 0);

		// Render the geometry.
		//		deviceContext->DrawIndexed(indexCount, 0, 0);
		deviceContext->DrawIndexedInstanced(indexCount, LENGTH * LENGTH, 0, 0, 0);
	}


	void InitShadow(ID3D11Device *device)
	{
		HRESULT hr;
		ID3D10Blob *errorMessage;
		ID3D10Blob *vertexShaderBuffer;
		ID3D10Blob *pixelShaderBuffer;
		V(D3DX11CompileFromFile(L"depth_vs.hlsl", NULL, NULL, "DepthVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL,
			&vertexShaderBuffer, &errorMessage, NULL));
		V(D3DX11CompileFromFile(L"depth_ps.hlsl", NULL, NULL, "DepthPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL,
			&pixelShaderBuffer, &errorMessage, NULL));
		V(device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader));
		V(device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader));

		D3D11_INPUT_ELEMENT_DESC polygonLayout[4];

		// Create the vertex input layout description.
		// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
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
		polygonLayout[1].AlignedByteOffset = 12;
		polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[1].InstanceDataStepRate = 0;

		polygonLayout[2].SemanticName = "NORMAL";
		polygonLayout[2].SemanticIndex = 0;
		polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		polygonLayout[2].InputSlot = 0;
		polygonLayout[2].AlignedByteOffset = 20;
		polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[2].InstanceDataStepRate = 0;

		polygonLayout[3].SemanticName = "TEXCOORD";
		polygonLayout[3].SemanticIndex = 1;
		polygonLayout[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		polygonLayout[3].InputSlot = 1;
		polygonLayout[3].AlignedByteOffset = 0;
		polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
		polygonLayout[3].InstanceDataStepRate = 1;

		// Get a count of the elements in the layout.
		auto numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
		V(device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_layout));

		// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
		vertexShaderBuffer->Release();
		vertexShaderBuffer = 0;

		pixelShaderBuffer->Release();
		pixelShaderBuffer = 0;

		// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
		D3D11_BUFFER_DESC matrixBufferDesc;

		matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
		matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		matrixBufferDesc.MiscFlags = 0;
		matrixBufferDesc.StructureByteStride = 0;

		// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
		V(device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer));

		lightPosition = D3DXVECTOR3(0, 0, -15);
		lookat = D3DXVECTOR3(0, 0, 0);
		up = D3DXVECTOR3(0, 1, 0);
		D3D11_TEXTURE2D_DESC textureDesc;
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

		// Initialize the render target texture description.
		ZeroMemory(&textureDesc, sizeof(textureDesc));

		// Setup the render target texture description.
		textureDesc.Width = m_textureWidth;
		textureDesc.Height = m_textureHeight;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		// Create the render target textures.
		V(device->CreateTexture2D(&textureDesc, NULL, &m_renderTargetTexture));

		// Setup the description of the render target view.
		renderTargetViewDesc.Format = textureDesc.Format;
		renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.Texture2D.MipSlice = 0;

		// Create the render target views.
		V(device->CreateRenderTargetView(m_renderTargetTexture, &renderTargetViewDesc, &m_renderTargetView));

		// Setup the description of the shader resource view.
		shaderResourceViewDesc.Format = textureDesc.Format;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;

		// Create the shader resource views.
		V(device->CreateShaderResourceView(m_renderTargetTexture, &shaderResourceViewDesc, &m_shaderResourceView));


		// Setup the viewport for rendering.
		m_viewport.Width = (float)m_textureWidth;
		m_viewport.Height = (float)m_textureHeight;
		m_viewport.MinDepth = 0.0f;
		m_viewport.MaxDepth = 1.0f;
		m_viewport.TopLeftX = 0.0f;
		m_viewport.TopLeftY = 0.0f;
	}

	void ReleaseShadow()
	{
		// Release the matrix constant buffer.
		if (m_matrixBuffer)
		{
			m_matrixBuffer->Release();
			m_matrixBuffer = 0;
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

		SAFE_RELEASE(m_renderTargetTexture);
		SAFE_RELEASE(m_renderTargetView);
		SAFE_RELEASE(m_shaderResourceView);
	}
}
