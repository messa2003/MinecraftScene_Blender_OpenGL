#version 410 core

in vec3 particleColor;
out vec4 fColor;

void main()
{
    // Create circular snowflakes
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float circle = 1.0 - length(circCoord);
    circle = smoothstep(0.0, 0.8, circle);
    
    fColor = vec4(particleColor, circle * 0.6); // Softer snow particles
} 