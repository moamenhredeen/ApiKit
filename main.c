#include <stdio.h>
#include <GLFW/glfw3.h>

#include "http_client.h"

int main(int argc, char *argv[]) {

    http_client_t *client = http_client_create();

    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    GLFWwindow *window = glfwCreateWindow(800, 600, "Hello World", NULL, NULL);
    if (!window) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    http_client_destroy(client);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}