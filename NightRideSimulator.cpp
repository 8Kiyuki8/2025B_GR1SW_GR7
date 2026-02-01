#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <irrKlang/irrKlang.h>

#include <iostream>
#include <vector>
#include <cmath>

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
const glm::vec3 initialBikePos = glm::vec3(0.0f, -0.5f, 0.0f);
const float initialBikeAngle = 0.0f;

// --- ESTADOS ---
bool isFirstPerson = false;
bool vKeyPressed = false;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Recursos Globales
unsigned int planeVAO, planeVBO, floorTexture;
glm::vec3 fogColor = glm::vec3(0.0f, 0.05f, 0.15f);
bool isBraking = false;                // Para saber si la tecla S está presionada
unsigned int cubeVAO = 0, cubeVBO = 0; // Para poder dibujar el cubo
void renderCube();                     // Prototipo de la función

// LUNA
glm::vec3 moonPos = glm::vec3(0.0f, 100.0f, 300.0f);

// --- FÍSICA DE LA MOTO ---
float currentSpeed = 0.0f;
float acceleration = 15.0f;
float deceleration = 25.0f;
float brakingPower = 80.0f;
float maxSpeedNormal = 90.0f;
float maxSpeedTurbo = 120.0f;

// --- SONIDO ---
irrklang::ISoundEngine *soundEngine = nullptr;
irrklang::ISound *engineSound = nullptr;
float collisionCooldown = 0.0f;

glm::vec3 oldBikePos;

// --- MONEDAS / TIEMPO ---
std::vector<glm::vec3> coinPositions;
std::vector<bool> coinCollected;
int totalCoins = 0;
int desiredCoinCount = 16;
float totalTimeLimit = 90.0f;
float timeLeft = 90.0f;
bool gameWon = false;
bool gameLost = false;

// Posiciones de las luces REALES (Solo 4 para rendimiento)
glm::vec3 pointLightPositions[] = {
    glm::vec3(0.0f, 4.5f, 100.0f),
    glm::vec3(0.0f, 4.5f, 60.0f),
    glm::vec3(0.0f, 4.5f, 20.0f),
    glm::vec3(0.0f, 4.5f, -20.0f)};

bool checkCollision(glm::vec3 pos1, float radius1, glm::vec3 pos2, float radius2) {
    float distance = glm::distance(glm::vec2(pos1.x, pos1.z), glm::vec2(pos2.x, pos2.z));
    return distance < (radius1 + radius2);
}

