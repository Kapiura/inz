#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 color;
uniform bool useTexture;
uniform sampler2D clothTexture;

void main()
{
    if (useTexture)
    {
        FragColor = texture(clothTexture, TexCoord);
    }
    else
    {
        FragColor = vec4(color, 1.0);
    }
}