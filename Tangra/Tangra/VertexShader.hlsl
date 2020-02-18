
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
    
    //matrix totalTransform = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float4 totalPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    
    for (uint i = 0; i < 4; ++i)
    {
        float weight = input.weights[i];
      //  totalTransform += boneMatricesSB[input.jointID[i]] * weight;
        
        matrix boneTransform = boneMatricesSB[input.jointID[i]];
        
        float4 posePos = mul(boneTransform, float4(input.pos, 1.0f));
        totalPos += posePos * weight;
    }
    
    //totalPos = mul(totalTransform, float4(input.pos, 1.0f));
    
    vout.Position = mul(MatCB.MVP, totalPos);
    vout.TexCoords = input.tex;
    return vout;
}