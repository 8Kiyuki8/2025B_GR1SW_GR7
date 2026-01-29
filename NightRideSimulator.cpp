#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <learnopengl/stb_image.h>

// --- PROTOTIPOS ---
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(char const *path);
void renderSphere();

// --- CONFIGURACIÓN ---
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 600;

// --- CÁMARA ---
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// --- JUGADOR (MOTO) ---
glm::vec3 bikePos = glm::vec3(0.0f, -0.5f, 0.0f);
float bikeAngle = 0.0f;

// --- ESTADOS ---
bool isFirstPerson = false;
bool vKeyPressed = false;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Recursos Globales
unsigned int planeVAO, planeVBO, floorTexture;
glm::vec3 fogColor = glm::vec3(0.0f, 0.05f, 0.15f);

// LUNA
glm::vec3 moonPos = glm::vec3(0.0f, 100.0f, 300.0f);

// --- FÍSICA DE LA MOTO ---
float currentSpeed = 0.0f;
float acceleration = 15.0f;
float deceleration = 25.0f;
float maxSpeedNormal = 90.0f;
float maxSpeedTurbo = 120.0f;

// Posiciones de las luces REALES (Solo 4 para rendimiento)
glm::vec3 pointLightPositions[] = {
    glm::vec3(0.0f, 4.5f, 100.0f),
    glm::vec3(0.0f, 4.5f, 60.0f),
    glm::vec3(0.0f, 4.5f, 20.0f),
    glm::vec3(0.0f, 4.5f, -20.0f)};

