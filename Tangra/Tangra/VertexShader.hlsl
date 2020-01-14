
struct Matrices
{
    matrix MVP;
};

ConstantBuffer<Matrices> MatCB : register(b0, space0);

float4 main( float3 pos : POSITION ) : SV_POSITION
{
    return mul(MatCB.MVP, float4(pos, 1));
}