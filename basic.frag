#version 150 compatibility

// in vec4 Color;
// in vec3 normal;
// in vec4 Vertex;
// out vec4 FragColor;

// Fragment shader from hw7
// This is a wacky and crazy shader, with a lot of math in it
// Currently, it implements albedo, metallic, and roughness maps. 
// This was sourced from this very helpful video and website:
//      https://www.youtube.com/watch?v=5p0e7YNONr8&list=PLIbUZ3URbL0ESKHrvzXuHjrcLi7gxhBby&index=29
//      https://learnopengl.com/

uniform sampler2D imgA;
uniform sampler2D imgM;
uniform sampler2D imgR;
uniform sampler2D imgN;

// Values from vertex shader
in vec3 worldPos;
uniform vec3 camPos;
in vec3 normal;

// Material perimeters
vec3 albedo = texture2D(imgA, gl_TexCoord[0].st).rgb;
float metallic = texture2D(imgM, gl_TexCoord[0].st).r;
float roughness = texture2D(imgR, gl_TexCoord[0].st).r;
vec3 nmap = texture2D(imgN, gl_TexCoord[0].st).rgb;

// Lights
uniform vec4 position;
uniform vec4 diffuse;
vec3 lightPosition = vec3(camPos.x + 0.1, camPos.y + 0.1, camPos.z + 0.1);
vec3 lightColor = vec3(1, 0.8, 0.5);

const float PI = 3.14159265359;

// Noise function
// Source: https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
#define NUM_OCTAVES 5
float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}
// Fractional Brownian motion
float fbm(vec3 x) {
	float v = 0.0;
	float a = 0.5;
	vec3 shift = vec3(100);
	for (int i = 0; i < NUM_OCTAVES; ++i) {
		v += a * noise(x);
		x = x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}

// Helper functions
float distributionGGX(float NdotH, float roughness) {
    // Get roughness ^4
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0000001);
}
float geometrySmith(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float ggx1 = NdotV / (NdotV * (1.0-k) + k);
    float ggx2 = NdotL / (NdotL * (1.0-k) + k);
    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float HdotV, vec3 baseReflectivity) {
    return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - HdotV, 5.0);
}

void main() {
    vec3 nVal = normal;
    // Base grey
    albedo = vec3(0.26 + (smoothstep(-1000, -1050, camPos.y) * 0.2), 0.26, 0.26);
    roughness = 0.9;
    metallic = 0.1;
    // Top color
    if (worldPos.y > -5) {
        albedo = vec3(0.5, 0.5, 0.5);
    }
    else {
        // Highlights
        if (fbm(worldPos / 4) > 0.5) {
            albedo = vec3(0.23 + (smoothstep(-1000, -1050, camPos.y) * 0.2), 0.23, 0.23);
            roughness = 0.85;
        }
        if (fbm(worldPos * 3) > 0.6) {
            albedo = vec3(0.24 + (smoothstep(-1000, -1050, camPos.y) * 0.2), 0.24, 0.24);
            roughness = 0.87;
        }
    }

    roughness = max(roughness, 0.08);
    // Get normal and view vector
    vec3 N = normalize(nVal);
    vec3 V = normalize(camPos - worldPos);

    vec3 baseReflectivity = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < 1; ++i) { // Increase this loop for each light
        // Per light radiance
        // This is a measure of input energy
        vec3 L = normalize(lightPosition - worldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPosition - worldPos) / 10;
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColor * 10 * attenuation;
        if (distance > 4) {
            radiance *= mix(1, 0, clamp((distance - 4) / 0.7, 0, 1));
        }

        // Cook-Torrance BRDF
        // This stands for bidirectional reflectance distribution function
        float NdotV = max(dot(N, V), 0.0000001); // These cannot be zero
        float NdotL = max(dot(N, L), 0.0000001); // These cannot be zero
        float HdotV = max(dot(H, V), 0);
        float NdotH = max(dot(N, H), 0);

        float D = distributionGGX(NdotH, roughness);
        float G = geometrySmith(NdotV, NdotL, roughness);
        vec3 F = fresnelSchlick(HdotV, baseReflectivity);

        vec3 specular = D * G * F;
        specular = specular / (4.0 * NdotV * NdotL);
        // This has been confusing, but in all this should 
        //     give a larger specular the more the light vector diverges from the normal

        // KD is diffuse light
        // Due to energy conservation, diffuse + specular cannot be more than 1
        vec3 KD = vec3(1.0) - F;

        KD *= 1.0 - metallic;

        Lo += (KD * albedo * albedo * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.0) * albedo;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));

    color = pow(color, vec3(1.0/2.2));
    gl_FragColor = vec4(color, 1.0);
}
