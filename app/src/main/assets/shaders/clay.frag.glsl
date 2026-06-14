#version 300 es
precision mediump float;
uniform vec4 uColor;
uniform vec4 uRect;
uniform vec4 uCorners;
uniform sampler2D uTexture;
uniform int uUseTexture;
in vec2 vPos;
out vec4 fragColor;

void main()
{
    vec2 halfSize = uRect.zw * 0.5;
    vec2 center = uRect.xy + halfSize;
    vec2 p = vPos - center;

    // uCorners = (topLeft, topRight, bottomRight, bottomLeft)
    float r = (p.x > 0.0) ? ((p.y < 0.0) ? uCorners.y : uCorners.z)
                          : ((p.y < 0.0) ? uCorners.x : uCorners.w);

    vec2 q = abs(p) - halfSize + r;
    float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;

    if (dist > 0.0) discard;

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
