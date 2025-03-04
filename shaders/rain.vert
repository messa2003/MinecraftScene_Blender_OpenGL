#version 410 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vVelocity;

uniform mat4 view;
uniform mat4 projection;
uniform float deltaTime;
uniform vec3 windDirection;
uniform float windStrength;

out vec3 particleColor;

const vec3 snowCenter = vec3(-0.1, 0.5, 0.2);
const float groundLevel = -1.0;
const float resetHeight = 5.0;

// Add time-based movement
float rand(float n) { return fract(sin(n) * 43758.5453123); }

void main()
{
    vec3 position = vPosition;
    vec3 velocity = vVelocity;
    
    // Add swirling motion for snow
    float time = deltaTime * gl_VertexID * 0.0001;
    float swayX = sin(time + position.y * 0.1) * 0.5;
    float swayZ = cos(time + position.x * 0.1) * 0.5;
    
    // Apply wind and sway
    velocity += windDirection * windStrength * deltaTime;
    velocity.x += swayX * deltaTime;
    velocity.z += swayZ * deltaTime;
    
    // Update position with velocity
    position += velocity * deltaTime;
    
    // Reset particle if it falls below ground level
    if(position.y < groundLevel) {
        float randX = snowCenter.x + (rand(gl_VertexID) * 2.0 - 1.0) * 7.5;
        float randZ = snowCenter.z + (rand(gl_VertexID + 1.0) * 2.0 - 1.0) * 7.5;
        position = vec3(randX, snowCenter.y + resetHeight, randZ);
        velocity = vVelocity;
    }
    
    gl_Position = projection * view * vec4(position, 1.0);
    
    // White color for snow with distance fade
    float dist = length(position - snowCenter);
    float alpha = clamp(1.0 - dist * 0.005, 0.4, 1.0);
    particleColor = vec3(1.0, 1.0, 1.0) * alpha; // Pure white for snow
    
    // Larger points for snowflakes
    gl_PointSize = 4.0;
} 