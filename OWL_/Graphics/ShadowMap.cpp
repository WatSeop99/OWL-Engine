#include "../Common.h"
#include "../Renderer/BaseRenderer.h"
#include "Camera.h"
#include "ConstantDataType.h"
#include "../Geometry/Model.h"
#include "ShadowMap.h"

using DirectX::SimpleMath::Vector4;

void ShadowMap::Initialize(BaseRenderer* pRenderer, const UINT LIGHT_TYPE)
{
	_ASSERT(pRenderer);

	m_pRenderer = pRenderer;
	m_LightType = LIGHT_TYPE;

	ID3D11Device* pDevice = pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = pRenderer->GetDeviceContext();
	int shadowBufferCount = 0;

	switch (LIGHT_TYPE & m_TOTAL_LIGHT_TYPE)
	{
		case LIGHT_SUN:
		{
			D3D11_TEXTURE2D_DESC textureDesc = {};
			textureDesc.Width = m_ShadowWidth;
			textureDesc.Height = m_ShadowHeight;
			textureDesc.MipLevels = 1;
			textureDesc.ArraySize = 4;
			textureDesc.Format = DXGI_FORMAT_D32_FLOAT;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;
			textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
			textureDesc.CPUAccessFlags = 0;
			textureDesc.MiscFlags = 0;
			m_CascadeShadowBuffer.Initialize(pDevice, pContext, textureDesc, nullptr, false);

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Flags = 0;
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.ArraySize = 4;
			dsvDesc.Texture2DArray.MipSlice = 0;
			m_CascadeShadowBuffer.CreateDSV(dsvDesc);

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = 4;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			m_CascadeShadowBuffer.CreateSRV(srvDesc);

			shadowBufferCount = 4;
		}
			break;

		case LIGHT_POINT:
		{
			// https://www.gamedev.net/forums/topic/659535-cubemap-texture-as-depth-buffer-shadowmapping/5171722/
			D3D11_TEXTURE2D_DESC cubeDesc = {};
			cubeDesc.Width = m_ShadowWidth;
			cubeDesc.Height = m_ShadowHeight;
			cubeDesc.MipLevels = 1;
			cubeDesc.ArraySize = 6;
			cubeDesc.Format = DXGI_FORMAT_D32_FLOAT; // DSV 포맷 기준
			cubeDesc.SampleDesc.Count = 1;
			cubeDesc.SampleDesc.Quality = 0;
			cubeDesc.Usage = D3D11_USAGE_DEFAULT;
			cubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
			cubeDesc.CPUAccessFlags = 0;
			cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
			m_ShadowCubeBuffer.Initialize(pDevice, pContext, cubeDesc, nullptr, false);

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = cubeDesc.MipLevels;
			srvDesc.TextureCube.MostDetailedMip = 0;
			m_ShadowCubeBuffer.CreateSRV(srvDesc);

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.MipSlice = 0;
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.ArraySize = 6;
			m_ShadowCubeBuffer.CreateDSV(dsvDesc);

			shadowBufferCount = 6;
		}
			break;

		case LIGHT_DIRECTIONAL:
		case LIGHT_SPOT:
		{
			D3D11_TEXTURE2D_DESC textureDesc = {};
			textureDesc.Width = m_ShadowWidth;
			textureDesc.Height = m_ShadowHeight;
			textureDesc.MipLevels = 1;
			textureDesc.ArraySize = 1;
			textureDesc.Format = DXGI_FORMAT_D32_FLOAT; // DSV 포맷 기준으로 설정해야 함.
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;
			textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
			textureDesc.CPUAccessFlags = 0;
			textureDesc.MiscFlags = 0;
			m_Shadow2DBuffer.Initialize(pDevice, pContext, textureDesc, nullptr, true);

			shadowBufferCount = 1;
		}
		break;

		default:
			break;
	}

	GlobalConstants initialGlobal;
	ShadowConstants initialShadow;
	for (int i = 0; i < shadowBufferCount; ++i)
	{
		m_pShadowConstantsBuffers[i].Initialize(pDevice, pContext, sizeof(GlobalConstants), &initialGlobal);
	}
	if ((LIGHT_TYPE & m_TOTAL_LIGHT_TYPE) & (LIGHT_SUN | LIGHT_POINT))
	{
		m_ShadowConstantsBufferForGS.Initialize(pDevice, pContext, sizeof(ShadowConstants), &initialShadow);
	}
}

