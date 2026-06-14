#version 300 es
precision mediump float;

in vec2 vUV;

uniform sampler2D uFontAtlas;
uniform vec4 uColor;

out vec4 fragColor;

void main()
{
    float alpha = texture(uFontAtlas, vUV).r;

    fragColor = vec4(uColor.rgb, uColor.a * alpha);
}
