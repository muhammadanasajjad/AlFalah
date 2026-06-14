#version 300 es
layout(location = 0) in vec2 aPos;
uniform vec2 uResolution;
out vec2 vPos;

void main()
{
    vec2 ndc = (aPos / uResolution) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
    vPos = aPos;
}
