#version 450

const uint MAX_TEXTURES = 32;

layout(set = 0, binding = 0) uniform UBO {
    mat4 proj;
    mat4 view;
    vec3 cameraPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint AlbedoID;
    uint NormalMapID;
    uint MetalnessID;
    uint RoughnessID;
    uint HeightMapID;
} mesh;

layout(set = 0, binding = 1) uniform sampler2D texSampler[MAX_TEXTURES];

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

const float ROTATE_DEG = 0.0;    // e.g. 0, 90, 180, 270
const int   FLIP_X     = 0;      // 1 = mirror horizontally
const int   FLIP_Y     = 0;      // 1 = mirror vertically

vec2 parallaxUV(vec2 uv, vec3 viewDir, sampler2D heightMap, float scale) {
    float height = texture(heightMap, uv).r;
    return uv + viewDir.xy * (height * scale);
}

vec2 transformUV(vec2 uv) {
    uv.x = (FLIP_X == 1) ? 1.0 - uv.x : uv.x;
    uv.y = (FLIP_Y == 1) ? 1.0 - uv.y : uv.y;
    float a = radians(ROTATE_DEG);
    float s = sin(a), c = cos(a);
    uv -= 0.5;
    uv = 1.0 - uv;
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = transformUV(fragTexCoord);

    float parallaxScale = 0.04;
    if (mesh.HeightMapID < MAX_TEXTURES) {
        vec3 viewDir = normalize(ubo.cameraPos);
        uv = parallaxUV(uv, viewDir, texSampler[mesh.HeightMapID], parallaxScale);
    }

    vec3 albedo = (mesh.AlbedoID < MAX_TEXTURES)
        ? texture(texSampler[mesh.AlbedoID], uv).rgb
        : fragColor;

    vec3 N = (mesh.NormalMapID < MAX_TEXTURES)
        ? normalize(texture(texSampler[mesh.NormalMapID], uv).rgb * 2.0 - 1.0)
        : normalize(fragNormal);

    float metallic = (mesh.MetalnessID < MAX_TEXTURES)
        ? texture(texSampler[mesh.MetalnessID], uv).r
        : 0.0;

    float roughness = (mesh.RoughnessID < MAX_TEXTURES)
        ? texture(texSampler[mesh.RoughnessID], uv).r
        : 1.0;

    vec3 L = normalize(vec3(-0.3, -0.6, 0.6));
    vec3 V = normalize(ubo.cameraPos);
    vec3 H = normalize(L + V);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = (1.0 - metallic) * albedo / 3.141592 * NdotL;

    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
    float spec = pow(NdotH, 1.0 / max(roughness * roughness, 0.01));
    vec3 specular = F * spec * NdotL;

    const float lightIntensity = 3.0;
    const float AMBIENT = 0.2;

    vec3 color = AMBIENT * albedo + lightIntensity * (diffuse + specular);
    outColor = vec4(color, 1.0);
}
