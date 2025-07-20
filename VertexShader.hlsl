struct VERTEX_POS_COLOR
{
    float3 position : POSITION;
    float3 color : COLOR;
};

cbuffer modelViewProjectionCB : register(b0)
{
    matrix modelToProj;
};

struct VERTEX_SHADER_OUTPUT
{
    float4 color : COLOR;
    float4 position : SV_Position;
};

VERTEX_SHADER_OUTPUT main(VERTEX_POS_COLOR input)
{
    VERTEX_SHADER_OUTPUT output;
    
    output.position = mul(modelToProj, float4(input.position, 1.0f));
    output.color = float4(input.color, 1.0f);
    
    return output;
}