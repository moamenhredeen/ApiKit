#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#include "http_client.h"
#include "http_parser.h"

// Forward declarations
void save_data();
void load_data();

// History item
typedef struct {
    char method[10];
    char url[512];
    long status_code;
    char timestamp[64];
} history_item_t;

// Collection item
typedef struct {
    char name[128];
    char method[10];
    char url[512];
    char headers[1024];
    char body[2048];
} collection_item_t;

// GUI State
typedef struct {
    char url[512];
    char headers[1024];
    char body[2048];
    char response[8192];
    int method_selected;
    int request_in_progress;
    long last_status_code;
    
    // Side panel state
    int show_sidebar;
    char search_text[256];
    int active_tab; // 0 = history, 1 = collections
    
    // History
    history_item_t history[100];
    int history_count;
    
    // Collections
    collection_item_t collections[50];
    int collection_count;
} gui_state_t;

static gui_state_t gui_state = {
    .url = "https://httpbin.org/get",
    .headers = "Content-Type: application/json\nAuthorization: Bearer your-token",
    .body = "{\n  \"message\": \"Hello from API Kit!\",\n  \"data\": {\n    \"key\": \"value\"\n  }\n}",
    .response = "Response will appear here...",
    .method_selected = 0,
    .request_in_progress = 0,
    .last_status_code = 0,
    .show_sidebar = 1,
    .search_text = "",
    .active_tab = 0,
    .history_count = 0,
    .collection_count = 0
};

// Helper functions
void add_to_history(const char* method, const char* url, long status_code) {
    if (gui_state.history_count < 100) {
        history_item_t* item = &gui_state.history[gui_state.history_count];
        strncpy(item->method, method, sizeof(item->method) - 1);
        strncpy(item->url, url, sizeof(item->url) - 1);
        item->status_code = status_code;
        
        // Simple timestamp
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        strftime(item->timestamp, sizeof(item->timestamp), "%H:%M:%S", tm_info);
        
        gui_state.history_count++;
        
        // Auto-save after adding to history
        save_data();
    }
}

void add_to_collection(const char* name, const char* method, const char* url, const char* headers, const char* body) {
    if (gui_state.collection_count < 50) {
        collection_item_t* item = &gui_state.collections[gui_state.collection_count];
        strncpy(item->name, name, sizeof(item->name) - 1);
        strncpy(item->method, method, sizeof(item->method) - 1);
        strncpy(item->url, url, sizeof(item->url) - 1);
        strncpy(item->headers, headers, sizeof(item->headers) - 1);
        strncpy(item->body, body, sizeof(item->body) - 1);
        gui_state.collection_count++;
        
        // Auto-save after adding to collection
        save_data();
    }
}

// Save data in HTTP file format for both collections and history
void save_data() {
    // Save history in HTTP format
    http_collection_t history_collection;
    http_collection_clear(&history_collection);
    
    // Convert history to HTTP format
    for (int i = 0; i < gui_state.history_count; i++) {
        http_request_t request;
        history_item_t* hist_item = &gui_state.history[i];
        
        // Create descriptive name with timestamp and status
        snprintf(request.name, sizeof(request.name), "[%s] %s %s - Status: %ld", 
                hist_item->timestamp, hist_item->method, hist_item->url, hist_item->status_code);
        
        strncpy(request.method, hist_item->method, sizeof(request.method) - 1);
        strncpy(request.url, hist_item->url, sizeof(request.url) - 1);
        
        // Add timestamp and status as comment in headers
        snprintf(request.headers, sizeof(request.headers), 
                "# Timestamp: %s\n# Status Code: %ld", 
                hist_item->timestamp, hist_item->status_code);
        
        request.body[0] = '\0'; // No body for history items
        
        // Null terminate strings
        request.name[sizeof(request.name) - 1] = '\0';
        request.method[sizeof(request.method) - 1] = '\0';
        request.url[sizeof(request.url) - 1] = '\0';
        request.headers[sizeof(request.headers) - 1] = '\0';
        request.body[sizeof(request.body) - 1] = '\0';
        
        http_collection_add(&history_collection, &request);
    }
    
    // Save history using parser
    http_save_file("history.http", &history_collection);
    
    // Save collections in HTTP file format using parser
    http_collection_t collection;
    http_collection_clear(&collection);
    
    // Convert from GUI state to parser format
    for (int i = 0; i < gui_state.collection_count; i++) {
        http_request_t request;
        collection_item_t* item = &gui_state.collections[i];
        
        strncpy(request.name, item->name, sizeof(request.name) - 1);
        strncpy(request.method, item->method, sizeof(request.method) - 1);
        strncpy(request.url, item->url, sizeof(request.url) - 1);
        strncpy(request.headers, item->headers, sizeof(request.headers) - 1);
        strncpy(request.body, item->body, sizeof(request.body) - 1);
        
        // Null terminate strings
        request.name[sizeof(request.name) - 1] = '\0';
        request.method[sizeof(request.method) - 1] = '\0';
        request.url[sizeof(request.url) - 1] = '\0';
        request.headers[sizeof(request.headers) - 1] = '\0';
        request.body[sizeof(request.body) - 1] = '\0';
        
        http_collection_add(&collection, &request);
    }
    
    // Save using parser
    http_save_file("apikit_collection.http", &collection);
}

