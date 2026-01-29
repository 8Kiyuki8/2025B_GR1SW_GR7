#version 330 core
out vec4 FragColor;

// --- ESTRUCTURAS ---
struct Material {
    sampler2D texture_diffuse1;  // Aquí va tu .jpg de la moto
    sampler2D texture_specular1; // Si no tienes mapa de brillo, usaremos el mismo o negro
    float shininess;
}; 

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

#define NR_POINT_LIGHTS 4

// --- ENTRADAS (Desde Vertex Shader) ---
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// --- UNIFORMS (Desde C++) ---
uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;

// Color de la niebla (Debe ser el mismo que el fondo glClearColor)
uniform vec3 fogColor;

// --- PROTOTIPOS ---
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{    
    // Normalizar vectores una sola vez
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // ========================================================
    // 1. CÁLCULO DE LUCES (Blinn-Phong)
    // ========================================================
    
    // A. Luz Direccional (Luna)
    vec3 result = CalcDirLight(dirLight, norm, viewDir);
    
    // B. Luces Puntuales (Postes)
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);    
    
    // C. Spotlight (Faro de la Moto)
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);    
    
    // ========================================================
    // 2. APLICACIÓN DE NIEBLA (FOG)
    // ========================================================
    float distance = length(viewPos - FragPos);
    
    // Ajustar estos valores según el tamaño de tu mapa (5000x5000)
    float fogStart = 200.0;   // La niebla empieza a 200 metros
    float fogEnd = 4000.0;    // La niebla es total a 4000 metros
    
    // Factor de mezcla (0.0 = Niebla pura, 1.0 = Objeto visible)
    float visibility = (fogEnd - distance) / (fogEnd - fogStart);
    visibility = clamp(visibility, 0.0, 1.0);
    
    // Mezclar el color calculado de las luces con el color de fondo
    result = mix(fogColor, result, visibility);

    FragColor = vec4(result, 1.0);
}

// --------------------------------------------------------
// IMPLEMENTACIÓN DE FUNCIONES
// --------------------------------------------------------

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // Combinar resultados usando LA TEXTURA (.jpg)
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // Combine
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // Spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // Combine
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}