//#include "Common.hlsli"
//#include "ShadingUtil.hlsli"

//// https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/pbr.hlsl

//// �޽� ���� �ؽ���� t0 ���� ����.
//Texture2D g_AlbedoTex : register(t0);
//Texture2D g_EmissiveTex : register(t1);
//Texture2D g_NormalTex : register(t2);
//Texture2D g_AmbientOcclusionTex : register(t3);
//Texture2D g_MetallicTex : register(t4);
//Texture2D g_RoughnessTex : register(t5);

//struct PixelShaderOutput
//{
//    float4 PixelColor : SV_TARGET0;
//};


//// Assuming that LIGHT_FRUSTUM_WIDTH == LIGHT_FRUSTUM_HEIGHT
//// #define LIGHT_RADIUS_UV (LIGHT_WORLD_RADIUS / LIGHT_FRUSTUM_WIDTH)

//float3 GetNormal(PixelShaderInput input)
//{
//    float3 normalWorld = normalize(input.WorldNormal);
    
//    if (bUseNormalMap)
//    {
//        float3 normal = g_NormalTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias).rgb;
//        normal = 2.0f * normal - 1.0f; // ���� ���� [-1.0, 1.0]

//        // OpenGL �� ��ָ��� ��쿡�� y ������ ��������.
//        normal.y = (bInvertNormalMapY ? -normal.y : normal.y);
        
//        float3 N = normalWorld;
//        float3 T = normalize(input.WorldTangent - dot(input.WorldTangent, N) * N);
//        float3 B = cross(N, T);
        
//        // matrix�� float4x4, ���⼭�� ���� ��ȯ���̶� 3x3 ���.
//        float3x3 TBN = float3x3(T, B, N);
//        normalWorld = normalize(mul(normal, TBN));
//    }
    
//    return normalWorld;
//}

//// ����: https://github.com/opengl-tutorials/ogl/blob/master/tutorial16_shadowmaps/ShadowMapping.fragmentshader
//float random(float3 seed, int i)
//{
//    float4 seed4 = float4(seed, i);
//    float dotProduct = dot(seed4, float4(12.9898f, 78.233f, 45.164f, 94.673f));
    
//    return frac(sin(dotProduct) * 43758.5453f);
//}

//float3 LightRadiance(Light light, int shadowMapIndex, float3 representativePoint, float3 posWorld, float3 normalWorld)
//{
//    // Directional light.
//    float3 lightVec = (light.Type & LIGHT_DIRECTIONAL ? -light.Direction : representativePoint - posWorld); // light.position - posWorld;
//    float lightDist = length(lightVec);
//    lightVec /= lightDist;

//    // Spot light.
//    float spotFator = (light.Type & LIGHT_SPOT ? pow(max(-dot(lightVec, light.Direction), 0.0f), light.SpotPower) : 1.0f);
        
//    // Distance attenuation.
//    float att = saturate((light.FallOffEnd - lightDist) / (light.FallOffEnd - light.FallOffStart));

//    // Shadow map.
//    float shadowFactor = 1.0f;

//    if (light.Type & LIGHT_SHADOW)
//    {
//        float4 lightScreen = float4(0.0f, 0.0f, 0.0f, 0.0f);
//        float3 lightTexcoord = float3(0.0f, 0.0f, 0.0f);
//        float radiusScale = 0.5f; // ������ �������� Ű���� �� ������ �� ����.
        
//        switch (light.Type & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
//        {
//            case LIGHT_DIRECTIONAL:
//                {
//                    int index = -1;
//                    lightScreen = float4(0.0f, 0.0f, 0.0f, 0.0f);
                    
//                    for (int i = 0; i < 4; ++i)
//                    {
//                        lightScreen = mul(float4(posWorld, 1.0f), light.ViewProjection[i]);
//                        lightScreen.xyz /= lightScreen.w;
//                        if (lightScreen.z > 1.0f)
//                        {
//                            continue;
//                        }
                    
//                        lightTexcoord.xy = float2(lightScreen.x, -lightScreen.y);
//                        lightTexcoord.xy += 1.0f;
//                        lightTexcoord.xy *= 0.5f;
                    
