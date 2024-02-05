#pragma once

#include "../Common.h"
#include "GraphicsUtils.h"

// "Common.hlsli"와 동일해야 함.
#define MAX_LIGHTS 3
#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SHADOW 0x10

namespace Core
{
	using DirectX::SimpleMath::Matrix;
	using DirectX::SimpleMath::Vector2;
	using DirectX::SimpleMath::Vector3;

	// DirectX-Graphics-Samples/MiniEngine을 따라서 파일이름 변경
	// __declspec(align(256)) : DX12에서는 256 align (예습)

	// 주로 Vertex/Geometry 쉐이더에서 사용.
	ALIGN(16) struct MeshConstants
	{
		Matrix World;
		Matrix WorldInverseTranspose;
		Matrix WorldInverse;
		BOOL bUseHeightMap = FALSE;
		float HeightScale = 0.0f;
		float WindTrunk = 0.0f;
		float WindLeaves = 0.0f;
	};

	// 주로 Pixel 쉐이더에서 사용.
	ALIGN(16) struct MaterialConstants
	{
		Vector3 AlbedoFactor = Vector3(1.0f);
		float RoughnessFactor = 1.0f;
		float MetallicFactor = 1.0f;
		Vector3 EmissionFactor = Vector3(0.0f);

		// 여러 옵션들에 BOOL 플래그 하나만 사용할 수도 있음.
		BOOL bUseAlbedoMap = FALSE;
		BOOL bUseNormalMap = FALSE;
		BOOL bUseAOMap = FALSE;
		BOOL bInvertNormalMapY = FALSE;
		BOOL bUseMetallicMap = FALSE;
		BOOL bUseRoughnessMap = FALSE;
		BOOL bUseEmissiveMap = FALSE;
		float dummy = 0.0f;
	};

	struct Light
	{
		Vector3 Radiance = Vector3(5.0f); // strength.
		float FallOffStart = 0.0f;
		Vector3 Direction = Vector3(0.0f, 0.0f, 1.0f);
		float FallOffEnd = 20.0f;
		Vector3 Position = Vector3(0.0f, 0.0f, -2.0f);
		float SpotPower = 6.0f;

		// Light type bitmasking.
		// ex) LIGHT_SPOT | LIGHT_SHADOW
		uint32_t Type = LIGHT_OFF;
		float Radius = 0.035f; // 반지름.

		float HaloRadius = 0.0f;
		float HaloStrength = 0.0f;

		Matrix ViewProjection;	  // 그림자 렌더링에 필요.
		Matrix InverseProjection; // 그림자 렌더링 디버깅용.
	};

	// register(b1) 사용
	ALIGN(16) struct GlobalConstants
	{
		Matrix View;
		Matrix Projection;
		Matrix InverseProjection;
		Matrix ViewProjection;
		Matrix InverseViewProjection; // Proj -> World
		Matrix InverseView;

		Vector3 EyeWorld;
		float StrengthIBL = 0.0f;
		
		int TextureToDraw = 0;	 // 0: Env, 1: Specular, 2: Irradiance, 그외: 검은색.
		float EnvLODBias = 0.0f; // 환경맵 LodBias.
		float LODBias = 2.0f;    // 다른 물체들 LodBias.
		float GlobalTime = 0.0f;

		Light Lights[MAX_LIGHTS];
	};

	// register(b5) 사용, PostEffectsPS.hlsl
	ALIGN(16) struct PostEffectsConstants
	{
		int Mode = 1; // 1: Rendered image, 2: DepthOnly
		float DepthScale = 1.0f;
		float FogStrength = 0.0f;
	};

	ALIGN(16) struct VolumeConsts
	{
		Vector3 UVWOffset = Vector3(0.0f);
		float LightAbsorption = 5.0f;
		Vector3 LightDir = Vector3(0.0f, 1.0f, 0.0f);
		float DensityAbsorption = 10.0f;
		Vector3 LightColor = Vector3(1, 1, 1) * 40.0f;
		float Aniso = 0.3f;
	};

	// bone 개수 제약을 없애기 위해 StructuredBuffer로 교체.
	// __declspec(align(256)) struct SkinnedConsts {
	//    Matrix pBoneTransforms[52]; // bone 개수
	//};

	template <typename CONSTANTS> 
	class ConstantsBuffer
	{
	public:
		~ConstantsBuffer() { destroy(); }

		void Initialize(ID3D11Device* pDevice)
		{
			HRESULT hr = S_OK;

			destroy();

			hr = Graphics::CreateConstBuffer(pDevice, CPU, &pGPU);
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(pGPU, "ConstantsBuffer::pGPU");
		}

		void Upload(ID3D11DeviceContext* pContext)
		{
			Graphics::UpdateBuffer(pContext, CPU, pGPU);
		}

	protected:
		void destroy()
		{
			SAFE_RELEASE(pGPU);
		}

	public:
		CONSTANTS CPU;
		ID3D11Buffer* pGPU = nullptr;
	};

}
