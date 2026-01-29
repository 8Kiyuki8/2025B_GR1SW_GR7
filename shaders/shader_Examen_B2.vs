#version 330 core

// --- ATRIBUTOS DE ENTRADA ---
// Deben coincidir con los números en tu C++ (glVertexAttribPointer)
layout (location = 0) in vec3 aPos;       // Posición
layout (location = 1) in vec3 aNormal;    // Normal (para luces)
layout (location = 2) in vec2 aTexCoords; // Textura

// --- SALIDAS HACIA EL FRAGMENT SHADER ---
out vec3 FragPos;    // Posición real en el mundo 3D
out vec3 Normal;     // Dirección de la superficie corregida
out vec2 TexCoords;  // Coordenadas de la imagen

// --- MATRICES DE TRANSFORMACIÓN ---
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // 1. Calcular la posición del fragmento en el mundo (World Space)
    // Es vital hacer esto ANTES de aplicar la cámara (view/projection) para que la iluminación sea correcta.
    FragPos = vec3(model * vec4(aPos, 1.0));

    // 2. Calcular la Normal Corregida (Matriz Normal)
    // Esto es CRÍTICO: Como escalamos el piso a 5000.0 y la moto a 0.005,
    // las normales se deformarían si usamos solo la matriz 'model'.
    // Esta fórmula (inversa transpuesta) arregla la luz en objetos estirados.
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // 3. Pasar las coordenadas de textura tal cual
    TexCoords = aTexCoords;
    
    // 4. Calcular la posición final en la pantalla (Clip Space)
    gl_Position = projection * view * vec4(FragPos, 1.0);
}