//                        float depth = g_CascadeShadowMap.SampleLevel(g_ShadowPointSampler, float3(lightTexcoord.xy, i), 0.0f).r;
//                        if (depth <= lightScreen.z - 0.005f || depth >= lightScreen.z + 0.005f)
//                        {
//                            index = i;
//                            break;
//                        }
//                    }
                    
//                    if (index != -1)
//                    {
//                        shadowFactor = PCSSForDirectionalLight(g_CascadeShadowMap, g_ShadowPointSampler, g_ShadowCompareSampler, index, lightTexcoord.xy, lightScreen.z - 0.001f, light.InverseProjections[index], light.Radius * radiusScale);
//                    }
//                }
//                break;
            
//            case LIGHT_POINT:
//                {
//                    const float3 VIEW_DIRs[6] =
//                    {
//                        float3(1.0f, 0.0f, 0.0f), // right
//			            float3(-1.0f, 0.0f, 0.0f), // left
//			            float3(0.0f, 1.0f, 0.0f), // up
//			            float3(0.0f, -1.0f, 0.0f), // down
//			            float3(0.0f, 0.0f, 1.0f), // front
//			            float3(0.0f, 0.0f, -1.0f) // back 
//                    };
//                    int index = 0;
//                    float maxDotProduct = -2.0f;
//                    float3 lightToPos = normalize(posWorld - light.Position);
                    
//                    for (int i = 0; i < 6; ++i)
//                    {
//                        float curDot = dot(lightToPos, VIEW_DIRs[i]);
//                        if (maxDotProduct < curDot)
//                        {
//                            maxDotProduct = curDot;
//                            index = i;
//                        }
//                    }
        
//                    lightScreen = mul(float4(posWorld, 1.0f), light.ViewProjection[index]);
//                    lightScreen.xyz /= lightScreen.w;
        
//                    lightTexcoord = lightToPos;
        
//                    shadowFactor = PCSSForPointLight(g_PointLightShadowMap, g_ShadowPointSampler, g_ShadowCompareSampler, lightTexcoord, lightScreen.z - 0.001f, light.InverseProjections[0], light.Radius * radiusScale);
//                }
//                break;
            
//            case LIGHT_SPOT:
//                {
//                    // Project posWorld to light screen.  
//                    lightScreen = mul(float4(posWorld, 1.0f), light.ViewProjection[0]);
//                    lightScreen.xyz /= lightScreen.w;
        
//                    // ī�޶�(����)���� �� ���� �ؽ��� ��ǥ ���. ([-1, 1], [-1, 1]) ==> ([0, 1], [0, 1])
//                    lightTexcoord.xy = float2(lightScreen.x, -lightScreen.y);
//                    lightTexcoord.xy += 1.0f;
//                    lightTexcoord.xy *= 0.5f;
       
//                    shadowFactor = PCSSForSpotLight(g_ShadowMaps[shadowMapIndex], g_ShadowPointSampler, g_ShadowCompareSampler, lightTexcoord.xy, lightScreen.z - 0.001f, light.InverseProjections[0], light.Radius * radiusScale);
//                }
//                break;
            
//            default:
//                break;
//        }
//    }

//    float3 radiance = light.Radiance * spotFator * att * shadowFactor;
//    return radiance;
//}

//PixelShaderOutput main(PixelShaderInput input)
//{
//    float3 pixelToEye = normalize(g_EyeWorld - input.WorldPosition);
//    float3 normalWorld = GetNormal(input);
    
//    float4 albedo = (bUseAlbedoMap ? g_AlbedoTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias) * float4(g_AlbedoFactor, 1.0f) : float4(g_AlbedoFactor, 1.0f));
//    clip(albedo.a - 0.5f); // Tree leaves. ������ �κ��� �ȼ��� �׸��� ����.
    
