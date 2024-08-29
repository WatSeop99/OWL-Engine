#include "../Common.h"
#include "Camera.h"
#include "ConstantBuffers.h"
#include "GraphicsCommon.h"
#include "../Geometry/Model.h"
#include "ShadowMap.h"

using DirectX::SimpleMath::Vector4;

void ShadowMap::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, UINT lightType)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);

	m_LightType = lightType;

	D3D11_TEXTURE2D_DESC desc2D = {};
	desc2D.Width = m_ShadowWidth;
	desc2D.Height = m_ShadowHeight;
	desc2D.MipLevels = 1;
	desc2D.ArraySize = 1;
	desc2D.Format = DXGI_FORMAT_D32_FLOAT; // DSV 포맷 기준으로 설정해야 함.
	desc2D.SampleDesc.Count = 1;
	desc2D.SampleDesc.Quality = 0;
	desc2D.Usage = D3D11_USAGE_DEFAULT;
	desc2D.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	desc2D.CPUAccessFlags = 0;
	desc2D.MiscFlags = 0;
	m_SpotLightShadowBuffer.Initialize(pDevice, desc2D);

	HRESULT hr = S_OK;
	{
		// https://www.gamedev.net/forums/topic/659535-cubemap-texture-as-depth-buffer-shadowmapping/5171722/
		D3D11_TEXTURE2D_DESC cubeDesc = {};
		cubeDesc.Width = m_ShadowWidth;
		cubeDesc.Height = m_ShadowHeight;
		cubeDesc.MipLevels = 1;
		cubeDesc.ArraySize = 6;
		cubeDesc.Format = DXGI_FORMAT_R32_TYPELESS; // texture 포맷 기준
		cubeDesc.SampleDesc.Count = 1;
		cubeDesc.SampleDesc.Quality = 0;
		cubeDesc.Usage = D3D11_USAGE_DEFAULT;
		cubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
		cubeDesc.CPUAccessFlags = 0;
		cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		hr = pDevice->CreateTexture2D(&cubeDesc, nullptr, &(m_PointLightShadowBuffer.pTexture));
		BREAK_IF_FAILED(hr);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = cubeDesc.MipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
		hr = pDevice->CreateShaderResourceView(m_PointLightShadowBuffer.pTexture, &srvDesc, &(m_PointLightShadowBuffer.pSRV));
		BREAK_IF_FAILED(hr);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		ZeroMemory(&dsvDesc, sizeof(dsvDesc));
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.ArraySize = 6;
		hr = pDevice->CreateDepthStencilView(m_PointLightShadowBuffer.pTexture, &dsvDesc, &(m_PointLightShadowBuffer.pDSV));
		BREAK_IF_FAILED(hr);
	}

	{
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = m_ShadowWidth;
		textureDesc.Height = m_ShadowHeight;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 4;
		textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;
		hr = pDevice->CreateTexture2D(&textureDesc, nullptr, &(m_DirectionalLightShadowBuffer.pTexture));
		BREAK_IF_FAILED(hr);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Flags = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.ArraySize = 4;
		dsvDesc.Texture2DArray.MipSlice = 0;
		hr = pDevice->CreateDepthStencilView(m_DirectionalLightShadowBuffer.pTexture, &dsvDesc, &(m_DirectionalLightShadowBuffer.pDSV));
		BREAK_IF_FAILED(hr);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = 4;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		hr = pDevice->CreateShaderResourceView(m_DirectionalLightShadowBuffer.pTexture, &srvDesc, &(m_DirectionalLightShadowBuffer.pSRV));
		BREAK_IF_FAILED(hr);
	}

	GlobalConstants initialGlobal;
	ShadowConstants initialShadow;
	for (int i = 0; i < 6; ++i)
	{
		m_pShadowConstantsBuffers[i].Initialize(pDevice, pContext, sizeof(GlobalConstants), &initialGlobal);
	}
	m_ShadowConstantsBufferForGS.Initialize(pDevice, pContext, sizeof(ShadowConstants), &initialShadow);
}

