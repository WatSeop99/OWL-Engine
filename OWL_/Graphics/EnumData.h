#pragma once

enum eGraphicsPSOType
{
	GraphicsPSOType_DefaultSolid = 0,
	GraphicsPSOType_SkinnedSolid,
	GraphicsPSOType_DefaultWire,
	GraphicsPSOType_SkinnedWire,
	GraphicsPSOType_StencilMask,
	GraphicsPSOType_ReflectSolid,
	GraphicsPSOType_ReflectSkinnedSolid,
	GraphicsPSOType_ReflectWire,
	GraphicsPSOType_ReflectSkinnedWire,
	GraphicsPSOType_MirrorBlendSolid,
	GraphicsPSOType_MirrorBlendWire,
	GraphicsPSOType_SkyboxSolid,
	GraphicsPSOType_SkyboxWire,
	GraphicsPSOType_ReflectSkyboxSolid,
	GraphicsPSOType_ReflectSkyboxWire,
	GraphicsPSOType_Normal,
	GraphicsPSOType_DepthOnly,
	GraphicsPSOType_DepthOnlySkinned,
	GraphicsPSOType_DepthOnlyCube,
	GraphicsPSOType_DepthOnlyCubeSkinned,
	GraphicsPSOType_DepthOnlyCascade,
	GraphicsPSOType_DepthOnlyCascadeSkinned,
	GraphicsPSOType_PostEffects,
	GraphicsPSOType_PostProcessing,
	GraphicsPSOType_BoundingBox,
	GraphicsPSOType_GrassSolid,
	GraphicsPSOType_GrassWire,
	GraphicsPSOType_Ocean,
	GraphicsPSOType_GBuffer,
	GraphicsPSOType_GBufferSkinned,
	GraphicsPSOType_GBufferWire,
	GraphicsPSOType_GBufferSkinnedWire,
	GraphicsPSOType_DeferredRendering,
	GraphicsPSOType_SkyLUT,
	GraphicsPSOType_Sky,
	GraphicsPSOType_Sun,
	GraphicsPSOType_VolumeSmoke,
	GraphicsPSOType_Count
};
enum eComputePSOType
{
	ComputePSOType_AerialLUT = 0,
	ComputePSOType_MultiScatterLUT,
	ComputePSOType_TransmittanceLUT,
	ComputePSOType_Count
};

enum ePipelineStage
{
	PipelineStage_VS = 0,
	PipelineStage_GS,
	PipelineStage_PS,
	PipelineStage_CS,
	PipelineStage_Count
};