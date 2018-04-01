#version 120

attribute vec4 position;
attribute vec2 uvs;
attribute vec2 lightData;

varying vec3 vNormal;
varying vec3 pos;
varying vec2 vUvs;
varying vec2 vLightData;

uniform mat4 projViewMatrix;
uniform mat4 modelMatrix;

void main() {
	vec3 cNormals[6];
	cNormals[0] = vec3(1.0,0.0,0.0);  // East
	cNormals[1] = vec3(0.0,1.0,0.0);  // Up
	cNormals[2] = vec3(-1.0,0.0,0.0); // West
	cNormals[3] = vec3(0.0,-1.0,0.0); // Down
	cNormals[4] = vec3(0.0,0.0,1.0);  // North
	cNormals[5] = vec3(0.0,0.0,-1.0); // South

	mat4 mvp = projViewMatrix * modelMatrix;
	gl_Position = mvp * vec4(position.xyz, 1.0);
	vNormal = cNormals[int(position.w)];
	pos = vec3(position);
	vUvs = uvs;
	vLightData = lightData;
}