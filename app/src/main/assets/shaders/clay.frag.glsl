#version 300 es
precision mediump float;
uniform vec4 uColor;
uniform vec4 uRect;
uniform vec4 uCorners;
uniform sampler2D uTexture;
uniform int uUseTexture;

uniform vec4 uClipRect;
uniform vec4 uClipCorners;
uniform int uClipEnabled;

in vec2 vPos;
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
    float dist = roundedBoxSDF(vPos, uRect.xy, uRect.zw, uCorners);
    if (dist > 0.0) discard;

    if (uClipEnabled == 1) {
        float clipDist = roundedBoxSDF(vPos, uClipRect.xy, uClipRect.zw, uClipCorners);
        if (clipDist > 0.0) discard;
    }

    if (uUseTexture == 1) {
        vec2 uv = (vPos - uRect.xy) / uRect.zw;
        vec4 texColor = texture(uTexture, uv);
        vec4 tint = uColor;
        if (tint.a > 0.0)
        texColor = mix(texColor, tint, tint.a);
        fragColor = texColor;
    } else {
        fragColor = uColor;
    }
}