int main()
{
    // 1. INICIALIZACIÓN
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Night Ride - Arboles y Postes", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;
    glEnable(GL_DEPTH_TEST);

    camera.Yaw = 90.0f;

    // 2. SHADERS
    Shader ourShader("shaders/shader_Examen_B2.vs", "shaders/shader_Examen_B2.fs");
    Shader lampShader("shaders/lamp.vs", "shaders/lamp.fs");

    // =================================================================================
    // 3. CARGAR MODELOS
    // =================================================================================
    stbi_set_flip_vertically_on_load(true);

    // MOTO
    Model moto("C:/Users/pc/Documents/Visual Studio 2022/OpenGL/OpenGL/model/motorbike/motorbike.obj");

    // POSTE DE LUZ
    Model poste("C:/Users/pc/Documents/Visual Studio 2022/OpenGL/OpenGL/model/poste_de_luz/poste_de_luz.obj");

    // ARBOL (La ruta que me diste)
    Model arbol("C:/Users/pc/Documents/Visual Studio 2022/OpenGL/OpenGL/model/arbol/arbol.obj");

    stbi_set_flip_vertically_on_load(false);
    // =================================================================================

    // 4. PISO GIGANTE
    float planeVertices[] = {
        5000.0f, -0.5f, 5000.0f, 0.0f, 1.0f, 0.0f, 500.0f, 0.0f,
        -5000.0f, -0.5f, 5000.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        -5000.0f, -0.5f, -5000.0f, 0.0f, 1.0f, 0.0f, 0.0f, 500.0f,
        5000.0f, -0.5f, 5000.0f, 0.0f, 1.0f, 0.0f, 500.0f, 0.0f,
        -5000.0f, -0.5f, -5000.0f, 0.0f, 1.0f, 0.0f, 0.0f, 500.0f,
        5000.0f, -0.5f, -5000.0f, 0.0f, 1.0f, 0.0f, 500.0f, 500.0f};
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

    floorTexture = loadTexture("textures/suelo.png");

    std::cout << "LISTO. SOLO POSTES Y ARBOLES." << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // --- CÁMARA ---
        if (isFirstPerson)
        {
            float seatOffset = 3.0f;
            float seatX = sin(glm::radians(bikeAngle)) * seatOffset;
            float seatZ = cos(glm::radians(bikeAngle)) * seatOffset;
            float eyeHeight = 2.5f;
            float sideOffset = 0.35f;
            float sideX = cos(glm::radians(bikeAngle)) * sideOffset;
            float sideZ = -sin(glm::radians(bikeAngle)) * sideOffset;
            camera.Position = bikePos + glm::vec3(seatX + sideX, eyeHeight, seatZ + sideZ);
            glm::vec3 front;
            front.x = -sin(glm::radians(bikeAngle));
            front.y = -0.1f;
            front.z = -cos(glm::radians(bikeAngle));
            camera.Front = glm::normalize(front);
            camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0.0f, 1.0f, 0.0f)));
            camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
        }
        else
        {
            float distance = 20.0f;
            float height = 8.0f;
            float lookAtH = 3.0f;
            float camX = cos(glm::radians(camera.Yaw)) * cos(glm::radians(camera.Pitch)) * distance;
            float camZ = sin(glm::radians(camera.Yaw)) * cos(glm::radians(camera.Pitch)) * distance;
            float camY = sin(glm::radians(camera.Pitch)) * distance;
            camera.Position = bikePos + glm::vec3(camX, camY + height, camZ);
            if (camera.Position.y < 0.2f)
                camera.Position.y = 0.2f;
            glm::vec3 cameraTarget = bikePos + glm::vec3(0.0f, lookAtH, 0.0f);
            camera.Front = glm::normalize(cameraTarget - camera.Position);
            camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0.0f, 1.0f, 0.0f)));
            camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
        }

        // --- RENDER ---
        glClearColor(fogColor.x, fogColor.y, fogColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 6000.0f);
        glm::mat4 view = camera.GetViewMatrix();

        ourShader.use();
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        ourShader.setVec3("fogColor", fogColor);

        // LUCES
        ourShader.setVec3("dirLight.direction", -moonPos);
        ourShader.setVec3("dirLight.ambient", 0.3f, 0.3f, 0.4f);
        ourShader.setVec3("dirLight.diffuse", 0.6f, 0.6f, 0.7f);
        ourShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        for (int i = 0; i < 4; i++)
        {
            std::string number = std::to_string(i);
            ourShader.setVec3("pointLights[" + number + "].position", pointLightPositions[i]);
            ourShader.setVec3("pointLights[" + number + "].ambient", 0.05f, 0.05f, 0.05f);
            ourShader.setVec3("pointLights[" + number + "].diffuse", 1.0f, 0.8f, 0.4f);
            ourShader.setVec3("pointLights[" + number + "].specular", 1.0f, 1.0f, 1.0f);
            ourShader.setFloat("pointLights[" + number + "].constant", 1.0f);
            ourShader.setFloat("pointLights[" + number + "].linear", 0.022f);
            ourShader.setFloat("pointLights[" + number + "].quadratic", 0.0019f);
        }

        glm::vec3 bikeFront;
        bikeFront.x = -sin(glm::radians(bikeAngle));
        bikeFront.y = 0.0f;
        bikeFront.z = -cos(glm::radians(bikeAngle));
        ourShader.setVec3("spotLight.position", bikePos + glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setVec3("spotLight.direction", glm::normalize(bikeFront));
        ourShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight.diffuse", 5.0f, 5.0f, 5.0f);
        ourShader.setVec3("spotLight.specular", 5.0f, 5.0f, 5.0f);
        ourShader.setFloat("spotLight.constant", 1.0f);
        ourShader.setFloat("spotLight.linear", 0.022f);
        ourShader.setFloat("spotLight.quadratic", 0.0019f);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(20.0f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(25.0f)));

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // PISO
        glm::mat4 model = glm::mat4(1.0f);
        ourShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        ourShader.setInt("material.texture_diffuse1", 0);
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // MOTO
        model = glm::mat4(1.0f);
        model = glm::translate(model, bikePos);
        model = glm::rotate(model, glm::radians(bikeAngle - 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
        ourShader.setMat4("model", model);
        moto.Draw(ourShader);

        // =================================================================================
        // --- MAPA: AVENIDA CENTRAL (CORRECCIÓN FINAL DE POSICIÓN DE LUCES) ---
        // =================================================================================

        // --- VARIABLES DE CALIBRACIÓN ---
        float scalePoste = 350.0f;
        float scaleArbol = 30.0f;

        // 1. CENTRADO DEL POSTE (Variable existente)
        float ajusteCentroX = -6.5f;

        // 2. AJUSTE FINO DE BOMBILLAS
        float alturaFoco = 13.0f;   // Altura correcta
        float distanciaBrazo = .0f; // Separación entre bombillas

        // ---> ¡NUEVA VARIABLE! CORRECCIÓN HORIZONTAL <---
        // Como las luces están a la izquierda, sumamos para moverlas a la DERECHA.
        // Prueba con 2.0f. Si es mucho, baja a 1.0f. Si es poco, sube a 3.0f.
        float correccionLucesX = 7.0f;

        // 3. SEPARACIÓN DE ÁRBOLES
        float treeDist = 35.0f;

        float startZ = 100.0f;
        float endZ = -2000.0f;
        float posteSpacing = 40.0f;
        float treeSpacing = 20.0f;

        // A) BUCLE DE POSTES CENTRALES
        for (float z = startZ; z > endZ; z -= posteSpacing)
        {
            // --- POSTE ---
            ourShader.use();
            ourShader.setVec3("spotLight.diffuse", 0.5f, 0.5f, 0.5f);
            ourShader.setVec3("spotLight.specular", 0.5f, 0.5f, 0.5f);

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(ajusteCentroX, -0.5f, z));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(scalePoste));
            ourShader.setMat4("model", model);
            poste.Draw(ourShader);

            // --- BOMBILLAS (LUCES) ---
            lampShader.use();
            lampShader.setMat4("projection", projection);
            lampShader.setMat4("view", view);

            // Foco Izquierdo
            model = glm::mat4(1.0f);
            // ¡AQUÍ SUMAMOS LA CORRECCIÓN! (+ correccionLucesX)
            model = glm::translate(model, glm::vec3(ajusteCentroX - distanciaBrazo + correccionLucesX, alturaFoco, z));
            model = glm::scale(model, glm::vec3(0.35f));
            lampShader.setMat4("model", model);
            renderSphere();

            // Foco Derecho
            model = glm::mat4(1.0f);
            // ¡AQUÍ TAMBIÉN! (+ correccionLucesX)
            model = glm::translate(model, glm::vec3(ajusteCentroX + distanciaBrazo + correccionLucesX, alturaFoco, z));
            model = glm::scale(model, glm::vec3(0.35f));
            lampShader.setMat4("model", model);
            renderSphere();
        }

        // B) BUCLE DE ÁRBOLES (Igual que antes)
        ourShader.use();
        ourShader.setVec3("spotLight.diffuse", 0.8f, 0.8f, 0.8f);
        for (float z = startZ; z > endZ; z -= treeSpacing)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(treeDist, -0.5f, z));
            model = glm::scale(model, glm::vec3(scaleArbol));
            ourShader.setMat4("model", model);
            arbol.Draw(ourShader);

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-treeDist, -0.5f, z));
            model = glm::scale(model, glm::vec3(scaleArbol));
            ourShader.setMat4("model", model);
            arbol.Draw(ourShader);
        }
        // Restaurar luces fuertes
        ourShader.setVec3("spotLight.diffuse", 5.0f, 5.0f, 5.0f);
        ourShader.setVec3("spotLight.specular", 5.0f, 5.0f, 5.0f);

        // LUNA
        lampShader.use();
        lampShader.setMat4("projection", projection);
        lampShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, moonPos);
        model = glm::scale(model, glm::vec3(15.0f));
        lampShader.setMat4("model", model);
        renderSphere();

        int velocidadDisplay = abs((int)currentSpeed);
        std::string title = "Night Ride | Velocidad: " + std::to_string(velocidadDisplay) + " km/h";
        glfwSetWindowTitle(window, title.c_str());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glfwTerminate();
    return 0;
}