void ShadowMap::Update(ID3D11DeviceContext* pContext, const LightProperty& PROPERTY, Camera& lightCam, Camera& mainCamera)
{
	_ASSERT(pContext);

	Matrix lightView;
	Matrix lightProjection = lightCam.GetProjection();

	switch (m_LightType & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
	{
		case LIGHT_DIRECTIONAL:
		{
			bool bOriginalFPS = mainCamera.bUseFirstPersonView;
			mainCamera.bUseFirstPersonView = true;

			Matrix camView = mainCamera.GetView();
			Matrix camProjection = mainCamera.GetProjection();
			Matrix lightSectionView;
			Matrix lightSectionProjection;
			Vector3 lightSectionPosition;

			for (int i = 0; i < 4; ++i)
			{
				calculateCascadeLightViewProjection(&lightSectionPosition, &lightSectionView, &lightSectionProjection, camView, camProjection, PROPERTY.Direction, i);

				GlobalConstants* pShadowGlobalConstData = (GlobalConstants*)m_pShadowConstantsBuffers[i].pSystemMem;
				ShadowConstants* pShadowConstGSData = (ShadowConstants*)m_ShadowConstantsBufferForGS.pSystemMem;

				pShadowGlobalConstData->EyeWorld = lightSectionPosition;
				pShadowGlobalConstData->View = lightSectionView.Transpose();
				pShadowGlobalConstData->Projection = lightSectionProjection.Transpose();
				pShadowGlobalConstData->InverseProjection = lightSectionProjection.Invert().Transpose();
				pShadowGlobalConstData->ViewProjection = (lightSectionView * lightSectionProjection).Transpose();

				pShadowConstGSData->ViewProjects[i] = pShadowGlobalConstData->ViewProjection;

				m_pShadowConstantsBuffers[i].Upload();
			}

			m_ShadowConstantsBufferForGS.Upload();

			mainCamera.bUseFirstPersonView = bOriginalFPS;
		}
		break;

		case LIGHT_POINT:
		{
			// https://stackoverflow.com/questions/59537726/directx-11-point-light-shadowing
			const Vector3 VIEW_DIRs[6] = // cubemap view vector.
			{
				Vector3(1.0f, 0.0f, 0.0f),	// right
				Vector3(-1.0f, 0.0f, 0.0f), // left
				Vector3(0.0f, 1.0f, 0.0f),	// up
				Vector3(0.0f, -1.0f, 0.0f), // down
				Vector3(0.0f, 0.0f, 1.0f),	// front
				Vector3(0.0f, 0.0f, -1.0f)	// back
			};
			const Vector3 UP_DIRs[6] = // 위에서 정의한 view vector에 대한 up vector.
			{
				Vector3(0.0f, 1.0f, 0.0f),
				Vector3(0.0f, 1.0f, 0.0f),
				Vector3(0.0f, 0.0f, -1.0f),
				Vector3(0.0f, 0.0f, 1.0f),
				Vector3(0.0f, 1.0f, 0.0f),
				Vector3(0.0f, 1.0f, 0.0f)
			};

			for (int i = 0; i < 6; ++i)
			{
				lightView = DirectX::XMMatrixLookAtLH(PROPERTY.Position, PROPERTY.Position + VIEW_DIRs[i], UP_DIRs[i]);

				GlobalConstants* pShadowGlobalConstData = (GlobalConstants*)m_pShadowConstantsBuffers[i].pSystemMem;
				ShadowConstants* pShadowConstGSData = (ShadowConstants*)m_ShadowConstantsBufferForGS.pSystemMem;

				pShadowGlobalConstData->EyeWorld = PROPERTY.Position;
				pShadowGlobalConstData->View = lightView.Transpose();
				pShadowGlobalConstData->Projection = lightProjection.Transpose();
				pShadowGlobalConstData->InverseProjection = lightProjection.Invert().Transpose();
				pShadowGlobalConstData->ViewProjection = (lightView * lightProjection).Transpose();

				pShadowConstGSData->ViewProjects[i] = pShadowGlobalConstData->ViewProjection;

				m_pShadowConstantsBuffers[i].Upload();
			}

			m_ShadowConstantsBufferForGS.Upload();
		}
		break;

		case LIGHT_SPOT:
		{
			lightView = DirectX::XMMatrixLookAtLH(PROPERTY.Position, PROPERTY.Position + PROPERTY.Direction, lightCam.GetUpDir());

			GlobalConstants* pShadowGlobalConstData = (GlobalConstants*)m_pShadowConstantsBuffers[0].pSystemMem;
			pShadowGlobalConstData->EyeWorld = PROPERTY.Position;
			pShadowGlobalConstData->View = lightView.Transpose();
			pShadowGlobalConstData->Projection = lightProjection.Transpose();
			pShadowGlobalConstData->InverseProjection = lightProjection.Invert().Transpose();
			pShadowGlobalConstData->ViewProjection = (lightView * lightProjection).Transpose();

			m_pShadowConstantsBuffers[0].Upload();
		}
		break;

		default:
			break;
	}
}

void ShadowMap::Render(ID3D11DeviceContext* pContext, std::vector<Model*>& pBasicList, Model* pMirror)
{
	setShadowViewport(pContext);

	switch (m_LightType & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
	{
		case LIGHT_DIRECTIONAL:
		{
			pContext->ClearDepthStencilView(m_DirectionalLightShadowBuffer.pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
			pContext->OMSetRenderTargets(0, nullptr, m_DirectionalLightShadowBuffer.pDSV);
			pContext->GSSetConstantBuffers(0, 1, &m_ShadowConstantsBufferForGS.pBuffer);

			for (UINT64 i = 0, size = pBasicList.size(); i < size; ++i)
			{
				Model* const pCurModel = pBasicList[i];
				if (pCurModel->bCastShadow && pCurModel->bIsVisible)
				{
					setPipelineState(pContext, pCurModel->GetDepthOnlyCascadePSO());
					pCurModel->Render(pContext);
				}
			}

			if (pMirror && pMirror->bCastShadow)
			{
				setPipelineState(pContext, g_DepthOnlyCascadePSO);
				pMirror->Render(pContext);
			}
		}
		break;

		case LIGHT_POINT:
		{
			pContext->ClearDepthStencilView(m_PointLightShadowBuffer.pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
			pContext->OMSetRenderTargets(0, nullptr, m_PointLightShadowBuffer.pDSV);
			pContext->GSSetConstantBuffers(0, 1, &m_ShadowConstantsBufferForGS.pBuffer);

			for (UINT64 i = 0, size = pBasicList.size(); i < size; ++i)
			{
				Model* const pCurModel = pBasicList[i];
				if (pCurModel->bCastShadow && pCurModel->bIsVisible)
				{
					setPipelineState(pContext, pCurModel->GetDepthOnlyCubePSO());
					pCurModel->Render(pContext);
				}
			}

			if (pMirror && pMirror->bCastShadow)
			{
				setPipelineState(pContext, g_DepthOnlyCubePSO);
				pMirror->Render(pContext);
			}
		}
		break;

		case LIGHT_SPOT:
		{
			pContext->ClearDepthStencilView(m_SpotLightShadowBuffer.pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
			pContext->OMSetRenderTargets(0, nullptr, m_SpotLightShadowBuffer.pDSV);
			pContext->VSSetConstantBuffers(0, 1, &m_pShadowConstantsBuffers[0].pBuffer);
			pContext->PSSetConstantBuffers(0, 1, &m_pShadowConstantsBuffers[0].pBuffer);
			pContext->GSSetConstantBuffers(0, 1, &m_pShadowConstantsBuffers[0].pBuffer);

			for (UINT64 i = 0, size = pBasicList.size(); i < size; ++i)
			{
				Model* const pCurModel = pBasicList[i];
				if (pCurModel->bCastShadow && pCurModel->bIsVisible)
				{
					setPipelineState(pContext, pCurModel->GetDepthOnlyPSO());
					pCurModel->Render(pContext);
				}
			}

			if (pMirror && pMirror->bCastShadow)
			{
				pMirror->Render(pContext);
			}
		}
		break;

		default:
			break;
	}
}

void ShadowMap::Cleanup()
{
	m_SpotLightShadowBuffer.Cleanup();
	m_PointLightShadowBuffer.Cleanup();
	m_DirectionalLightShadowBuffer.Cleanup();
	m_ShadowConstantsBufferForGS.Cleanup();
	for (int i = 0; i < 6; ++i)
	{
		m_pShadowConstantsBuffers[i].Cleanup();
	}
}

void ShadowMap::setPipelineState(ID3D11DeviceContext* pContext, const GraphicsPSO& PSO)
{
	_ASSERT(pContext);

	pContext->VSSetShader(PSO.pVertexShader, nullptr, 0);
	pContext->PSSetShader(PSO.pPixelShader, nullptr, 0);
	pContext->HSSetShader(PSO.pHullShader, nullptr, 0);
	pContext->DSSetShader(PSO.pDomainShader, nullptr, 0);
	pContext->GSSetShader(PSO.pGeometryShader, nullptr, 0);
	pContext->CSSetShader(nullptr, nullptr, 0);
	pContext->IASetInputLayout(PSO.pInputLayout);
	pContext->RSSetState(PSO.pRasterizerState);
	pContext->OMSetBlendState(PSO.pBlendState, PSO.BlendFactor, 0xffffffff);
	pContext->OMSetDepthStencilState(PSO.pDepthStencilState, PSO.StencilRef);
	pContext->IASetPrimitiveTopology(PSO.PrimitiveTopology);
}

void ShadowMap::setShadowViewport(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);

	switch (m_LightType & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
	{
		case LIGHT_DIRECTIONAL:
		{
			const D3D11_VIEWPORT pViewports[4] =
			{
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
			};
			pContext->RSSetViewports(4, pViewports);
		}
		break;

		case LIGHT_POINT:
		{
			const D3D11_VIEWPORT pViewports[6] =
			{
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
			};
			pContext->RSSetViewports(6, pViewports);
		}
		break;

		case LIGHT_SPOT:
		{
			D3D11_VIEWPORT shadowViewport = { 0, };
			shadowViewport.TopLeftX = 0;
			shadowViewport.TopLeftY = 0;
			shadowViewport.Width = (float)m_ShadowWidth;
			shadowViewport.Height = (float)m_ShadowHeight;
			shadowViewport.MinDepth = 0.0f;
			shadowViewport.MaxDepth = 1.0f;

			pContext->RSSetViewports(1, &shadowViewport);
		}
		break;

		default:
			break;
	}
}

void ShadowMap::calculateCascadeLightViewProjection(Vector3* pPosition, Matrix* pView, Matrix* pProjection, const Matrix& VIEW, const Matrix& PROJECTION, const Vector3& DIR, int cascadeIndex)
{
	_ASSERT(pPosition != nullptr);
	_ASSERT(pView != nullptr);
	_ASSERT(pProjection != nullptr);

	const float FRUSTUM_Zs[5] = { 0.01f, 10.0f, 40.0f, 80.0f, 500.0f }; // 고정 값들로 우선 설정.
	Matrix inverseView = VIEW.Invert();
	Vector3 frustumCenter(0.0f);
	float boundingSphereRadius = 0.0f;

	float fov = 45.0f;
	float aspectRatio = 1270.0f / 720.0f;
	float nearZ = FRUSTUM_Zs[0];
	float farZ = FRUSTUM_Zs[4];
	float tanHalfVFov = tanf(DirectX::XMConvertToRadians(fov * 0.5f)); // 수직 시야각.
	float tanHalfHFov = tanHalfVFov * aspectRatio; // 수평 시야각.

	float xn = FRUSTUM_Zs[cascadeIndex] * tanHalfHFov;
	float xf = FRUSTUM_Zs[cascadeIndex + 1] * tanHalfHFov;
	float yn = FRUSTUM_Zs[cascadeIndex] * tanHalfVFov;
	float yf = FRUSTUM_Zs[cascadeIndex + 1] * tanHalfVFov;

	Vector3 frustumCorners[8] =
	{
		Vector3(xn, yn, FRUSTUM_Zs[cascadeIndex]),
		Vector3(-xn, yn, FRUSTUM_Zs[cascadeIndex]),
		Vector3(xn, -yn, FRUSTUM_Zs[cascadeIndex]),
		Vector3(-xn, -yn, FRUSTUM_Zs[cascadeIndex]),
		Vector3(xf, yf, FRUSTUM_Zs[cascadeIndex + 1]),
		Vector3(-xf, yf, FRUSTUM_Zs[cascadeIndex + 1]),
		Vector3(xf, -yf, FRUSTUM_Zs[cascadeIndex + 1]),
		Vector3(-xf, -yf, FRUSTUM_Zs[cascadeIndex + 1]),
	};

	for (int i = 0; i < 8; ++i)
	{
		frustumCorners[i] = Vector3::Transform(frustumCorners[i], inverseView);
		frustumCenter += frustumCorners[i];
	}
	frustumCenter /= 8.0f;

	for (int i = 0; i < 8; ++i)
	{
		float dist = (frustumCorners[i] - frustumCenter).Length();
		boundingSphereRadius = Max(boundingSphereRadius, dist);
	}
	boundingSphereRadius = ceil(boundingSphereRadius * 16.0f) / 16.0f;

	Vector3 frustumMax(boundingSphereRadius);
	Vector3 frustumMin = -frustumMax;
	Vector3 cascadeExtents = frustumMax - frustumMin;

	*pPosition = frustumCenter - DIR * fabs(frustumMin.z);
	*pView = DirectX::XMMatrixLookAtLH(*pPosition, frustumCenter, Vector3::UnitY);
	*pProjection = DirectX::XMMatrixOrthographicOffCenterLH(frustumMin.x, frustumMax.x, frustumMin.y, frustumMax.y, 0.001f, cascadeExtents.z);
}