int main()
{
    // 1. INICIALIZACIÓN
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Night Ride - Final", NULL, NULL);
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

    camera.Yaw = -90.0f;

    // 2. SHADERS
    Shader ourShader("shaders/shader_Examen_B2.vs", "shaders/shader_Examen_B2.fs");
    Shader lampShader("shaders/lamp.vs", "shaders/lamp.fs");

    // 2.1 SONIDO
    soundEngine = irrklang::createIrrKlangDevice();
    if (!soundEngine)
    {
        std::cout << "No se pudo iniciar el sonido (irrKlang)." << std::endl;
    }
    else
    {
        engineSound = soundEngine->play2D("third_party/LearnOpenGL-master/resources/audio/breakout.mp3", true, false, true);
        if (engineSound)
            engineSound->setVolume(0.2f);
    }

    // =================================================================================
    // 3. CARGAR MODELOS
    // =================================================================================
    stbi_set_flip_vertically_on_load(true);

    // MOTO
    Model moto("model/motorbike/motorbike.obj");

    // POSTE DE LUZ
    Model poste("model/poste_de_luz/poste_de_luz.obj");

    // ARBOL
    Model arbol("model/arbol/arbol.obj");

    // ---> AGREGADO: LA CASA <---
    Model casaModel("model/casa/casa.obj");

    stbi_set_flip_vertically_on_load(false);
    // =================================================================================

    // =================================================================================
    // --- MONEDAS (INICIALIZACIÓN) ---
    // =================================================================================
    coinPositions.clear();
    float coinStartZ = 80.0f;
    float coinEndZ = -600.0f;
    int coinCount = desiredCoinCount < 1 ? 1 : desiredCoinCount;
    float coinSpacing = (coinCount > 1) ? (coinStartZ - coinEndZ) / (coinCount - 1) : 0.0f;
    float coinXLeft = -6.0f;
    float coinXRight = 6.0f;
    bool leftSide = true;
    for (int i = 0; i < coinCount; ++i)
    {
        float z = coinStartZ - (coinSpacing * i);
        float x = leftSide ? coinXLeft : coinXRight;
        coinPositions.push_back(glm::vec3(x, 0.5f, z));
        leftSide = !leftSide;
    }
    totalCoins = static_cast<int>(coinPositions.size());
    coinCollected.assign(totalCoins, false);
    timeLeft = totalTimeLimit;

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

        if (collisionCooldown > 0.0f)
            collisionCooldown -= deltaTime;

        if (!gameWon && !gameLost)
        {
            timeLeft -= deltaTime;
            if (timeLeft <= 0.0f)
            {
                timeLeft = 0.0f;
                gameLost = true;
            }
        }

        if (!gameWon && !gameLost)
        {
            processInput(window);
        }
        else
        {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            {
                bikePos = initialBikePos;
                bikeAngle = initialBikeAngle;
                oldBikePos = bikePos;
                currentSpeed = 0.0f;
                isFirstPerson = false;
                isBraking = false;
                collisionCooldown = 0.0f;
                timeLeft = totalTimeLimit;
                gameWon = false;
                gameLost = false;
                for (size_t i = 0; i < coinCollected.size(); ++i)
                    coinCollected[i] = false;
            }
            else
            {
                currentSpeed = 0.0f;
            }
        }

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

        // =========================================================
        // --- LUCES DE FRENO (CONFIGURACIÓN FINAL) ---
        // =========================================================
        lampShader.use();
        lampShader.setMat4("projection", projection);
        lampShader.setMat4("view", view);

        // Color: Rojo Brillante (1.0) si frena, Rojo Oscuro (0.4) si no
        glm::vec3 tailColor = isBraking ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.4f, 0.0f, 0.0f);
        lampShader.setVec3("lightColor", tailColor);

        // --- CALIBRACIÓN DE POSICIÓN ---
        float h = 1.1f;     // Altura
        float backX = 4.3f; // Profundidad (Atrás)
        float sepZ = 0.20f; // Separación entre focos

        // Variable para alinear a la derecha (Corrige el desfase del modelo)
        float ajusteDerecha = -0.26f;

        // Escala del rectángulo
        glm::vec3 scaleLight = glm::vec3(0.15f, 0.09f, 0.09f);

        // --- LUZ 1 (Izquierda) ---
        model = glm::mat4(1.0f);
        model = glm::translate(model, bikePos);
        model = glm::rotate(model, glm::radians(bikeAngle - 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Aplicamos el ajuste lateral
        model = glm::translate(model, glm::vec3(backX, h, -sepZ + ajusteDerecha));

        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scaleLight);
        lampShader.setMat4("model", model);
        renderCube();

        // --- LUZ 2 (Derecha) ---
        model = glm::mat4(1.0f);
        model = glm::translate(model, bikePos);
        model = glm::rotate(model, glm::radians(bikeAngle - 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Aplicamos el ajuste lateral
        model = glm::translate(model, glm::vec3(backX, h, sepZ + ajusteDerecha));

        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scaleLight);
        lampShader.setMat4("model", model);
        renderCube();

        // =========================================================
        // --- FARO DELANTERO (CENTRADO) ---
        // =========================================================
        lampShader.use();
        lampShader.setMat4("projection", projection);
        lampShader.setMat4("view", view);

        // 1. Color BLANCO intenso
        lampShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

        // 2. Posición
        float frontX = 0.6f;     // Tu valor
        float alturaFaro = 1.6f; // Tu valor

        // Corrección de centro (Mismo valor que las luces traseras)
        // Esto mueve la luz hacia la izquierda de la pantalla para centrarla.
        float correccionCentro = -0.26f;

        model = glm::mat4(1.0f);
        model = glm::translate(model, bikePos);

        // Rotamos igual que la moto
        model = glm::rotate(model, glm::radians(bikeAngle - 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Nos movemos al frente (frontX), arriba (alturaFaro) y CENTRO (correccionCentro)
        model = glm::translate(model, glm::vec3(frontX, alturaFaro, correccionCentro));

        // Hacemos la esfera pequeña
        model = glm::scale(model, glm::vec3(0.15f));

        lampShader.setMat4("model", model);
        renderSphere();

        // =================================================================================
        // --- MAPA: AVENIDA CENTRAL ---
        // =================================================================================

        bool collided = false;

        // --- VARIABLES DE CALIBRACIÓN (TUS VALORES ORIGINALES) ---
        float scalePoste = 350.0f;
        float scaleArbol = 30.0f;

        // 1. CENTRADO DEL POSTE (Variable existente - NO TOCADA)
        float ajusteCentroX = -6.5f;

        // 2. AJUSTE FINO DE BOMBILLAS (NO TOCADO)
        float alturaFoco = 13.0f;
        float distanciaBrazo = 3.0f;

        // ---> ¡VARIABLE TUYA! CORRECCIÓN HORIZONTAL (NO TOCADA) <---
        float correccionLucesX = 7.0f;

        // 3. SEPARACIÓN DE ÁRBOLES
        float treeDist = 35.0f;

        float startZ = 100.0f;
        float endZ = -2000.0f;
        float posteSpacing = 40.0f;
        float treeSpacing = 20.0f;

        // A) BUCLE DE POSTES CENTRALES (TU LÓGICA INTACTA)
        for (float z = startZ; z > endZ; z -= posteSpacing)
        {
            // --- POSTE ---
            glm::vec3 postePos = glm::vec3(ajusteCentroX + 7.0f, -0.5f, z);

            // --- DETECCIÓN DE COLISIÓN ---
            // Radio moto, Radio poste
            if (checkCollision(bikePos, 0.8f, postePos, 0.5f))
            {
                bikePos = oldBikePos; // Resetear posición
                currentSpeed = 0;     // Detener moto
                collided = true;
            }

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
            lampShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
            lampShader.setMat4("projection", projection);
            lampShader.setMat4("view", view);

            // Foco Izquierdo
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(ajusteCentroX - distanciaBrazo + correccionLucesX, alturaFoco, z));
            model = glm::scale(model, glm::vec3(0.35f));
            lampShader.setMat4("model", model);
            renderSphere();

            // Foco Derecho
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(ajusteCentroX + distanciaBrazo + correccionLucesX, alturaFoco, z));
            model = glm::scale(model, glm::vec3(0.35f));
            lampShader.setMat4("model", model);
            renderSphere();
        }

        // B) BUCLE DE ÁRBOLES (TU LÓGICA INTACTA)
        ourShader.use();
        ourShader.setVec3("spotLight.diffuse", 0.8f, 0.8f, 0.8f);
        for (float z = startZ; z > endZ; z -= treeSpacing)
        {
            // --- Árbol Derecho ---
            glm::vec3 posArbolDer = glm::vec3(treeDist, -0.5f, z);
            if (checkCollision(bikePos, 0.8f, posArbolDer, 0.5f)) // Radio de árbol un poco más ancho
            {
                bikePos = oldBikePos;
                currentSpeed = 0.0f;
                collided = true;
            }
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(treeDist, -0.5f, z));
            model = glm::scale(model, glm::vec3(scaleArbol));
            ourShader.setMat4("model", model);
            arbol.Draw(ourShader);

            // --- Árbol Izquierdo ---
                glm::vec3 posArbolIzq = glm::vec3(-treeDist, -0.5f, z);
            if (checkCollision(bikePos, 0.8f, posArbolIzq, 0.5f))
            {
                bikePos = oldBikePos;
                currentSpeed = 0.0f;
                collided = true;
            }
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-treeDist, -0.5f, z));
            model = glm::scale(model, glm::vec3(scaleArbol));
            ourShader.setMat4("model", model);
            arbol.Draw(ourShader);
        }

        // ---> C) BUCLE DE CASAS (MODIFICADO: MENOS CASAS) <---
        float distCasas = 55.0f;
        float scaleCasa = 0.02f;

        // ¡AQUÍ ESTÁ EL TRUCO!
        // Aumenta este número para separar más las casas.
        // 20.0f = Muchas casas (pegadas). 100.0f = Pocas casas (dispersas).
        float houseSpacing = 150.0f;

        for (float z = startZ; z > endZ; z -= houseSpacing)
        {
            // Casa Izquierda
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-distCasas, -0.5f, z));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(scaleCasa));
            ourShader.setMat4("model", model);
            casaModel.Draw(ourShader);

            // Casa Derecha
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(distCasas, -0.5f, z));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(scaleCasa));
            ourShader.setMat4("model", model);
            casaModel.Draw(ourShader);
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

        // --- MONEDAS (COLECCIÓN Y RENDER) ---
        int collectedCount = 0;
        for (size_t i = 0; i < coinPositions.size(); ++i)
        {
            if (coinCollected[i])
            {
                collectedCount++;
                continue;
            }

            if (!gameWon && !gameLost && checkCollision(bikePos, 0.8f, coinPositions[i], 0.6f))
            {
                coinCollected[i] = true;
                collectedCount++;
                if (soundEngine)
                    soundEngine->play2D("third_party/LearnOpenGL-master/resources/audio/powerup.wav", false);
                continue;
            }

            lampShader.use();
            lampShader.setMat4("projection", projection);
            lampShader.setMat4("view", view);
            lampShader.setVec3("lightColor", glm::vec3(1.0f, 0.85f, 0.1f));
            model = glm::mat4(1.0f);
            model = glm::translate(model, coinPositions[i]);
            model = glm::scale(model, glm::vec3(0.5f));
            lampShader.setMat4("model", model);
            renderSphere();
        }

        if (!gameWon && !gameLost && collectedCount >= totalCoins)
        {
            gameWon = true;
        }

        // --- SONIDO (motor y colisión) ---
        if (engineSound)
        {
            float speedAbs = fabs(currentSpeed);
            float t = speedAbs / maxSpeedTurbo;
            if (t > 1.0f)
                t = 1.0f;
            float baseVolume = 0.15f;
            float maxVolume = 0.7f;
            engineSound->setVolume(baseVolume + (maxVolume - baseVolume) * t);
        }

        if (collided && collisionCooldown <= 0.0f && soundEngine)
        {
            soundEngine->play2D("third_party/LearnOpenGL-master/resources/audio/bleep.wav", false);
            collisionCooldown = 0.5f;
        }

        // --- VELOCÍMETRO (barra 2D) ---
        float speedAbs = fabs(currentSpeed);
        float speedNorm = speedAbs / maxSpeedTurbo;
        if (speedNorm > 1.0f)
            speedNorm = 1.0f;

        glDisable(GL_DEPTH_TEST);
        lampShader.use();
        glm::mat4 ortho = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT, -1.0f, 1.0f);
        lampShader.setMat4("projection", ortho);
        lampShader.setMat4("view", glm::mat4(1.0f));

        float barWidth = 260.0f;
        float barHeight = 18.0f;
        float barX = 20.0f;
        float barY = 20.0f;

        // Fondo
        lampShader.setVec3("lightColor", glm::vec3(0.15f, 0.15f, 0.15f));
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(barX + barWidth * 0.5f, barY + barHeight * 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(barWidth, barHeight, 1.0f));
        lampShader.setMat4("model", model);
        renderCube();

        // Barra de velocidad
        glm::vec3 speedColor = glm::mix(glm::vec3(0.1f, 0.9f, 0.2f), glm::vec3(0.9f, 0.1f, 0.1f), speedNorm);
        lampShader.setVec3("lightColor", speedColor);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(barX + (barWidth * speedNorm) * 0.5f, barY + barHeight * 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(barWidth * speedNorm, barHeight, 1.0f));
        lampShader.setMat4("model", model);
        renderCube();
        glEnable(GL_DEPTH_TEST);

        int velocidadDisplay = abs((int)currentSpeed);
        int remainingCoins = totalCoins - collectedCount;
        std::string status = "";
        if (gameWon)
            status = " | GANASTE";
        else if (gameLost)
            status = " | PERDISTE";

        std::string title = "Night Ride | Velocidad: " + std::to_string(velocidadDisplay) +
                            " km/h | Monedas: " + std::to_string(remainingCoins) +
                            " | Tiempo: " + std::to_string((int)timeLeft) + "s" + status;
        glfwSetWindowTitle(window, title.c_str());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glfwTerminate();
    if (engineSound)
        engineSound->drop();
    if (soundEngine)
        soundEngine->drop();
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

    // GUARDAR POSICIÓN ANTES DE MOVER
    oldBikePos = bikePos;

    bikePos.x += -sin(glm::radians(bikeAngle)) * currentSpeed * deltaTime;
    bikePos.z += -cos(glm::radians(bikeAngle)) * currentSpeed * deltaTime;

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
        isBraking = true;
        if (currentSpeed > -10.0f)
            currentSpeed -= brakingPower * deltaTime;
    }
    else
    {
        isBraking = false;
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

void renderCube()
{
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // Posición            // Normales         // UVs
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindVertexArray(cubeVAO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}