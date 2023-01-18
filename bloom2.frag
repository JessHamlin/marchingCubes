// Bloom
#version 120

uniform float dX;
uniform float dY;
uniform sampler2D img;
uniform sampler2D em;

vec4 sample(float dx,float dy)
{
   return texture2D(em,gl_TexCoord[0].st+vec2(dx,dy));
}

void main()
{
    vec4 color = sample(0.0,0.0);
    float one = 2.0/13.0;
    float two = 4.0/13.0;
    color = ( 
          one*sample(-dX,+dY) + two*sample(0.0,+dY) + one*sample(+dX,+dY)
        + two*sample(-dX,0.0) + one*sample(0.0,0.0) + two*sample(+dX,0.0)
        + one*sample(-dX,-dY) + two*sample(0.0,-dY) + one*sample(+dX,-dY)
    );

    color = color + texture2D(img, gl_TexCoord[0].st);
    // vec4 color = texture2D(em, gl_TexCoord[0].st);
    gl_FragColor = color;
}