#version 330 core
out vec4 FragColor;

// Esta variable nos permite enviarle un color desde C++
uniform vec3 lightColor; 

void main()
{
    // Pintamos el objeto del color que recibimos, con 1.0 de opacidad
    FragColor = vec4(lightColor, 1.0);
}