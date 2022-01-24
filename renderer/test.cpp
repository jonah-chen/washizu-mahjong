#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "mesh.hpp"
#include <iostream>

int main()
{
    std::vector <unsigned int> indices;
    std::vector <vertex3d> vertices;
    load_obj("a.obj", indices, vertices);

    std::cout << "indices.size(): " << indices.size() << std::endl;
    std::cout << "vertices.size(): " << vertices.size() << std::endl;

    if (!glfwInit())
    {
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    GLFWwindow* window {glfwCreateWindow(800, 600, "test", nullptr, nullptr)};
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    if (glewInit() != GLEW_OK)
    {
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex3d), &vertices[0], GL_STATIC_DRAW);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex3d), (void*)offsetof(vertex3d, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex3d), (void*)offsetof(vertex3d, tex_coord));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex3d), (void*)offsetof(vertex3d, normal));
    glEnableVertexAttribArray(2);

    // shader
    const char* vertex_shader_source = R"(
        #version 450 core
        layout (location = 0) in vec3 position;
        layout (location = 1) in vec2 tex_coord;
        layout (location = 2) in vec3 normal;
        layout (location = 3) in float tex_index;
        out vec2 tex_coord_out;
        out vec3 normal_out;
        out float tex_index_out;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main()
        {
            tex_coord_out = tex_coord;
            normal_out = normal;
            tex_index_out = tex_index;
            gl_Position = projection * view * model * vec4(position, 1.0);
        }
    )";

    const char* fragment_shader_source = R"(
        #version 450 core
        in vec2 tex_coord_out;
        in vec3 normal_out;
        in float tex_index_out;
        out vec4 color;
        uniform sampler2D tex_array[32];
        void main()
        {
            vec3 light_direction = normalize(vec3(0.0, 0.0, 1.0));
            float ambient_strength = 0.3;
            float diffuse_strength = 0.7;
            float specular_strength = 1.0;
            float shininess = 32.0;
            vec3 ambient = ambient_strength * vec3(1.0, 1.0, 1.0);
            vec3 diffuse = diffuse_strength * vec3(1.0, 1.0, 1.0);
            vec3 specular = specular_strength * vec3(1.0, 1.0, 1.0);
            float light_dot_normal = max(dot(light_direction, normalize(normal_out)), 0.0);
            vec3 diffuse_contribution = diffuse * light_dot_normal;
            vec3 reflection_contribution = specular * pow(max(dot(reflect(-light_direction, normalize(normal_out)), normalize(normal_out)), 0.0), shininess);
            vec3 total_light = (ambient + diffuse_contribution + reflection_contribution) * vec3(1.0, 1.0, 1.0);
            // perform texture sampling
            vec4 tex_color = texture(tex_array[int(tex_index_out)], tex_coord_out);
            color = vec4(tex_color.rgb * total_light, tex_color.a);
        }
    )";



    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}