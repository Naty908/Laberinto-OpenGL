#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords; 

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 uColor; 
uniform float uFade;
uniform vec3 viewPos; 
uniform vec3 fogColor; 
uniform sampler2D texture1; 
uniform bool usesTexture;
uniform vec3 farolasPos[5]; 
uniform int numFarolas;

void main() {

    vec3 baseColor = uColor;
    float alpha = 1.0;
    if(usesTexture) {
        vec4 texColor = texture(texture1, TexCoords);
        if(texColor.a < 0.1) discard;
        baseColor = texColor.rgb * uColor;
        alpha = texColor.a;
    }

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    
    if(!usesTexture && uColor.r > 1.5) { 
        diff = 1.0; 
    } 

    float intensity = (diff > 0.9) ? 1.0 : (diff > 0.5) ? 0.7 : (diff > 0.2) ? 0.4 : 0.15;
    

    vec3 result = intensity * baseColor * lightColor;


    vec3 pointLightsOutput = vec3(0.0);
    for(int i = 0; i < numFarolas; i++) {
        float dist = length(farolasPos[i] + vec3(0, 3, 0) - FragPos);
        

        float attenuation = 1.0 / (1.0 + 0.1 * dist + 0.05 * (dist * dist));
        
        vec3 lampColor = vec3(1.0, 0.7, 0.3) * 2.0; 
        pointLightsOutput += (lampColor * attenuation) * baseColor;
    }

    vec3 finalResult = result + pointLightsOutput;

    float distFog = length(viewPos - FragPos);
    float fogFactor = clamp(exp(-pow(distFog * 0.025, 2)), 0.0, 1.0);

    vec3 finalColor = mix(fogColor, finalResult * uFade, fogFactor);
    FragColor = vec4(finalColor, alpha);
}