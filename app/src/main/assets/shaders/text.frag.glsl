#version 300 es
precision mediump float;

in vec2 vUV;
in vec2 vPos;

uniform sampler2D uFontAtlas;
uniform vec4 uColor;

uniform vec4 uClipRect;
uniform vec4 uClipCorners;
uniform int uClipEnabled;

out vec4 fragColor;

float roundedBoxSDF(vec2 p, vec2 rectPos, vec2 rectSize, vec4 corners)
{
    vec2 halfSize = rectSize * 0.5;
    vec2 center = rectPos + halfSize;
    vec2 d = p - center;

    float r = (d.x > 0.0) ? ((d.y < 0.0) ? corners.y : corners.z)
    : ((d.y < 0.0) ? corners.x : corners.w);

    vec2 q = abs(d) - halfSize + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main()
{
    if (uClipEnabled == 1) {
        float clipDist = roundedBoxSDF(vPos, uClipRect.xy, uClipRect.zw, uClipCorners);
        if (clipDist > 0.0) discard;
    }

    float alpha = texture(uFontAtlas, vUV).r;
    fragColor = vec4(uColor.rgb, uColor.a * alpha);
}