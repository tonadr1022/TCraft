#version 460 core
// TODO: cleanup

in vec3 TexCoords;

out vec4 o_Color;

uniform float u_time;
// uniform samplerCube skybox;
uniform vec3 u_sun_direction; // The normalized direction of the sun
uniform vec3 sunColor = vec3(1, 0, 0); // The color of the sun (e.g., vec3(1.0, 0.9, 0.6) for a warm sun)
uniform float sunIntensity = 100; // Controls the intensity and size of the sun
uniform float moonIntensity = 300;

// Stars shader from: https://www.shadertoy.com/view/NtsBzB

// 3D Gradient noise from: https://www.shadertoy.com/view/Xsl3Dl
// replace this by something better
vec3 hash(vec3 p) {
    p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
            dot(p, vec3(269.5, 183.3, 246.1)),
            dot(p, vec3(113.5, 271.9, 124.6)));

    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}
float noise(in vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);

    vec3 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(mix(dot(hash(i + vec3(0.0, 0.0, 0.0)), f - vec3(0.0, 0.0, 0.0)),
                dot(hash(i + vec3(1.0, 0.0, 0.0)), f - vec3(1.0, 0.0, 0.0)), u.x),
            mix(dot(hash(i + vec3(0.0, 1.0, 0.0)), f - vec3(0.0, 1.0, 0.0)),
                dot(hash(i + vec3(1.0, 1.0, 0.0)), f - vec3(1.0, 1.0, 0.0)), u.x), u.y),
        mix(mix(dot(hash(i + vec3(0.0, 0.0, 1.0)), f - vec3(0.0, 0.0, 1.0)),
                dot(hash(i + vec3(1.0, 0.0, 1.0)), f - vec3(1.0, 0.0, 1.0)), u.x),
            mix(dot(hash(i + vec3(0.0, 1.0, 1.0)), f - vec3(0.0, 1.0, 1.0)),
                dot(hash(i + vec3(1.0, 1.0, 1.0)), f - vec3(1.0, 1.0, 1.0)), u.x), u.y), u.z);
}
vec3 calc_star(vec3 view_dir) {
    float stars_threshold = 8.0f; // modifies the number of stars that are visible
    float stars_exposure = 200.0f; // modifies the overall strength of the stars
    float stars = pow(clamp(noise(view_dir * 200.0f), 0.0f, 1.0f), stars_threshold) * stars_exposure;
    stars *= mix(0.4, 1.4, noise(view_dir * 100.0f + vec3(u_time))); // time based flickering

    return vec3(stars);
}

void main() {
    // Sample the skybox
    // vec3 skyboxColor = texture(skybox, TexCoords).rgb;

    // Calculate the angle between the sun direction and the fragment direction
    float sunFactor = dot(normalize(TexCoords), normalize(u_sun_direction));
    float moon_factor = dot(normalize(TexCoords), normalize(-u_sun_direction));

    // Make the sun more concentrated by raising the sunFactor to a high power
    sunFactor = pow(max(sunFactor, 0.0), sunIntensity);
    moon_factor = pow(max(moon_factor, 0.0), moonIntensity);

    // Blend the sun color with the skybox color
    vec3 finalColor = mix(vec3(0, 0, 0), sunColor, sunFactor);
    finalColor = mix(finalColor, vec3(1), moon_factor) + calc_star(normalize(TexCoords));

    // Output the final color
    o_Color = vec4(finalColor, 1.0);
}
