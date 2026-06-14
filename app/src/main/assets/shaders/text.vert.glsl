#version 300 es

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform vec2 uViewport;

out vec2 vUV;

void main()
{
    vec2 ndc = (aPos / uViewport) * 2.0 - 1.0;
    ndc.y = -ndc.y;

    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV = aUV;
}
