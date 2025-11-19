#version 450

layout(location = 0) in vec3 fragColor;   // 物体本来的颜色
layout(location = 1) in vec3 fragNormal;  // 法线
layout(location = 2) in vec3 fragPos;     // 世界坐标

layout(location = 0) out vec4 outColor;

void main() {
  // --- 定义一个光源 ---
  // 假设光源在右上方 (10, 10, 10) 的位置
  vec3 lightPos = vec3(10.0, 10.0, 10.0);
  vec3 lightColor = vec3(1.0, 1.0, 1.0);  // 白光

  // --- 1. 环境光 (Ambient) ---
  // 即使没有光直接照到的地方，也不应该是纯黑的，给一点点底色
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * lightColor;

  // --- 2. 漫反射 (Diffuse) ---
  // 核心算法：计算 法线 和 光线方向 的夹角。
  // 夹角越小（正对着光），这一项就越大，也就越亮。

  // 归一化法线（插值过程中长度可能会变，所以要重新 normalize）
  vec3 norm = normalize(fragNormal);
  // 计算从物体指向光源的向量
  vec3 lightDir = normalize(lightPos - fragPos);

  // 点乘计算夹角余弦值。max(..., 0.0) 确保背对光的一面是黑的，不会变成负数
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * lightColor;

  // --- 3. 合成最终颜色 ---
  // (环境光 + 漫反射光) * 物体本身的颜色
  vec3 result = (ambient + diffuse) * fragColor;

  outColor = vec4(result, 1.0);
}