// Load data from files  
void load_data() {
    // Load history from HTTP format
    http_collection_t history_collection;
    if (http_parse_file("history.http", &history_collection) == 0) {
        gui_state.history_count = 0;
        for (int i = 0; i < history_collection.count && i < 100; i++) {
            const http_request_t* request = &history_collection.requests[i];
            history_item_t* hist_item = &gui_state.history[gui_state.history_count];
            
            strncpy(hist_item->method, request->method, sizeof(hist_item->method) - 1);
            strncpy(hist_item->url, request->url, sizeof(hist_item->url) - 1);
            
            // Parse timestamp and status from headers (comments)
            hist_item->status_code = 0;
            hist_item->timestamp[0] = '\0';
            
            // Simple parsing of our comment format
            char headers_copy[1024];
            strncpy(headers_copy, request->headers, sizeof(headers_copy));
            headers_copy[sizeof(headers_copy) - 1] = '\0';
            
            char *line = strtok(headers_copy, "\n");
            while (line) {
                if (strncmp(line, "# Timestamp: ", 13) == 0) {
                    strncpy(hist_item->timestamp, line + 13, sizeof(hist_item->timestamp) - 1);
                    hist_item->timestamp[sizeof(hist_item->timestamp) - 1] = '\0';
                } else if (strncmp(line, "# Status Code: ", 15) == 0) {
                    hist_item->status_code = atol(line + 15);
                }
                line = strtok(NULL, "\n");
            }
            
            // Fallback if parsing failed
            if (hist_item->timestamp[0] == '\0') {
                strncpy(hist_item->timestamp, "00:00:00", sizeof(hist_item->timestamp) - 1);
            }
            
            // Null terminate strings
            hist_item->method[sizeof(hist_item->method) - 1] = '\0';
            hist_item->url[sizeof(hist_item->url) - 1] = '\0';
            hist_item->timestamp[sizeof(hist_item->timestamp) - 1] = '\0';
            
            gui_state.history_count++;
        }
    }
    
    // Load collections from HTTP file using parser
    http_collection_t collection;
    if (http_parse_file("apikit_collection.http", &collection) == 0) {
        gui_state.collection_count = 0;
        for (int i = 0; i < collection.count && i < 50; i++) {
            const http_request_t* request = &collection.requests[i];
            collection_item_t* item = &gui_state.collections[gui_state.collection_count];
            
            strncpy(item->name, request->name, sizeof(item->name) - 1);
            strncpy(item->method, request->method, sizeof(item->method) - 1);
            strncpy(item->url, request->url, sizeof(item->url) - 1);
            strncpy(item->headers, request->headers, sizeof(item->headers) - 1);
            strncpy(item->body, request->body, sizeof(item->body) - 1);
            
            // Null terminate strings
            item->name[sizeof(item->name) - 1] = '\0';
            item->method[sizeof(item->method) - 1] = '\0';
            item->url[sizeof(item->url) - 1] = '\0';
            item->headers[sizeof(item->headers) - 1] = '\0';
            item->body[sizeof(item->body) - 1] = '\0';
            
            gui_state.collection_count++;
        }
    }
}



