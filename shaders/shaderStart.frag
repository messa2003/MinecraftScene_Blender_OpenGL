#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

//lighting
uniform	vec3 lightDir;
uniform	vec3 lightColor;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

// Add fog uniforms
uniform vec3 fogColor;
uniform float fogDensity;
uniform int fogType; // 0 = linear, 1 = exponential, 2 = exponential squared

// Add render mode uniform
uniform int renderMode; // 0=solid, 1=wireframe, 2=polygonal, 3=smooth

vec3 ambient;
float ambientStrength = 0.1f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.3f;
float shininess = 16.0f;

float computeShadow()
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // Get current depth value
    float currentDepth = projCoords.z;
    
    // Calculate bias based on depth map resolution and slope
    vec3 normal = normalize(fNormal);
    vec3 lightDirN = normalize(lightDir);
    float bias = max(0.015 * (1.0 - dot(normal, lightDirN)), 0.0005);
    
    // Check whether current frag pos is in shadow
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    // Further reduced PCF sample area for even sharper shadows
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize * 0.25).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum
    if(projCoords.z > 1.0)
        shadow = 0.0;
    
    return shadow;
}

void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);
	
	//transform normal
	vec3 normalEye = normalize(fNormal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDir);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
		
	// Adjust ambient strength based on light color intensity
	float avgLightIntensity = (lightColor.r + lightColor.g + lightColor.b) / 3.0;
	float nightAmbientStrength = 0.3;  // Stronger ambient at night to simulate moonlight scatter
	float dayAmbientStrength = 0.1;    // Normal ambient for daytime
	
	// Interpolate ambient strength based on light intensity
	ambientStrength = mix(nightAmbientStrength, dayAmbientStrength, avgLightIntensity);
	
	//compute ambient light
	ambient = ambientStrength * lightColor;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
}

// Add fog calculation function
float computeFog()
{
    float fragmentDistance = length(fPosEye);
    float fogFactor = 0.0f;
    
    if(fogType == 0) // linear fog
    {
        float fogStart = 2.0f;
        float fogEnd = 10.0f;
        fogFactor = (fogEnd - fragmentDistance) / (fogEnd - fogStart);
    }
    else if(fogType == 1) // exponential fog
    {
        fogFactor = 1.0f / exp(fragmentDistance * fogDensity);
    }
    else if(fogType == 2) // exponential squared fog
    {
        fogFactor = 1.0f / exp((fragmentDistance * fogDensity) * (fragmentDistance * fogDensity));
    }
    
    fogFactor = clamp(fogFactor, 0.0f, 1.0f);
    
    return fogFactor;
}

void main() 
{
	computeLightComponents();
	
	vec3 baseColor = vec3(0.9f, 0.35f, 0.0f);
	vec3 finalColor;
	
	// Handle different render modes
	switch(renderMode) {
		case 0: // Solid
			ambient *= texture(diffuseTexture, fTexCoords).rgb;
			diffuse *= texture(diffuseTexture, fTexCoords).rgb;
			specular *= texture(specularTexture, fTexCoords).rgb;
			break;
			
		case 1: // Wireframe
			ambient = vec3(0.0);
			diffuse = vec3(1.0); // White wireframe
			specular = vec3(0.0);
			break;
			
		case 2: // Polygonal (flat shading)
			ambient *= texture(diffuseTexture, fTexCoords).rgb;
			diffuse *= texture(diffuseTexture, fTexCoords).rgb;
			specular *= 0.0; // No specular for polygonal look
			break;
			
		case 3: // Smooth
			ambient *= texture(diffuseTexture, fTexCoords).rgb;
			diffuse *= texture(diffuseTexture, fTexCoords).rgb * 0.8; // Softer diffuse
			specular *= texture(specularTexture, fTexCoords).rgb * 1.2; // Enhanced specular
			break;
	}
	
	// Calculate shadow
	float shadow = computeShadow();
	
	// Calculate lighting with shadows
	vec3 color = min(ambient + (1.0 - shadow) * (diffuse + specular), 1.0f);
	
	// Apply fog
	float fogFactor = computeFog();
	vec3 foggedColor = mix(fogColor, color, fogFactor);
	
	fColor = vec4(foggedColor, 1.0f);
}
