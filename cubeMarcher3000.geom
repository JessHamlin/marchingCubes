//
//  Marching Cubes Geometry Shader
//  Converts a point to zero, one, or more quads
//  One input point corresponds to one cube to be marched through
//

#version 150 compatibility
#extension GL_EXT_geometry_shader4 : enable
layout(points) in;
layout(triangle_strip,max_vertices=12) out;

// Inputs
uniform float CPD;
uniform float CPFD;
uniform float zh;
uniform vec3 camPos;
//uniform int triTableFlat[16 * 256];
uniform isampler2D triTableTex;

// Other
const float isolevel = 100;

// Outputs
out vec4 Color;
out vec3 normal;
out vec3 worldPos;

//	Simplex 3D Noise 
//	by Ian McEwan, Ashima Arts
//  https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}

float snoise(vec3 v){ 
    const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
    const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

    // First corner
    vec3 i  = floor(v + dot(v, C.yyy) );
    vec3 x0 =   v - i + dot(i, C.xxx) ;

    // Other corners
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min( g.xyz, l.zxy );
    vec3 i2 = max( g.xyz, l.zxy );

    //  x0 = x0 - 0. + 0.0 * C 
    vec3 x1 = x0 - i1 + 1.0 * C.xxx;
    vec3 x2 = x0 - i2 + 2.0 * C.xxx;
    vec3 x3 = x0 - 1. + 3.0 * C.xxx;

    // Permutations
    i = mod(i, 289.0 ); 
    vec4 p = permute( permute( permute( 
              i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
            + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
            + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

    // Gradients
    // ( N*N points uniformly over a square, mapped onto an octahedron.)
    float n_ = 1.0/7.0; // N=7
    vec3  ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z *ns.z);  //  mod(p,N*N)

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

    vec4 x = x_ *ns.x + ns.yyyy;
    vec4 y = y_ *ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4( x.xy, y.xy );
    vec4 b1 = vec4( x.zw, y.zw );

    vec4 s0 = floor(b0)*2.0 + 1.0;
    vec4 s1 = floor(b1)*2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
    vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

    vec3 p0 = vec3(a0.xy,h.x);
    vec3 p1 = vec3(a0.zw,h.y);
    vec3 p2 = vec3(a1.xy,h.z);
    vec3 p3 = vec3(a1.zw,h.w);

    //Normalise gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                    dot(p2,x2), dot(p3,x3) ) );
}

void makeTri(vec3 points[8], float weights[8], int e1, int e2, int e3)
{
    // Transform edge into 2 indices in vert list
    int i1, i2;
    ivec2 edgeToIndex[12] = ivec2[] (
        ivec2(0, 1), //0
        ivec2(1, 2), //1
        ivec2(2, 3), //2
        ivec2(0, 3), //3
        ivec2(4, 5), //4
        ivec2(5, 6), //5
        ivec2(6, 7), //6
        ivec2(4, 7), //7
        ivec2(0, 4), //8
        ivec2(1, 5), //9
        ivec2(2, 6), //10
        ivec2(3, 7) //11
    );
    // Get points of triangle by averaging the two points defining an edge
    i1 = edgeToIndex[e1].x;
    i2 = edgeToIndex[e1].y;
    //vec3 p1 = vec3((points[i1].x + points[i2].x) / 2.f, (points[i1].y + points[i2].y) / 2.f, (points[i1].z + points[i2].z) / 2.f);
    vec3 p1 = points[i1] + (isolevel - weights[i1]) * (points[i2] - points[i1]) / (weights[i2] - weights[i1]);
    i1 = edgeToIndex[e2].x;
    i2 = edgeToIndex[e2].y;
    //vec3 p2 = vec3((points[i1].x + points[i2].x) / 2.f, (points[i1].y + points[i2].y) / 2.f, (points[i1].z + points[i2].z) / 2.f);
    vec3 p2 = points[i1] + (isolevel - weights[i1]) * (points[i2] - points[i1]) / (weights[i2] - weights[i1]);
    i1 = edgeToIndex[e3].x;
    i2 = edgeToIndex[e3].y;
    //vec3 p3 = vec3((points[i1].x + points[i2].x) / 2.f, (points[i1].y + points[i2].y) / 2.f, (points[i1].z + points[i2].z) / 2.f);
    vec3 p3 = points[i1] + (isolevel - weights[i1]) * (points[i2] - points[i1]) / (weights[i2] - weights[i1]);
    // Get the normal
    // Source: https://stackoverflow.com/questions/19350792/calculate-normal-of-a-single-triangle-in-3d-space
    vec3 A = p2 - p1;
    vec3 B = p3 - p1;
    normal = vec3(
        A.y * B.z - A.z * B.y,
        A.z * B.x - A.x * B.z,
        A.x * B.y - A.y * B.x
    );
    // Draw the triangle
    gl_Position = gl_ModelViewProjectionMatrix * vec4(p1, 1);
    Color = vec4(1, 0, 0, 1);
    worldPos = p1;
    EmitVertex();
    gl_Position = gl_ModelViewProjectionMatrix * vec4(p2, 1);
    Color = vec4(0, 1, 0, 1);
    worldPos = p2;
    EmitVertex();
    gl_Position = gl_ModelViewProjectionMatrix * vec4(p3, 1);
    Color = vec4(0, 0, 1, 1);
    worldPos = p3;
    EmitVertex();
    // End the triangle
    EndPrimitive();
}

void main()
{
    // Make points
    vec4 pCube = gl_PositionIn[0];
    float offset = CPD / 2.f;
    vec3 points[8] = vec3[] (
        vec3(pCube.x - offset, pCube.y - offset, pCube.z + offset),
        vec3(pCube.x - offset, pCube.y + offset, pCube.z + offset),
        vec3(pCube.x + offset, pCube.y + offset, pCube.z + offset),
        vec3(pCube.x + offset, pCube.y - offset, pCube.z + offset),
        vec3(pCube.x - offset, pCube.y - offset, pCube.z - offset),
        vec3(pCube.x - offset, pCube.y + offset, pCube.z - offset),
        vec3(pCube.x + offset, pCube.y + offset, pCube.z - offset),
        vec3(pCube.x + offset, pCube.y - offset, pCube.z - offset));
    // Set weights for each point
    float weights[8] = float[] (0, 0, 0, 0, 0, 0, 0, 0);
    for (int i = 0; i < 8; i++) {
        if (points[i].y < -2 && ( (gl_ModelViewProjectionMatrix * vec4(points[i], 1)).z > 0)) {
            // Make a sphere around the player
            if (pow(points[i].x - camPos.x, 2) + pow(points[i].y - camPos.y, 2) + pow(points[i].z - camPos.z, 2) < 15) {
                weights[i] = 5 * (pow(points[i].x - camPos.x, 2) + pow(points[i].y - camPos.y, 2) + pow(points[i].z - camPos.z, 2));
            }
            else {
                // Use simplex noise
                float scale = 64;
                weights[i] = ((snoise(vec3((points[i].x + zh) / scale, points[i].y / scale, points[i].z / scale)) + 1) / 2) * 255;
            }
        }
        else {
            weights[i] = 0;
        }
    }
    // Filter out empty cubes
    int cubeindex = 0;
    if (weights[0] < isolevel) cubeindex |= 1;
    if (weights[1] < isolevel) cubeindex |= 2;
    if (weights[2] < isolevel) cubeindex |= 4;
    if (weights[3] < isolevel) cubeindex |= 8;
    if (weights[4] < isolevel) cubeindex |= 16;
    if (weights[5] < isolevel) cubeindex |= 32;
    if (weights[6] < isolevel) cubeindex |= 64;
    if (weights[7] < isolevel) cubeindex |= 128;
    if (cubeindex == 0 || cubeindex == 255) {
        return;
    }
    // Make mesh for non-empty cubes
    for (int i = 0; texelFetch2D(triTableTex, ivec2(i, cubeindex), 0).a != -1; i += 3) {
        makeTri(points, weights,
            texelFetch2D(triTableTex, ivec2(i + 0, cubeindex), 0).a,
            texelFetch2D(triTableTex, ivec2(i + 1, cubeindex), 0).a,
            texelFetch2D(triTableTex, ivec2(i + 2, cubeindex), 0).a);
    }
    // For Debugging:
    // Color = vec4(0.5, 0.5, 0.5, 1);
    // gl_Position = gl_ModelViewProjectionMatrix * vec4(0, 1, 1, 1);
    // EmitVertex();
    // gl_Position = gl_ModelViewProjectionMatrix * vec4(1, 1, 0, 1);
    // EmitVertex();
    // //gl_Position = gl_ModelViewProjectionMatrix * vec4(0, texelFetch2D(triTableTex, ivec2(2, 2), 0).a, 0, 1);
    // gl_Position = gl_ModelViewProjectionMatrix * gl_PositionIn[0];
    // EmitVertex();
    // EndPrimitive();
}
