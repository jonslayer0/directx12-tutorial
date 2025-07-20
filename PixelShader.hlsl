struct PIXEL_SHADER_INPUT
{
    float4 color : COLOR;
};

float4 main(PIXEL_SHADER_INPUT input) : SV_Target
{
    return input.color;
}