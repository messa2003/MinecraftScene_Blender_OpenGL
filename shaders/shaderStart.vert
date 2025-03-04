#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 fNormal;
out vec4 fPosEye;
out vec2 fTexCoords;
out vec4 fragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform	mat3 normalMatrix;
uniform mat4 lightSpaceTrMatrix;
uniform int renderMode;

void main() 
{
	//compute eye space coordinates
	fPosEye = view * model * vec4(vPosition, 1.0f);
	if(renderMode == 2) { // Polygonal mode
		fNormal = vNormal; // Use raw normals without normalization
	} else {
		fNormal = normalize(normalMatrix * vNormal);
	}
	fTexCoords = vTexCoords;
	
	// Compute position in light space for shadows
	fragPosLightSpace = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f);
	
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);
}
