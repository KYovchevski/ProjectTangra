
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

struct Vertex
{
    float3 pos;
    float2 tex;
};

StructuredBuffer<Vertex> VerticesSB : register(t0, space0);

VS_OUT main(float3 pos : POSITION, float2 tex : TEXCOORD) 
{
    VS_OUT vout;
    vout.Position = mul(MatCB.MVP, float4(pos, 1));
    vout.TexCoords = tex;
    return vout;
}