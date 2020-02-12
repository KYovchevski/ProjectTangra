
struct Matrices
{
    matrix MVP;
};

ConstantBuffer<Matrices> MatCB : register(b0, space0);

struct VS_OUT
{
    float2 TexCoords : TEXCOORD;
    float4 Position : SV_POSITION;
};

struct Light
{
    float3 position;
    float attenuation;
    //---------- 16-bytes
    float3 color;
    float padding;
    //---------- 16-bytes
};

struct Vertex
{
    float3 pos;
    float2 tex;
};

StructuredBuffer<Vertex> VerticesSB : register(t0, space0);
StructuredBuffer<Light> LightsSB : register(t1, space0);

VS_OUT main(uint vertexID : SV_VertexID) 
{
    VS_OUT vout;
    vout.Position = mul(MatCB.MVP, float4(VerticesSB[vertexID].pos, 1));
    vout.TexCoords = VerticesSB[vertexID].tex;
    return vout;
}