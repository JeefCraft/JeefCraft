#version 120

varying vec3 vNormal;
varying vec3 pos;
varying vec2 vUvs;
varying vec2 vLightData;

uniform sampler2D textureAtlas;

const vec3 sun_dir = vec3(0.32, 0.75, 0.54);
const vec3 sun_color = vec3(1.4, 1.2, 0.4);
const vec4 ambient = vec4(0.4, 0.4, 0.5, 0.0);

#define MIN_LIGHT_CONSTANT 2.0
#define MAX_LIGHT_CONSTANT 15.0

void main() {
	vec4 diffuse = texture2D(textureAtlas, vUvs);
	float cosTheta = clamp(dot(vNormal, sun_dir), 0.0, 1.0);
	vec4 sun_color_theta = vec4(sun_color * cosTheta, 1.0) + ambient;

	float light = clamp(vLightData.x, MIN_LIGHT_CONSTANT, MAX_LIGHT_CONSTANT) / MAX_LIGHT_CONSTANT;
	vec3 torchLight = vec3(light);
	sun_color_theta *= vec4(torchLight, 0.0);

	gl_FragColor = diffuse * sun_color_theta;
}