//    float ao = (bUseAOMap ? g_AmbientOcclusionTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias).r : 1.0f);
//    float metallic = (bUseMetallicMap ? g_MetallicTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias).b * g_MetallicFactor : g_MetallicFactor);
//    float roughness = (bUseRoughnessMap ? g_RoughnessTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias).g * g_RoughnessFactor : g_RoughnessFactor);
//    float3 emission = (bUseEmissiveMap ? g_EmissiveTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias).rgb : g_EmissionFactor);

//    float3 ambientLighting = AmbientLightingByIBL(g_IrradianceIBLTex, g_SpecularIBLTex, g_BRDFTex, g_LinearClampSampler, g_LinearWrapSampler, albedo.rgb, normalWorld, pixelToEye, ao, metallic, roughness) * g_StrengthIBL;
//    float3 directLighting = float3(0.0f, 0.0f, 0.0f);
    
//    [unroll] // warning X3550: sampler array index must be a literal expression, forcing loop to unroll
//    for (int i = 0; i < MAX_LIGHTS; ++i)
//    {
//        if (lights[i].Type & LIGHT_OFF)
//        {
//            continue;
//        }
        
//        float3 L = lights[i].Position - input.WorldPosition; // ������ �������� ���� �߽������� ����.
//        float3 r = normalize(reflect(g_EyeWorld - input.WorldPosition, normalWorld)); // ������ �ݻ纤��.
//        float3 centerToRay = dot(L, r) * r - L; // ���� �߽ɿ��� �ݻ纤�� r���� ����.
//        float3 representativePoint = L + centerToRay * clamp(lights[i].Radius / length(centerToRay), 0.0f, 1.0f); // ������ Ŀ�� ������ �� ��� �� ����ϴ� ������ ��ǥ ��.
//        representativePoint += input.WorldPosition; // world space�� ��ȯ�� ���� input.posWorld�� ������. ������ ����� ���͵��� input.posWorld�� �������� �ϴ� ��ǥ�迡�� ���ǵ�.
        
//        float3 lightVec = representativePoint - input.WorldPosition;
//        //float3 lightVec = lights[i].Position - input.WorldPosition;
//        float lightDist = length(lightVec);
//        lightVec /= lightDist;
//        float3 halfway = normalize(pixelToEye + lightVec);
        
//        float NdotI = max(0.0f, dot(normalWorld, lightVec));
//        float NdotH = max(0.0f, dot(normalWorld, halfway));
//        float NdotO = max(0.0f, dot(normalWorld, pixelToEye));
        
//        // const float3 F_DIELECTRIC = 0.04f; // ��ݼ�(Dielectric) ������ F0
//        float3 F0 = lerp(F_DIELECTRIC, albedo.rgb, metallic);
//        float3 F = SchlickFresnel(F0, max(0.0f, dot(halfway, pixelToEye)));
//        float3 kd = lerp(float3(1.0f, 1.0f, 1.0f) - F, float3(0.0f, 0.0f, 0.0f), metallic);
//        float3 diffuseBRDF = kd * albedo.rgb;

//       // Sphere Normalization
//        float alpha = roughness * roughness;
//        float alphaPrime = saturate(alpha + lights[i].Radius / (2.0f * lightDist));

//        float D = NdfGGX(NdotH, roughness, alphaPrime);
//        float3 G = SchlickGGX(NdotI, NdotO, roughness);
//        float3 specularBRDF = (F * D * G) / max(1e-5, 4.0f * NdotI * NdotO);

//        float3 radiance = float3(0.0f, 0.0f, 0.0f);
//        radiance = LightRadiance(lights[i], i, representativePoint, input.WorldPosition, normalWorld);
            
//        // ���� �ӽ� ���� (radiance�� (0,0,0)�� ���, directLighting += ... �ε��� 0 ���Ͱ� �Ǿ����.
//        if (abs(dot(radiance, float3(1.0f, 1.0f, 1.0f))) > 1e-5)
//        {
//            directLighting += (diffuseBRDF + specularBRDF) * radiance * NdotI;
//        }
//    }
    
//    PixelShaderOutput output;
//    output.PixelColor = float4(ambientLighting + directLighting + emission, 1.0f);
//    output.PixelColor = clamp(output.PixelColor, 0.0f, 1000.0f);
    
//    return output;
//}
