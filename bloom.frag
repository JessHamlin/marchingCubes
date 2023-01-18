#version 150 compatibility

// Values from vertex shader
in vec3 worldPos;
uniform vec3 camPos;
uniform vec3 coinPos;

vec3 coinPosTmp = coinPos;

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

void main() {
    vec3 glowColor = vec3(0, 0, 0);
    if (length(coinPos - worldPos) < 10) {
        if (fbm(worldPos * 3) > 0.7) {
            glowColor = vec3(0.9, 0.8, 0);
        }
    }
    
    float distance = length(camPos - worldPos) / 10;
    if (distance > 3.5) {
        glowColor *= mix(1, 0, clamp((distance - 3.5) / 0.9, 0, 1));
    }

    gl_FragColor = vec4(glowColor, 1.0);
}
