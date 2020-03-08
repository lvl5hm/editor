#shader vertex

#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 tex;
layout(location = 2) in mat4x4 model;
layout(location = 6) in vec4 color;

out vec2 tex_coord;
out vec4 fr_color;

void main() {
  mat4x4 u_model = transpose(model);
  gl_Position = u_model*vec4(position, 0.0f, 1.0f);
  
  fr_color = color/255.0f;
  tex_coord = tex.xy + position.xy*tex.zw;
}





#shader fragment

#version 330 core
in vec2 tex_coord;
in vec4 fr_color;

out vec4 color;
uniform sampler2D texture_image;

void main() {
  ivec2 atlas_size = textureSize(texture_image, 0);
  
  vec2 uv = floor(tex_coord) + 0.5; // snaps to the nearest pixel
  uv += 1.0 - clamp((1.0 - fract(tex_coord)), 0.0, 1.0); // sets the blend coordinates
  
  color = texture(texture_image, uv/atlas_size)*fr_color;
}
