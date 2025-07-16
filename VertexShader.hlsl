#pragma once

struct VERTEX_POS_COLOR
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct MODEL_VIEW_PROJECTION
{
    matrix modelToProj;
};

ConstantBuffer<MODEL_VIEW_PROJECTION> modelViewProjectionCB : register(b0);

struct VERTEX_SHADER_OUTPUT
{
    float4 color : COLOR;
    float4 position : SV_Position;
};

VERTEX_SHADER_OUTPUT main(VERTEX_POS_COLOR input)
{
    VERTEX_SHADER_OUTPUT output;
    
    output.position = mul(modelViewProjectionCB.modelToProj, float4(input.position, 1.0f));
    output.color = input.color;
    
    return output;
}