
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

struct VS_IN
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD;
    uint4 jointID : JOINT_IDS;
    float4 weights : WEIGHTS;
};

StructuredBuffer<matrix> boneMatricesSB : register(t0, space0);

VS_OUT main(VS_IN input) 
{
    VS_OUT vout;
    
    
    float4 totalPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < 4; ++i)
    {
        float weight = input.weights[i];
        matrix boneTransform = boneMatricesSB[input.jointID[i] - 2];
        
        float4 posePos = mul(boneTransform, float4(input.pos, 1.0f));
        totalPos += posePos * weight;
    }
    
    vout.Position = mul(MatCB.MVP, totalPos);
    vout.TexCoords = input.tex;
    return vout;
}