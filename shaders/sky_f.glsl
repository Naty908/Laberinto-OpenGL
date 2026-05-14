#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

uniform float uFade; 

void main() {
    float y = normalize(TexCoords).y;

    vec3 zenithColor = vec3(0.05, 0.05, 0.2);
    vec3 horizonColor = vec3(0.95, 0.5, 0.3);
    vec3 groundColor = vec3(0.15, 0.12, 0.08);

    vec3 skyColor;
    if(y > 0.0) {
        skyColor = mix(horizonColor, zenithColor, pow(y, 0.5));
    } else {
        skyColor = mix(horizonColor, groundColor, pow(-y, 0.8));
    }

    FragColor = vec4(skyColor * uFade, 1.0);
}