#version 410 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vVelocity;

uniform mat4 view;
uniform mat4 projection;

out vec3 particleColor;

void main()
{
    // Calculate position
    vec3 position = vPosition;
    
    // Add slight wobble based on position
    float wobble = sin(position.y * 0.5 + position.x * 0.3) * 0.2;
    position.x += wobble;
    
    gl_Position = projection * view * vec4(position, 1.0);
    
    // Dynamic size based on height and wobble
    float heightFactor = (position.y + 1.0) / 9.0; // Normalize height
    float sizeVariation = sin(position.y * 0.5) * 0.5 + 1.0;
    gl_PointSize = mix(6.0, 10.0, heightFactor) * sizeVariation;
    
    // Color variation based on height and movement
    float brightness = mix(0.8, 1.0, heightFactor);
    float alpha = mix(0.6, 1.0, heightFactor);
    particleColor = vec3(brightness) * alpha;
} 