#version 460 core

out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 skyColorTop = vec3(0.0, 0.0, 1.0);
uniform vec3 skyColorBottom = vec3(0);
uniform vec3 moonColor = vec3(1);
uniform float moonRadius = 0.2; // Radius of the moon
uniform vec3 sunPosition = vec3(1000, 100, 0);
uniform vec3 viewPos;

void main()
{
    vec3 moonPosition = -sunPosition;
    // Simulate 3D depth for the sky
    float depth = length(vec3(TexCoord, 1.0)); // Simulated depth
    vec3 fragPos = viewPos + depth * normalize(vec3(TexCoord, 1.0));

    // Calculate sky gradient based on depth
    vec3 skyColor = mix(skyColorBottom, skyColorTop, (depth + 1.0) * 0.5);

    // Calculate distance to moon
    vec3 toMoon = moonPosition - fragPos;
    float moonDistance = length(toMoon);
    float moonIntensity = max(dot(normalize(toMoon), normalize(viewPos - fragPos)), 0.0);
    float moonMask = smoothstep(moonRadius, moonRadius - 0.01, moonDistance);

    // Calculate distance to sun (for ambient lighting)
    vec3 toSun = sunPosition - fragPos;
    float sunIntensity = max(dot(normalize(toSun), normalize(viewPos - fragPos)), 0.0);

    vec3 color = mix(skyColor, moonColor * moonIntensity * sunIntensity, moonMask);

    FragColor = vec4(color, 1.0);
}