void ShadowMap::Update(const LightProperty& PROPERTY, Camera* pLightCam, Camera* pMainCamera)
{
	Matrix lightView;
	Matrix lightProjection = pLightCam->GetProjection();

	switch (m_LightType & m_TOTAL_LIGHT_TYPE)
	{
		case LIGHT_SUN:
		{
			bool bOriginalFPS = pMainCamera->bUseFirstPersonView;
			pMainCamera->bUseFirstPersonView = true;

			Matrix camView = pMainCamera->GetView();
			Matrix camProjection = pMainCamera->GetProjection();
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

			pMainCamera->bUseFirstPersonView = bOriginalFPS;
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
			lightView = DirectX::XMMatrixLookAtLH(PROPERTY.Position, PROPERTY.Position + PROPERTY.Direction, pLightCam->GetUpDir());

			GlobalConstants* pShadowGlobalConstData = (GlobalConstants*)m_pShadowConstantsBuffers[0].pSystemMem;
			pShadowGlobalConstData->EyeWorld = PROPERTY.Position;
			pShadowGlobalConstData->View = lightView.Transpose();
			pShadowGlobalConstData->Projection = lightProjection.Transpose();
			pShadowGlobalConstData->InverseProjection = lightProjection.Invert().Transpose();
			pShadowGlobalConstData->ViewProjection = (lightView * lightProjection).Transpose();

			m_pShadowConstantsBuffers[0].Upload();
		}
		break;

		case LIGHT_DIRECTIONAL:
		{
			bool bOriginalFPS = pMainCamera->bUseFirstPersonView;
			pMainCamera->bUseFirstPersonView = true;

			lightProjection = pLightCam->GetProjection();
			lightView = DirectX::XMMatrixLookAtLH(PROPERTY.Position, PROPERTY.Position + PROPERTY.Direction, pLightCam->GetUpDir());

			GlobalConstants* pShadowGlobalConstData = (GlobalConstants*)m_pShadowConstantsBuffers[0].pSystemMem;
			pShadowGlobalConstData->EyeWorld = PROPERTY.Position;
			pShadowGlobalConstData->View = lightView.Transpose();
			pShadowGlobalConstData->Projection = lightProjection.Transpose();
			pShadowGlobalConstData->InverseProjection = lightProjection.Invert().Transpose();
			pShadowGlobalConstData->ViewProjection = (lightView * lightProjection).Transpose();

			m_pShadowConstantsBuffers[0].Upload();

			pMainCamera->bUseFirstPersonView = bOriginalFPS;
		}
		break;

		default:
			break;
	}
}

