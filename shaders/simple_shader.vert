#version 450

// Input from the vertex buffer
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 v_fragColor;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 v_fragTexCoord;

layout(location = 0) out vec3 f_fragColor;
layout(location = 1) out vec2 f_fragTexCoord;

layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projectionViewMatrix;
	vec3 directionToLight;
} ubo;

// Model matrix and a pretty normal matrix
layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

const float AMBIENT = 0.12;

void main() {
  gl_Position = ubo.projectionViewMatrix * push.modelMatrix * vec4(position, 1.0);

  vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal);

  float lightIntensity = AMBIENT + max(dot(normalWorldSpace, ubo.directionToLight), 0);

  f_fragColor = lightIntensity * v_fragColor;
  f_fragTexCoord = v_fragTexCoord;
}
