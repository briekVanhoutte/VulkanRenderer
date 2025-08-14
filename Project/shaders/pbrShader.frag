#version 450

const uint MAX_TEXTURES = 32;

layout(set = 0, binding = 0) uniform UBO {
    mat4 proj;
    mat4 view;
    vec3 cameraPos;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D texSampler[MAX_TEXTURES];

layout(location = 0) in vec3 vWorldNormal;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vUV;
layout(location = 3) flat in uvec4 vTexIds0; // {albedo, normal, metal, rough}
layout(location = 4) flat in uint  vHeightId;

layout(location = 0) out vec4 outColor;

// (optional) UV transforms / parallax
const float ROTATE_DEG = 0.0;
const int   FLIP_X     = 0;
const int   FLIP_Y     = 1;

vec2 transformUV(vec2 uv) {
    uv.x = (FLIP_X == 1) ? 1.0 - uv.x : uv.x;
    uv.y = (FLIP_Y == 1) ? 1.0 - uv.y : uv.y;
    float a = radians(ROTATE_DEG);
    float s = sin(a), c = cos(a);
    uv -= 0.5;
    uv = mat2(c, -s, s, c) * uv;
    uv += 0.5;
    return uv;
}

vec2 parallaxUV(vec2 uv, vec3 viewDir, sampler2D heightMap, float scale) {
    float height = texture(heightMap, uv).r;
    return uv + viewDir.xy * (height * scale);
}

void main() {
    vec2 uv = transformUV(vUV);

    // crude view dir; for accuracy pass worldPos from VS and use (cameraPos - worldPos)
    vec3 V = normalize(ubo.cameraPos);
    const float parallaxScale = 0.04;
    if (vHeightId < MAX_TEXTURES) {
        uv = parallaxUV(uv, V, texSampler[vHeightId], parallaxScale);
    }

    vec3 N = normalize(vWorldNormal);

    vec3 albedo = (vTexIds0.x < MAX_TEXTURES)
        ? texture(texSampler[vTexIds0.x], uv).rgb
        : vColor;

    if (vTexIds0.y < MAX_TEXTURES) {
        vec3 nrm = texture(texSampler[vTexIds0.y], uv).rgb * 2.0 - 1.0;
        // Proper TBN would be better; this is a quick approximation:
        N = normalize(nrm);
    }

    float metallic  = (vTexIds0.z < MAX_TEXTURES) ? texture(texSampler[vTexIds0.z], uv).r : 0.0;
    float roughness = (vTexIds0.w < MAX_TEXTURES) ? texture(texSampler[vTexIds0.w], uv).r : 1.0;

    // simple lighting
    vec3 L = normalize(vec3(-0.3, -0.6, 0.6));
    vec3 H = normalize(L + V);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = (1.0 - metallic) * albedo / 3.141592 * NdotL;

    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    vec3  F = F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
    float spec = pow(NdotH, 1.0 / max(roughness * roughness, 0.01));
    vec3 specular = F * spec * NdotL;

    const float lightIntensity = 3.0;
    const float AMBIENT = 0.2;
    vec3 color = AMBIENT * albedo + lightIntensity * (diffuse + specular);

    outColor = vec4(color, 1.0);
}
