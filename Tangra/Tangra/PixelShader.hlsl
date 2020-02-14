sampler samp : register(s0, space1);

Texture2D diffuseTex : register(t0, space1);

float4 main(float2 texCoord : TEXCOORD) : SV_TARGET
{
    return diffuseTex.Sample(samp, texCoord);
}