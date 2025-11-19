#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;  // 这是你之前生成的彩虹色

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
}
ubo;

// 输出给 Frag Shader
layout(location = 0) out vec3 fragColor;   // 传递物体本来的颜色
layout(location = 1) out vec3 fragNormal;  // 传递法线（朝向）
layout(location = 2) out vec3 fragPos;     // 传递世界坐标（用于计算光的方向）

void main() {
  // 1. 计算最终裁切空间的坐标 (给 GPU 画图用)
  gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

  // 2. 传递物体颜色
  fragColor = inColor;

  // 3. 计算世界空间下的法线
  // 因为球心在原点，法线就是位置向量。
  // mat3(ubo.model) 是为了只取旋转部分，不取位移，让法线跟着球旋转
  fragNormal = mat3(ubo.model) * normalize(inPosition);

  // 4. 传递世界空间下的位置
  fragPos = vec3(ubo.model * vec4(inPosition, 1.0));
}