void draw_sidebar(struct nk_context *ctx, int sidebar_width, int height) {
    if (nk_begin(ctx, "Sidebar", nk_rect(0, 0, sidebar_width, height),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
        
        // Toggle button at top
        nk_layout_row_static(ctx, 30, sidebar_width - 20, 1);
        if (nk_button_label(ctx, "◀")) {
            gui_state.show_sidebar = 0;
        }
        
        // Search field
        nk_layout_row_static(ctx, 25, sidebar_width - 20, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, gui_state.search_text, 
                                       sizeof(gui_state.search_text), nk_filter_default);
        
        // Tabs
        nk_layout_row_static(ctx, 30, (sidebar_width - 20) / 2, 2);
        if (nk_button_label(ctx, gui_state.active_tab == 0 ? "[History]" : "History")) {
            gui_state.active_tab = 0;
        }
        if (nk_button_label(ctx, gui_state.active_tab == 1 ? "[Collections]" : "Collections")) {
            gui_state.active_tab = 1;
        }
        
        // Tab content
        nk_layout_row_dynamic(ctx, height - 120, 1);
        if (nk_group_begin(ctx, "tab_content", NK_WINDOW_BORDER)) {
            if (gui_state.active_tab == 0) {
                // History tab
                for (int i = gui_state.history_count - 1; i >= 0; i--) {
                    history_item_t* item = &gui_state.history[i];
                    
                    // Filter by search
                    if (strlen(gui_state.search_text) > 0 && 
                        !strstr(item->url, gui_state.search_text) && 
                        !strstr(item->method, gui_state.search_text)) {
                        continue;
                    }
                    
                    nk_layout_row_dynamic(ctx, 60, 1);
                    if (nk_group_begin(ctx, item->url, NK_WINDOW_BORDER)) {
                        nk_layout_row_dynamic(ctx, 15, 2);
                        nk_label(ctx, item->method, NK_TEXT_LEFT);
                        
                        char status_text[32];
                        snprintf(status_text, sizeof(status_text), "%ld", item->status_code);
                        struct nk_color color = nk_rgb(0, 255, 0);
                        if (item->status_code >= 400) color = nk_rgb(255, 0, 0);
                        else if (item->status_code >= 300) color = nk_rgb(255, 165, 0);
                        nk_label_colored(ctx, status_text, NK_TEXT_RIGHT, color);
                        
                        nk_layout_row_dynamic(ctx, 15, 1);
                        if (nk_button_label(ctx, item->url)) {
                            // Load this request
                            strncpy(gui_state.url, item->url, sizeof(gui_state.url));
                            // Set method
                            if (strcmp(item->method, "GET") == 0) gui_state.method_selected = 0;
                            else if (strcmp(item->method, "POST") == 0) gui_state.method_selected = 1;
                            else if (strcmp(item->method, "PUT") == 0) gui_state.method_selected = 2;
                            else if (strcmp(item->method, "DELETE") == 0) gui_state.method_selected = 3;
                            else if (strcmp(item->method, "PATCH") == 0) gui_state.method_selected = 4;
                        }
                        nk_group_end(ctx);
                    }
                }
            } else {
                // Collections tab
                nk_layout_row_dynamic(ctx, 30, 1);
                if (nk_button_label(ctx, "+ Save Current Request")) {
                    char name[128];
                    snprintf(name, sizeof(name), "%s %s", 
                             gui_state.method_selected == 0 ? "GET" :
                             gui_state.method_selected == 1 ? "POST" :
                             gui_state.method_selected == 2 ? "PUT" :
                             gui_state.method_selected == 3 ? "DELETE" : "PATCH",
                             gui_state.url);
                    
                    const char* method = gui_state.method_selected == 0 ? "GET" :
                                       gui_state.method_selected == 1 ? "POST" :
                                       gui_state.method_selected == 2 ? "PUT" :
                                       gui_state.method_selected == 3 ? "DELETE" : "PATCH";
                    
                    add_to_collection(name, method, gui_state.url, gui_state.headers, gui_state.body);
                }
                
                for (int i = 0; i < gui_state.collection_count; i++) {
                    collection_item_t* item = &gui_state.collections[i];
                    
                    // Filter by search
                    if (strlen(gui_state.search_text) > 0 && 
                        !strstr(item->name, gui_state.search_text) && 
                        !strstr(item->url, gui_state.search_text)) {
                        continue;
                    }
                    
                    nk_layout_row_dynamic(ctx, 40, 1);
                    if (nk_button_label(ctx, item->name)) {
                        // Load this collection item
                        strncpy(gui_state.url, item->url, sizeof(gui_state.url));
                        strncpy(gui_state.headers, item->headers, sizeof(gui_state.headers));
                        strncpy(gui_state.body, item->body, sizeof(gui_state.body));
                        
                        // Set method
                        if (strcmp(item->method, "GET") == 0) gui_state.method_selected = 0;
                        else if (strcmp(item->method, "POST") == 0) gui_state.method_selected = 1;
                        else if (strcmp(item->method, "PUT") == 0) gui_state.method_selected = 2;
                        else if (strcmp(item->method, "DELETE") == 0) gui_state.method_selected = 3;
                        else if (strcmp(item->method, "PATCH") == 0) gui_state.method_selected = 4;
                    }
                }
            }
            nk_group_end(ctx);
        }
    }
    nk_end(ctx);
}

void draw_postman_gui(struct nk_context *ctx, http_client_t *client) {
    // Get window size and fill the entire window
    int width, height;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
    
    const int sidebar_width = 300;
    int main_area_x = gui_state.show_sidebar ? sidebar_width : 0;
    int main_area_width = width - main_area_x;
    
    // Draw sidebar if visible
    if (gui_state.show_sidebar) {
        draw_sidebar(ctx, sidebar_width, height);
    }
    
    // Toggle button when sidebar is hidden
    if (!gui_state.show_sidebar) {
        if (nk_begin(ctx, "ToggleBtn", nk_rect(5, 5, 40, 30), NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_static(ctx, 25, 35, 1);
            if (nk_button_label(ctx, "▶")) {
                gui_state.show_sidebar = 1;
            }
        }
        nk_end(ctx);
    }
    
    if (nk_begin(ctx, "API Kit", nk_rect(main_area_x, 0, main_area_width, height),
                 NK_WINDOW_NO_SCROLLBAR)) {
        
        // Method dropdown, URL input, and Send button in same row with fixed sizes
        static const char *methods[] = {"GET", "POST", "PUT", "DELETE", "PATCH"};
        nk_layout_row_begin(ctx, NK_STATIC, 40, 3);
        nk_layout_row_push(ctx, 100);  // Fixed 100px for dropdown
        gui_state.method_selected = nk_combo(ctx, methods, 5, gui_state.method_selected, 25, nk_vec2(120, 200));
        nk_layout_row_push(ctx, main_area_width - 180);  // Remaining space minus button and dropdown
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, gui_state.url, 
                                       sizeof(gui_state.url), nk_filter_default);
        nk_layout_row_push(ctx, 80);  // Fixed 80px for send button
        if (nk_button_label(ctx, "SEND") && !gui_state.request_in_progress) {
            gui_state.request_in_progress = 1;
            
            // Convert method selection to enum
            http_method_t method = HTTP_METHOD_GET;
            switch (gui_state.method_selected) {
                case 0: method = HTTP_METHOD_GET; break;
                case 1: method = HTTP_METHOD_POST; break;
                case 2: method = HTTP_METHOD_PUT; break;
                case 3: method = HTTP_METHOD_DELETE; break;
                case 4: method = HTTP_METHOD_PATCH; break;
            }
            
            // Parse headers (simple implementation)
            const char *headers[10] = {NULL}; // Max 10 headers
            int header_count = 0;
            char headers_copy[1024];
            strncpy(headers_copy, gui_state.headers, sizeof(headers_copy));
            
            char *line = strtok(headers_copy, "\n");
            while (line && header_count < 9) {
                if (strlen(line) > 0) {
                    headers[header_count++] = line;
                }
                line = strtok(NULL, "\n");
            }
            
            // Prepare request options
            http_request_options_t options = {
                .headers = headers,
                .timeout_ms = 10000,
                .content_type = "application/json"
            };
            
            // Add body for POST/PUT/PATCH
            if (method != HTTP_METHOD_GET && method != HTTP_METHOD_DELETE) {
                if (strlen(gui_state.body) > 0) {
                    options.body = gui_state.body;
                }
            }
            
            // Make HTTP request
            http_response_t* response = http_request(client, method, gui_state.url, &options);
            
            if (response) {
                gui_state.last_status_code = response->status_code;
                snprintf(gui_state.response, sizeof(gui_state.response),
                         "Status: %ld\n\n--- Headers ---\n%s\n--- Body ---\n%s", 
                         response->status_code,
                         response->headers ? response->headers : "No headers",
                         response->body ? response->body : "No response body");
                
                // Add to history
                const char* method_name = methods[gui_state.method_selected];
                add_to_history(method_name, gui_state.url, response->status_code);
                
                http_response_free(response);
            } else {
                strncpy(gui_state.response, "Request failed!", sizeof(gui_state.response));
                gui_state.last_status_code = 0;
                
                // Add failed request to history
                const char* method_name = methods[gui_state.method_selected];
                add_to_history(method_name, gui_state.url, 0);
            }
            
            gui_state.request_in_progress = 0;
        }
        nk_layout_row_end(ctx);
        
        // Status indicator
        if (gui_state.request_in_progress) {
            nk_layout_row_static(ctx, 20, 200, 1);
            nk_label_colored(ctx, "Sending request...", NK_TEXT_LEFT, nk_rgb(255, 165, 0));
        } else if (gui_state.last_status_code > 0) {
            nk_layout_row_static(ctx, 20, 300, 1);
            char status_text[100];
            snprintf(status_text, sizeof(status_text), "Last Status: %ld", gui_state.last_status_code);
            struct nk_color color = nk_rgb(0, 255, 0); // Green for success
            if (gui_state.last_status_code >= 400) {
                color = nk_rgb(255, 0, 0); // Red for errors
            } else if (gui_state.last_status_code >= 300) {
                color = nk_rgb(255, 165, 0); // Orange for redirects
            }
            nk_label_colored(ctx, status_text, NK_TEXT_LEFT, color);
        }
        
        // Headers section
        nk_layout_row_static(ctx, 20, 100, 1);
        nk_label(ctx, "Headers:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 100, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, gui_state.headers, 
                                       sizeof(gui_state.headers), nk_filter_default);
        
        // Body section (only for POST/PUT/PATCH)
        if (gui_state.method_selected == 1 || gui_state.method_selected == 2 || gui_state.method_selected == 4) {
            nk_layout_row_static(ctx, 20, 100, 1);
            nk_label(ctx, "Request Body:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 120, 1);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, gui_state.body, 
                                           sizeof(gui_state.body), nk_filter_default);
        }
        
        // Response section
        nk_layout_row_static(ctx, 20, 100, 1);
        nk_label(ctx, "Response:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 200, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX | NK_EDIT_READ_ONLY, 
                                       gui_state.response, 
                                       sizeof(gui_state.response), nk_filter_default);
    }
    nk_end(ctx);
}

int main(int argc, char *argv[]) {
    // Load saved data
    load_data();
    
    // Initialize HTTP client
    if (http_client_global_init() != 0) {
        printf("Failed to initialize HTTP client\n");
        return -1;
    }
    
    http_client_t *client = http_client_create();
    if (!client) {
        printf("Failed to create HTTP client\n");
        http_client_global_cleanup();
        return -1;
    }
    
    // Initialize GLFW
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        http_client_destroy(client);
        http_client_global_cleanup();
        return -1;
    }

    // Set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    GLFWwindow *window = glfwCreateWindow(1000, 750, "API Kit - HTTP Client", NULL, NULL);
    if (!window) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        http_client_destroy(client);
        http_client_global_cleanup();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize Nuklear
    static struct nk_glfw glfw = {0};
    struct nk_context* ctx = nk_glfw3_init(&glfw, window, NK_GLFW3_INSTALL_CALLBACKS);
    if (!ctx) {
        printf("Failed to initialize Nuklear\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        http_client_destroy(client);
        http_client_global_cleanup();
        return -1;
    }

    // Load Fonts
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&glfw, &atlas);
    nk_glfw3_font_stash_end(&glfw);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        nk_glfw3_new_frame(&glfw);
        
        // Draw GUI
        draw_postman_gui(ctx, client);
        
        // Render
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.10f, 0.18f, 0.24f, 1.0f);
        
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, 512 * 1024, 128 * 1024);
        glfwSwapBuffers(window);
    }

    // Save data before exit
    save_data();
    
    // Cleanup
    nk_glfw3_shutdown(&glfw);
    glfwDestroyWindow(window);
    glfwTerminate();
    http_client_destroy(client);
    http_client_global_cleanup();
    return 0;
}