// --- FUNCIONES AUXILIARES (Sin cambios) ---
unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = (nrComponents == 1) ? GL_RED : (nrComponents == 3 ? GL_RGB : GL_RGBA);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else
    {
        std::cout << "ERROR TEXTURA: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
    {
        if (!vKeyPressed)
        {
            isFirstPerson = !isFirstPerson;
            vKeyPressed = true;
        }
    }
    else
    {
        vKeyPressed = false;
    }

    float speedLimit = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? maxSpeedTurbo : maxSpeedNormal;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        if (currentSpeed < speedLimit)
            currentSpeed += acceleration * deltaTime;
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        if (currentSpeed > -10.0f)
            currentSpeed -= acceleration * deltaTime;
    }
    else
    {
        if (currentSpeed > 0.1f)
            currentSpeed -= deceleration * deltaTime;
        else if (currentSpeed < -0.1f)
            currentSpeed += deceleration * deltaTime;
        else
            currentSpeed = 0.0f;
    }

    if (abs(currentSpeed) > 0.1f)
    {
        float turnSpeed = 100.0f * deltaTime;
        float direction = (currentSpeed > 0) ? 1.0f : -1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            bikeAngle += turnSpeed * direction;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            bikeAngle -= turnSpeed * direction;
    }

    bikePos.x += -sin(glm::radians(bikeAngle)) * currentSpeed * deltaTime;
    bikePos.z += -cos(glm::radians(bikeAngle)) * currentSpeed * deltaTime;
    bikePos.y = -0.5f;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) { glViewport(0, 0, width, height); }

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) { camera.ProcessMouseScroll(static_cast<float>(yoffset)); }

unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);
        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;
        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }
        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow)
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());
        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        float stride = (3 + 3 + 2) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));
    }
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}