#version 410 core

in vec3 normal;
in vec4 fragPosEye;
in vec4 fragPosLightSpace;
in vec2 fragTexCoords;

out vec4 fColor;

//light and texture variables
uniform	mat3 normalMatrix;
uniform mat3 lightDirMatrix;
uniform	vec3 lightColor;
uniform	vec3 lightDir;
uniform	vec3 lightPos1;
uniform	vec3 lightPos2;
uniform	vec3 lightPos3;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;
uniform mat4 view;

//alpha for transparency
uniform float alpha;


//directional light attributes
vec3 ambient;
float ambientStrength = 1.0f;
vec3 diffuse;
vec3 specular;
float specularStrength = 1.0f;
float shininess = 64.0f;


//point light attributes
float ambientPoint = 3.0f;
float specularStrengthPoint = 0.9f;
float shininessPoint = 32.0f;

float constant = 1.0f;
float linear = 0.7f;
float quadratic = 1.8f;


//shadow computation
float computeShadow()
{	
	// perform perspective divide
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    if(normalizedCoords.z > 1.0f)
        return 0.0f;
    // Transform to [0,1] range
    normalizedCoords = normalizedCoords * 0.5f + 0.5f;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;    
    // Get depth of current fragment from light's perspective
    float currentDepth = normalizedCoords.z;
    // Check whether current frag pos is in shadow
    float bias = 0.005f;
    float shadow = currentDepth - bias> closestDepth  ? 1.0f : 0.0f;

    return shadow;	
}

//compute directional light
vec3 computeLightComponents(vec3 lightDirection, int shadow, vec3 lightColor)
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(normalMatrix * normal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDirMatrix * lightDirection);	

	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);
	
	//compute half vector
	vec3 halfVector = normalize(lightDirN + viewDirN);
		
	//compute ambient light
	ambient = ambientStrength * lightColor;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	float specCoeff = pow(max(dot(halfVector, normalEye), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;

	ambient *= vec3(texture(diffuseTexture, fragTexCoords));
	diffuse *= vec3(texture(diffuseTexture, fragTexCoords));
	specular *= vec3(texture(specularTexture, fragTexCoords));
	
	if(shadow == 1)
	{
		float shadow = computeShadow();
		//modulate with shadow
		return min((ambient + (0.8f - 2*shadow)*diffuse) + (0.8f - 2*shadow)*specular, 1.0f);
	}
	else
	{
		return (ambient + diffuse + specular);
	}
    
}

//compute fog
float computeFog()
{
 	float fogDensity = 0.02f;
 	float fragmentDistance = length(fragPosEye);
	float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

	return clamp(fogFactor, 0.0f, 1.0f);
}


//compute point light
vec3 computePointLight(vec4 lightPosEye, vec3 lightColor, float constant, float linear, float quadratic)
{
    vec3 cameraPosEye = vec3(0.0f);
	vec3 normalEye = normalize(normalMatrix * normal);
	vec3 lightDirN = normalize(lightPosEye.xyz - fragPosEye.xyz);
	vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);
	
	vec3 ambient = ambientPoint * lightColor;
	vec3 diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	vec3 halfVector = normalize(lightDirN + viewDirN);
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininessPoint);
	vec3 specular = specularStrengthPoint * specCoeff * lightColor;
	
	float distance = length(lightPosEye.xyz - fragPosEye.xyz);
	
	float att = 1.0f / (constant + linear * distance + quadratic * distance * distance);
	
	ambient *= vec3(texture(diffuseTexture, fragTexCoords));
	diffuse *= vec3(texture(diffuseTexture, fragTexCoords));
	specular *= vec3(texture(specularTexture, fragTexCoords));
	
	return (ambient + diffuse + specular) * att;
} 

void main() 
{
	//compute directional light which generates shadow
	vec3 result = computeLightComponents(lightDir, 1, lightColor);
	
	vec4 lightPos1 = view * vec4(lightPos1, 1.0f);
	vec4 lightPos2 = view * vec4(lightPos2, 1.0f);
	vec4 lightPos3 = view * vec4(lightPos3, 1.0f);
	
	//compute point lights
	result += computePointLight(lightPos1, vec3(1.0f, 1.0f, 0.0f), 1.0f, 0.22f, 0.20f);
	result += computePointLight(lightPos2, vec3(1.0f, 0.0f, 1.0f), 1.0f, 0.22f, 0.20f);
	result += computePointLight(lightPos3, vec3(0.0f, 1.0f, 1.0f), 1.0f, 0.35f, 0.44f);
	
	//compute fog
	float fogFactor = computeFog();
	vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
	fColor = fogColor * (1 - fogFactor) + vec4(result, 1.0f) * fogFactor;
	
	//set alpha for transparency
	fColor.a = alpha;
}