void ShadowMap::Render(std::vector<Model*>& pBasicList, Model* pMirror)
{
	_ASSERT(m_pRenderer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	setShadowViewport();

	switch (m_LightType & m_TOTAL_LIGHT_TYPE)
	{
		case LIGHT_SUN:
		{
			pContext->ClearDepthStencilView(m_CascadeShadowBuffer.pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
			pContext->OMSetRenderTargets(0, nullptr, m_CascadeShadowBuffer.pDSV);
			pContext->GSSetConstantBuffers(0, 1, &m_ShadowConstantsBufferForGS.pBuffer);

			for (UINT64 i = 0, size = pBasicList.size(); i < size; ++i)
			{
				Model* const pCurModel = pBasicList[i];
				if (pCurModel->bCastShadow && pCurModel->bIsVisible)
				{
					pResourceManager->SetPipelineState(pCurModel->GetDepthOnlyCascadePSO());
					pCurModel->Render();
				}
			}

			if (pMirror && pMirror->bCastShadow)
			{
				pResourceManager->SetPipelineState(GraphicsPSOType_DepthOnlyCascade);
				pMirror->Render();
			}
		}
		break;

		case LIGHT_POINT:
		{
			pContext->ClearDepthStencilView(m_ShadowCubeBuffer.pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
			pContext->OMSetRenderTargets(0, nullptr, m_ShadowCubeBuffer.pDSV);
			pContext->GSSetConstantBuffers(0, 1, &m_ShadowConstantsBufferForGS.pBuffer);

			for (UINT64 i = 0, size = pBasicList.size(); i < size; ++i)
			{
				Model* const pCurModel = pBasicList[i];
				if (pCurModel->bCastShadow && pCurModel->bIsVisible)
				{
					pResourceManager->SetPipelineState(pCurModel->GetDepthOnlyCubePSO());
					pCurModel->Render();
				}
			}

			if (pMirror && pMirror->bCastShadow)
			{
				pResourceManager->SetPipelineState(GraphicsPSOType_DepthOnlyCube);
				pMirror->Render();
			}
		}
		break;

		case LIGHT_DIRECTIONAL:
		case LIGHT_SPOT:
		{
			pContext->ClearDepthStencilView(m_Shadow2DBuffer.pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
			pContext->OMSetRenderTargets(0, nullptr, m_Shadow2DBuffer.pDSV);
			m_pRenderer->SetGlobalConsts(&m_pShadowConstantsBuffers[0].pBuffer, 0);

			for (UINT64 i = 0, size = pBasicList.size(); i < size; ++i)
			{
				Model* const pCurModel = pBasicList[i];
				if (pCurModel->bCastShadow && pCurModel->bIsVisible)
				{
					pResourceManager->SetPipelineState(pCurModel->GetDepthOnlyPSO());
					pCurModel->Render();
				}
			}

			if (pMirror && pMirror->bCastShadow)
			{
				pResourceManager->SetPipelineState(GraphicsPSOType_DepthOnly);
				pMirror->Render();
			}
		}
		break;

		default:
			break;
	}
}

void ShadowMap::Cleanup()
{
	m_Shadow2DBuffer.Cleanup();
	m_ShadowCubeBuffer.Cleanup();
	m_CascadeShadowBuffer.Cleanup();
	m_ShadowConstantsBufferForGS.Cleanup();
	for (int i = 0; i < 6; ++i)
	{
		m_pShadowConstantsBuffers[i].Cleanup();
	}

	m_pRenderer = nullptr;
}

void ShadowMap::setShadowViewport()
{
	_ASSERT(m_pRenderer);



	switch (m_LightType & m_TOTAL_LIGHT_TYPE)
	{
		case LIGHT_SUN:
		{
			const D3D11_VIEWPORT pViewports[4] =
			{
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
				{ 0, 0, (float)m_ShadowWidth, (float)m_ShadowHeight, 0.0f, 1.0f },
			};
			m_pRenderer->SetViewport(pViewports, 4);
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
			m_pRenderer->SetViewport(pViewports, 6);
		}
		break;

		case LIGHT_DIRECTIONAL:
		case LIGHT_SPOT:
		{
			D3D11_VIEWPORT shadowViewport = { 0, };
			shadowViewport.TopLeftX = 0;
			shadowViewport.TopLeftY = 0;
			shadowViewport.Width = (float)m_ShadowWidth;
			shadowViewport.Height = (float)m_ShadowHeight;
			shadowViewport.MinDepth = 0.0f;
			shadowViewport.MaxDepth = 1.0f;

			m_pRenderer->SetViewport(&shadowViewport, 1);
		}
		break;

		default:
			break;
	}
}

void ShadowMap::calculateCascadeLightViewProjection(Vector3* pPosition, Matrix* pView, Matrix* pProjection, const Matrix& VIEW, const Matrix& PROJECTION, const Vector3& DIR, int cascadeIndex)
{
	_ASSERT(pPosition);
	_ASSERT(pView);
	_ASSERT(pProjection);

	const float FRUSTUM_Zs[5] = { 0.01f, 10.0f, 40.0f, 80.0f, 500.0f }; // 고정 값들로 우선 설정.
	Matrix inverseView = VIEW.Invert();
	Vector3 frustumCenter;
	float boundingSphereRadius = 0.0f;

	float fov = 45.0f;
	float aspectRatio = 1280.0f / 720.0f;
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
	*pProjection = DirectX::XMMatrixOrthographicOffCenterLH(frustumMin.x, frustumMax.x, frustumMin.y, frustumMax.y, 0.01f, cascadeExtents.z);
}
