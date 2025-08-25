/* ============================================================================
 * API Kit - HTTP Client GUI Application
 * 
 * A modern HTTP client with GUI interface built using Nuklear and OpenGL.
 * Features workspaces, collections, request history, and drag-and-drop support.
 * ============================================================================ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#include "http_client.h"
#include "http_parser.h"
#include "toml.h"

/* ============================================================================
 * CONSTANTS AND CONFIGURATION
 * ============================================================================ */

#define MAX_WORKSPACES 5
#define MAX_COLLECTIONS_PER_WORKSPACE 10
#define MAX_REQUESTS_PER_COLLECTION 20
#define MAX_HISTORY_ITEMS 100
#define SIDEBAR_WIDTH 300

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

// HTTP request history item
typedef struct {
    char method[10];
    char url[512];
    long status_code;
    char timestamp[64];
} history_item_t;

// Individual HTTP request
typedef struct {
    char name[128];
    char method[10];
    char url[512];
    char headers[1024];
    char body[2048];
} request_item_t;

// collection_t of requests (tree node in GUI)
typedef struct {
    char name[128];
    request_item_t requests[MAX_REQUESTS_PER_COLLECTION];
    int request_count;
    int expanded;
} collection_t;

// workspace_t containing collections
typedef struct {
    char name[128];
    char filename[256];
    collection_t collections[MAX_COLLECTIONS_PER_WORKSPACE];
    int collection_count;
} workspace_t;

// Drag and drop state
typedef struct {
    int active;
    int workspace_index;
    int collection_index;
    int request_index;
    char preview[256];
} drag_state_t;

// selection_t state for operations
typedef struct {
    int type;  // 0 = none, 1 = collection, 2 = request
    int workspace_index;
    int collection_index;
    int request_index;
} selection_t;

// Keyboard shortcut state
typedef struct {
    int prev_ctrl_b_combo;
    int prev_ctrl_f_combo;
} keyboard_state_t;

// Application settings
typedef struct {
    char data_folder_path[512];
    int theme_selected;
    int keybindings_enabled;
    int ctrl_b_enabled;
    int ctrl_f_enabled;
    int delete_key_enabled;
} settings_t;

// Main application state
typedef struct {
    // Current request data
    char url[512];
    char headers[1024];
    char body[2048];
    char response[8192];
    int method_selected;
    int request_in_progress;
    long last_status_code;
    
    // UI state
    int show_sidebar;
    int show_settings_page;
    char search_text[256];
    int active_tab;  // 0 = history, 1 = collections
    int search_has_focus;
    int url_has_focus;
    
    // Data
    history_item_t history[MAX_HISTORY_ITEMS];
    int history_count;
    workspace_t workspaces[MAX_WORKSPACES];
    int workspace_count;
    int active_workspace;
    
    // UI dialogs
    char new_workspace_name[128];
    char new_collection_name[128];
    int show_new_workspace_popup;
    int show_new_collection_popup;
    
    // Interactive state
    drag_state_t drag;
    selection_t selection;
    keyboard_state_t keyboard;
    settings_t settings;
} app_state_t;

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static app_state_t app_state = {
    .url = "https://httpbin.org/get",
    .headers = "Content-Type: application/json\nAuthorization: Bearer your-token",
    .body = "{\n  \"message\": \"Hello from API Kit!\",\n  \"data\": {\n    \"key\": \"value\"\n  }\n}",
    .response = "Response will appear here...",
    .method_selected = 0,
    .request_in_progress = 0,
    .last_status_code = 0,
    .show_sidebar = 1,
    .show_settings_page = 0,
    .search_text = "",
    .active_tab = 0,
    .search_has_focus = 0,
    .url_has_focus = 0,
    .history_count = 0,
    .workspace_count = 0,
    .active_workspace = 0,
    .new_workspace_name = "",
    .new_collection_name = "",
    .show_new_workspace_popup = 0,
    .show_new_collection_popup = 0,
    .drag = {
        .active = 0,
        .workspace_index = -1,
        .collection_index = -1,
        .request_index = -1,
        .preview = ""
    },
    .selection = {
        .type = 0,
        .workspace_index = -1,
        .collection_index = -1,
        .request_index = -1
    },
    .keyboard = {
        .prev_ctrl_b_combo = 0,
        .prev_ctrl_f_combo = 0
    },
    .settings = {
        .data_folder_path = "data",
        .theme_selected = 0,
        .keybindings_enabled = 1,
        .ctrl_b_enabled = 1,
        .ctrl_f_enabled = 1,
        .delete_key_enabled = 1
    }
};

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

// Core data management
static void save_data(void);
static void load_data(void);
static void ensure_data_directory(void);
static void save_settings(void);
static void load_settings(void);

// workspace_t management
static void ensure_default_workspace(void);
static void scan_and_load_workspaces(void);
static void extract_workspace_name(const char* filename, char* workspace_name, size_t max_len);
static void load_workspace_from_file(const char* filename);

// Request and collection operations
static void add_to_history(const char* method, const char* url, long status_code);
static void add_to_collection(const char* collection_name, const char* request_name, 
                            const char* method, const char* url, const char* headers, const char* body);
static void move_request_to_collection(int src_workspace, int src_collection, int src_request, 
                                     int dest_workspace, int dest_collection);
static void delete_selected_item(void);

// Input handling
static void handle_keyboard_shortcuts(struct nk_context *ctx);

// UI Components
static void ui_sidebar(struct nk_context *ctx, int width, int height);
static void ui_main_panel(struct nk_context *ctx, http_client_t *client, int x, int width, int height);
static void ui_settings_page(struct nk_context *ctx, int x, int width, int height);
static void ui_history_tab(struct nk_context *ctx);
static void ui_collections_tab(struct nk_context *ctx);
static void ui_workspace_dropdown(struct nk_context *ctx);
static void ui_collection_tree(struct nk_context *ctx);
static void ui_drag_preview(struct nk_context *ctx);

// Theme and styling
static void apply_theme(void);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static void add_to_history(const char* method, const char* url, long status_code) {
    if (app_state.history_count < MAX_HISTORY_ITEMS) {
        history_item_t* item = &app_state.history[app_state.history_count];
        strncpy(item->method, method, sizeof(item->method) - 1);
        item->method[sizeof(item->method) - 1] = '\0';
        strncpy(item->url, url, sizeof(item->url) - 1);
        item->url[sizeof(item->url) - 1] = '\0';
        item->status_code = status_code;
        
        // Generate timestamp
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        strftime(item->timestamp, sizeof(item->timestamp), "%H:%M:%S", tm_info);
        
        app_state.history_count++;
        save_data();
    }
}

static void ensure_data_directory(void) {
    struct stat st = {0};
    if (stat(app_state.settings.data_folder_path, &st) == -1) {
        mkdir(app_state.settings.data_folder_path, 0755);
    }
}

static void extract_workspace_name(const char* filename, char* workspace_name, size_t max_len) {
    // Copy filename and remove .http extension, convert underscores to spaces
    strncpy(workspace_name, filename, max_len - 1);
    workspace_name[max_len - 1] = '\0';
    
    // Remove .http extension
    char* ext = strstr(workspace_name, ".http");
    if (ext) {
        *ext = '\0';
    }
    
    // Convert underscores to spaces and capitalize first letter
    for (int i = 0; workspace_name[i]; i++) {
        if (workspace_name[i] == '_') {
            workspace_name[i] = ' ';
        }
        if (i == 0 && workspace_name[i] >= 'a' && workspace_name[i] <= 'z') {
            workspace_name[i] -= 32; // Capitalize first letter
        }
    }
}

static void load_workspace_from_file(const char* filename) {
    if (app_state.workspace_count >= MAX_WORKSPACES) return;
    
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", app_state.settings.data_folder_path, filename);
    
    workspace_t* workspace = &app_state.workspaces[app_state.workspace_count];
    
    // Extract workspace name from filename
    extract_workspace_name(filename, workspace->name, sizeof(workspace->name));
    strncpy(workspace->filename, full_path, sizeof(workspace->filename) - 1);
    workspace->filename[sizeof(workspace->filename) - 1] = '\0';
    workspace->collection_count = 0;
    
    // Load the workspace content
    http_collection_t workspace_collection;
    if (http_parse_file(full_path, &workspace_collection) == 0) {
        // Parse requests and group them by collection name from the [collection_t] prefix
        for (int i = 0; i < workspace_collection.count; i++) {
            const http_request_t* request = &workspace_collection.requests[i];
            
            // Extract collection name from request name format "[collection_t] Request"
            char collection_name[128] = "Default collection_t";
            char request_name[128];
            
            if (request->name[0] == '[') {
                char* end_bracket = strchr(request->name, ']');
                if (end_bracket) {
                    size_t len = end_bracket - request->name - 1;
                    if (len < sizeof(collection_name) - 1) {
                        strncpy(collection_name, request->name + 1, len);
                        collection_name[len] = '\0';
                    }
                    strncpy(request_name, end_bracket + 2, sizeof(request_name) - 1); // Skip "] "
                } else {
                    strncpy(request_name, request->name, sizeof(request_name) - 1);
                }
            } else {
                strncpy(request_name, request->name, sizeof(request_name) - 1);
            }
            
            // Find or create collection
            collection_t* target_collection = NULL;
            for (int c = 0; c < workspace->collection_count; c++) {
                if (strcmp(workspace->collections[c].name, collection_name) == 0) {
                    target_collection = &workspace->collections[c];
                    break;
                }
            }
            
            if (!target_collection && workspace->collection_count < MAX_COLLECTIONS_PER_WORKSPACE) {
                target_collection = &workspace->collections[workspace->collection_count];
                strncpy(target_collection->name, collection_name, sizeof(target_collection->name) - 1);
                target_collection->name[sizeof(target_collection->name) - 1] = '\0';
                target_collection->request_count = 0;
                target_collection->expanded = 1;
                workspace->collection_count++;
            }
            
            // Add request to collection
            if (target_collection && target_collection->request_count < MAX_REQUESTS_PER_COLLECTION) {
                request_item_t* item = &target_collection->requests[target_collection->request_count];
                strncpy(item->name, request_name, sizeof(item->name) - 1);
                strncpy(item->method, request->method, sizeof(item->method) - 1);
                strncpy(item->url, request->url, sizeof(item->url) - 1);
                strncpy(item->headers, request->headers, sizeof(item->headers) - 1);
                strncpy(item->body, request->body, sizeof(item->body) - 1);
                
                // Ensure null termination
                item->name[sizeof(item->name) - 1] = '\0';
                item->method[sizeof(item->method) - 1] = '\0';
                item->url[sizeof(item->url) - 1] = '\0';
                item->headers[sizeof(item->headers) - 1] = '\0';
                item->body[sizeof(item->body) - 1] = '\0';
                
                target_collection->request_count++;
            }
        }
    }
    
    app_state.workspace_count++;
}

static void scan_and_load_workspaces(void) {
    ensure_data_directory();
    
    DIR *dir = opendir(app_state.settings.data_folder_path);
    if (dir == NULL) {
        return;
    }
    
    struct dirent *entry;
    app_state.workspace_count = 0; // Reset workspace count
    
    while ((entry = readdir(dir)) != NULL && app_state.workspace_count < MAX_WORKSPACES) {
        // Skip . and .. entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Skip history.http as it's not a workspace
        if (strcmp(entry->d_name, "history.http") == 0) {
            continue;
        }
        
        // Only process .http files
        if (strstr(entry->d_name, ".http") != NULL) {
            load_workspace_from_file(entry->d_name);
        }
    }
    
    closedir(dir);
    
    // Ensure we have at least a default workspace
    if (app_state.workspace_count == 0) {
        ensure_default_workspace();
    }
    
    // Set active workspace to first one
    app_state.active_workspace = 0;
}

static void ensure_default_workspace(void) {
    if (app_state.workspace_count == 0) {
        workspace_t* workspace = &app_state.workspaces[0];
        strncpy(workspace->name, "Default", sizeof(workspace->name) - 1);
        workspace->name[sizeof(workspace->name) - 1] = '\0';
        snprintf(workspace->filename, sizeof(workspace->filename), "%s/default.http", app_state.settings.data_folder_path);
        workspace->collection_count = 0;
        app_state.workspace_count = 1;
        app_state.active_workspace = 0;
    }
}

/* ============================================================================
 * REQUEST AND COLLECTION OPERATIONS
 * ============================================================================ */

static void move_request_to_collection(int src_workspace, int src_collection, int src_request, 
                                     int dest_workspace, int dest_collection) {
    if (src_workspace < 0 || src_workspace >= app_state.workspace_count ||
        dest_workspace < 0 || dest_workspace >= app_state.workspace_count) {
        return;
    }
    
    workspace_t* src_ws = &app_state.workspaces[src_workspace];
    workspace_t* dest_ws = &app_state.workspaces[dest_workspace];
    
    if (src_collection < 0 || src_collection >= src_ws->collection_count ||
        dest_collection < 0 || dest_collection >= dest_ws->collection_count) {
        return;
    }
    
    collection_t* src_col = &src_ws->collections[src_collection];
    collection_t* dest_col = &dest_ws->collections[dest_collection];
    
    if (src_request < 0 || src_request >= src_col->request_count ||
        dest_col->request_count >= MAX_REQUESTS_PER_COLLECTION) {
        return;
    }
    
    // Copy the request to destination
    request_item_t* request = &src_col->requests[src_request];
    request_item_t* dest_item = &dest_col->requests[dest_col->request_count];
    *dest_item = *request;
    dest_col->request_count++;
    
    // Remove from source by shifting remaining requests
    for (int i = src_request; i < src_col->request_count - 1; i++) {
        src_col->requests[i] = src_col->requests[i + 1];
    }
    src_col->request_count--;
    
    // Save the changes
    save_data();
}

static void delete_selected_item(void) {
    if (app_state.selection.type == 0) return; // Nothing selected
    
    if (app_state.selection.workspace_index < 0 || 
        app_state.selection.workspace_index >= app_state.workspace_count) return;
    
    workspace_t* workspace = &app_state.workspaces[app_state.selection.workspace_index];
    
    if (app_state.selection.type == 1) { // Delete collection
        if (app_state.selection.collection_index < 0 || 
            app_state.selection.collection_index >= workspace->collection_count) return;
        
        // Shift remaining collections
        for (int i = app_state.selection.collection_index; i < workspace->collection_count - 1; i++) {
            workspace->collections[i] = workspace->collections[i + 1];
        }
        workspace->collection_count--;
        
    } else if (app_state.selection.type == 2) { // Delete request
        if (app_state.selection.collection_index < 0 || 
            app_state.selection.collection_index >= workspace->collection_count) return;
        
        collection_t* collection = &workspace->collections[app_state.selection.collection_index];
        
        if (app_state.selection.request_index < 0 || 
            app_state.selection.request_index >= collection->request_count) return;
        
        // Shift remaining requests
        for (int i = app_state.selection.request_index; i < collection->request_count - 1; i++) {
            collection->requests[i] = collection->requests[i + 1];
        }
        collection->request_count--;
    }
    
    // Clear selection
    app_state.selection.type = 0;
    app_state.selection.workspace_index = -1;
    app_state.selection.collection_index = -1;
    app_state.selection.request_index = -1;
    
    save_data();
}

/* ============================================================================
 * SETTINGS MANAGEMENT
 * ============================================================================ */

static void save_settings(void) {
    FILE *file = fopen("config.toml", "w");
    if (file) {
        fprintf(file, "# API Kit Configuration\n\n");
        fprintf(file, "[general]\n");
        fprintf(file, "data_folder = \"%s\"\n", app_state.settings.data_folder_path);
        fprintf(file, "theme = %d\n\n", app_state.settings.theme_selected);
        fprintf(file, "[keybindings]\n");
        fprintf(file, "enabled = %s\n", app_state.settings.keybindings_enabled ? "true" : "false");
        fprintf(file, "ctrl_b_enabled = %s\n", app_state.settings.ctrl_b_enabled ? "true" : "false");
        fprintf(file, "ctrl_f_enabled = %s\n", app_state.settings.ctrl_f_enabled ? "true" : "false");
        fprintf(file, "delete_key_enabled = %s\n", app_state.settings.delete_key_enabled ? "true" : "false");
        fclose(file);
    }
}

// TODO: move settings loading and saving to a seperate file
static void load_settings(void) {
    FILE *file = fopen("config.toml", "r");
    if (!file) return;
    
    char errbuf[200];
    toml_table_t *config = toml_parse_file(file, errbuf, sizeof(errbuf));
    fclose(file);
    
    if (!config) {
        printf("Error parsing config.toml: %s\n", errbuf);
        return;
    }
    
    // Parse [general] section
    toml_table_t *general = toml_table_in(config, "general");
    if (general) {
        toml_datum_t data_folder = toml_string_in(general, "data_folder");
        if (data_folder.ok) {
            strncpy(app_state.settings.data_folder_path, data_folder.u.s, sizeof(app_state.settings.data_folder_path) - 1);
            app_state.settings.data_folder_path[sizeof(app_state.settings.data_folder_path) - 1] = '\0';
            free(data_folder.u.s);
        }
        
        toml_datum_t theme = toml_int_in(general, "theme");
        if (theme.ok) {
            app_state.settings.theme_selected = (int)theme.u.i;
        }
    }
    
    // Parse [keybindings] section
    toml_table_t *keybindings = toml_table_in(config, "keybindings");
    if (keybindings) {
        toml_datum_t enabled = toml_bool_in(keybindings, "enabled");
        if (enabled.ok) {
            app_state.settings.keybindings_enabled = enabled.u.b;
        }
        
        toml_datum_t ctrl_b = toml_bool_in(keybindings, "ctrl_b_enabled");
        if (ctrl_b.ok) {
            app_state.settings.ctrl_b_enabled = ctrl_b.u.b;
        }
        
        toml_datum_t ctrl_f = toml_bool_in(keybindings, "ctrl_f_enabled");
        if (ctrl_f.ok) {
            app_state.settings.ctrl_f_enabled = ctrl_f.u.b;
        }
        
        toml_datum_t delete_key = toml_bool_in(keybindings, "delete_key_enabled");
        if (delete_key.ok) {
            app_state.settings.delete_key_enabled = delete_key.u.b;
        }
    }
    
    toml_free(config);
}

static void apply_theme(void) {
    // Theme application would go here
    // For now, we just store the preference
}

/* ============================================================================
 * INPUT HANDLING
 * ============================================================================ */

static void handle_keyboard_shortcuts(struct nk_context *ctx) {
    if (!app_state.settings.keybindings_enabled) return;
    
    struct nk_input *input = &ctx->input;
    
    // Delete key: Delete selected item
    if (app_state.settings.delete_key_enabled && nk_input_is_key_pressed(input, NK_KEY_DEL)) {
        delete_selected_item();
    }
    
    // Use GLFW for reliable key detection since Nuklear's text input is inconsistent
    GLFWwindow* window = glfwGetCurrentContext();
    if (window) {
        // Check modifier keys using GLFW
        int ctrl_left = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        int ctrl_right = glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        int ctrl_pressed = ctrl_left || ctrl_right;
        
        // Check letter keys
        int b_pressed = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        int f_pressed = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        
        // Ctrl+B combination
        if (app_state.settings.ctrl_b_enabled) {
            int ctrl_b_combo = ctrl_pressed && b_pressed;
            if (ctrl_b_combo && !app_state.keyboard.prev_ctrl_b_combo) {
                app_state.show_sidebar = !app_state.show_sidebar;
            }
            app_state.keyboard.prev_ctrl_b_combo = ctrl_b_combo;
        }
        
        // Ctrl+F combination  
        if (app_state.settings.ctrl_f_enabled) {
            int ctrl_f_combo = ctrl_pressed && f_pressed;
            if (ctrl_f_combo && !app_state.keyboard.prev_ctrl_f_combo) {
                app_state.show_sidebar = 1;
                app_state.active_tab = 1; // Switch to collections tab
                app_state.search_has_focus = 1;
            }
            app_state.keyboard.prev_ctrl_f_combo = ctrl_f_combo;
        }
    }
    
    // Ctrl+A, Ctrl+V, Ctrl+C are handled by Nuklear automatically for text inputs
}

static void add_to_collection(const char* collection_name, const char* request_name, 
                            const char* method, const char* url, const char* headers, const char* body) {
    ensure_default_workspace();
    
    workspace_t* workspace = &app_state.workspaces[app_state.active_workspace];
    
    // Find or create collection
    collection_t* target_collection = NULL;
    for (int i = 0; i < workspace->collection_count; i++) {
        if (strcmp(workspace->collections[i].name, collection_name) == 0) {
            target_collection = &workspace->collections[i];
            break;
        }
    }
    
    // Create new collection if not found
    if (!target_collection && workspace->collection_count < MAX_COLLECTIONS_PER_WORKSPACE) {
        target_collection = &workspace->collections[workspace->collection_count];
        strncpy(target_collection->name, collection_name, sizeof(target_collection->name) - 1);
        target_collection->name[sizeof(target_collection->name) - 1] = '\0';
        target_collection->request_count = 0;
        target_collection->expanded = 1;
        workspace->collection_count++;
    }
    
    // Add request to collection
    if (target_collection && target_collection->request_count < MAX_REQUESTS_PER_COLLECTION) {
        request_item_t* item = &target_collection->requests[target_collection->request_count];
        strncpy(item->name, request_name, sizeof(item->name) - 1);
        strncpy(item->method, method, sizeof(item->method) - 1);
        strncpy(item->url, url, sizeof(item->url) - 1);
        strncpy(item->headers, headers, sizeof(item->headers) - 1);
        strncpy(item->body, body, sizeof(item->body) - 1);
        
        // Ensure null termination
        item->name[sizeof(item->name) - 1] = '\0';
        item->method[sizeof(item->method) - 1] = '\0';
        item->url[sizeof(item->url) - 1] = '\0';
        item->headers[sizeof(item->headers) - 1] = '\0';
        item->body[sizeof(item->body) - 1] = '\0';
        
        target_collection->request_count++;
        save_data();
    }
}

/* ============================================================================
 * DATA PERSISTENCE
 * ============================================================================ */

static void save_data(void) {
    ensure_data_directory();
    
    // Save history in HTTP format
    http_collection_t history_collection;
    http_collection_clear(&history_collection);
    
    // Convert history to HTTP format
    for (int i = 0; i < app_state.history_count; i++) {
        http_request_t request;
        history_item_t* hist_item = &app_state.history[i];
        
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
    char history_path[512];
    snprintf(history_path, sizeof(history_path), "%s/history.http", app_state.settings.data_folder_path);
    http_save_file(history_path, &history_collection);
    
    // Save each workspace to its own file
    for (int w = 0; w < app_state.workspace_count; w++) {
        workspace_t* workspace = &app_state.workspaces[w];
        http_collection_t workspace_collection;
        http_collection_clear(&workspace_collection);

        // Convert all collections in this workspace to parser format
        for (int c = 0; c < workspace->collection_count; c++) {
            collection_t* collection = &workspace->collections[c];
            
            // Add all requests from this collection
            for (int r = 0; r < collection->request_count; r++) {
                request_item_t* item = &collection->requests[r];
                http_request_t request;
        
                // Create request name with collection prefix
                snprintf(request.name, sizeof(request.name), "[%s] %s", 
                        collection->name, item->name);
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
                
                http_collection_add(&workspace_collection, &request);
            }
        }

        // Save workspace to its file
        http_save_file(workspace->filename, &workspace_collection);
    }
}

// TODO: move data handling to a seperate file (same file as settings persisting file ?)
static void load_data(void) {
    ensure_data_directory();
    
    // Load history from HTTP format
    http_collection_t history_collection;
    char history_path[512];
    snprintf(history_path, sizeof(history_path), "%s/history.http", app_state.settings.data_folder_path);
    if (http_parse_file(history_path, &history_collection) == 0) {
        app_state.history_count = 0;
        for (int i = 0; i < history_collection.count && i < MAX_HISTORY_ITEMS; i++) {
            const http_request_t* request = &history_collection.requests[i];
            history_item_t* hist_item = &app_state.history[app_state.history_count];
            
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
            
            app_state.history_count++;
        }
    }
    
    // Scan and load all workspaces from data directory
    scan_and_load_workspaces();
}



/* ============================================================================
 * UI COMPONENTS (React-like structure)
 * ============================================================================ */

static void ui_sidebar(struct nk_context *ctx, int width, int height) {
    if (nk_begin(ctx, "Sidebar", nk_rect(0, 0, width, height),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
        
        // Toggle button at top
        nk_layout_row_static(ctx, 30, width - 20, 1);
        if (nk_button_label(ctx, "◀")) {
            app_state.show_sidebar = 0;
        }
        
        // Search field
        nk_layout_row_static(ctx, 25, width - 20, 1);
        nk_flags edit_flags = NK_EDIT_FIELD;
        if (app_state.search_has_focus) {
            edit_flags |= NK_EDIT_AUTO_SELECT;
            app_state.search_has_focus = 0; // Reset after one frame
        }
        nk_edit_string_zero_terminated(ctx, edit_flags, app_state.search_text, 
                                       sizeof(app_state.search_text), nk_filter_default);
        
        // Tabs
        nk_layout_row_static(ctx, 30, (width - 20) / 2, 2);
        if (nk_button_label(ctx, app_state.active_tab == 0 ? "[History]" : "History")) {
            app_state.active_tab = 0;
        }
        if (nk_button_label(ctx, app_state.active_tab == 1 ? "[collection_ts]" : "collection_ts")) {
            app_state.active_tab = 1;
        }
        
        // Tab content (reduced height to make room for settings button and help)
        nk_layout_row_dynamic(ctx, height - 175, 1);
        if (nk_group_begin(ctx, "tab_content", NK_WINDOW_BORDER)) {
            if (app_state.active_tab == 0) {
                ui_history_tab(ctx);
            } else if (app_state.active_tab == 1) {
                ui_collections_tab(ctx);
            }
            nk_group_end(ctx);
        }
    }
    
    // Handle drag cancellation
    if (app_state.drag.active && nk_input_is_mouse_released(&ctx->input, NK_BUTTON_LEFT)) {
        app_state.drag.active = 0;
    }
    
    // Draw drag preview
    ui_drag_preview(ctx);
    
    // settings_t button at bottom
    nk_layout_row_dynamic(ctx, 35, 1);
    if (nk_button_label(ctx, "⚙ settings_t")) {
        app_state.show_settings_page = !app_state.show_settings_page;
    }
    
    // Show keyboard shortcuts help at bottom
    nk_layout_row_dynamic(ctx, 15, 1);
    nk_label(ctx, "Keys: Ctrl+B=Toggle Sidebar | Ctrl+F=Search | Del=Delete | Right-click=Select", NK_TEXT_CENTERED);
    
    nk_end(ctx);
}

static void ui_history_tab(struct nk_context *ctx) {
    for (int i = app_state.history_count - 1; i >= 0; i--) {
        history_item_t* item = &app_state.history[i];
        
        // Filter by search
        if (strlen(app_state.search_text) > 0 && 
            !strstr(item->url, app_state.search_text) && 
            !strstr(item->method, app_state.search_text)) {
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
                strncpy(app_state.url, item->url, sizeof(app_state.url));
                app_state.url[sizeof(app_state.url) - 1] = '\0';
                
                // Set method
                if (strcmp(item->method, "GET") == 0) app_state.method_selected = 0;
                else if (strcmp(item->method, "POST") == 0) app_state.method_selected = 1;
                else if (strcmp(item->method, "PUT") == 0) app_state.method_selected = 2;
                else if (strcmp(item->method, "DELETE") == 0) app_state.method_selected = 3;
                else if (strcmp(item->method, "PATCH") == 0) app_state.method_selected = 4;
            }
            nk_group_end(ctx);
        }
    }
}

static void ui_collections_tab(struct nk_context *ctx) {
    // Save current request button
    nk_layout_row_dynamic(ctx, 30, 1);
    if (nk_button_label(ctx, "+ Save Current Request")) {
        char name[128];
        snprintf(name, sizeof(name), "%s %s", 
                 app_state.method_selected == 0 ? "GET" :
                 app_state.method_selected == 1 ? "POST" :
                 app_state.method_selected == 2 ? "PUT" :
                 app_state.method_selected == 3 ? "DELETE" : "PATCH",
                 app_state.url);
        
        const char* method = app_state.method_selected == 0 ? "GET" :
                           app_state.method_selected == 1 ? "POST" :
                           app_state.method_selected == 2 ? "PUT" :
                           app_state.method_selected == 3 ? "DELETE" : "PATCH";
        
        add_to_collection("Default collection_t", name, method, app_state.url, app_state.headers, app_state.body);
    }
    
    // workspace_t dropdown
    ui_workspace_dropdown(ctx);
    
    // collection_t tree
    ui_collection_tree(ctx);
}

static void ui_workspace_dropdown(struct nk_context *ctx) {
    // Display workspace dropdown with add button
    if (app_state.workspace_count > 0) {
        workspace_t* current_workspace = &app_state.workspaces[app_state.active_workspace];
                    
                    nk_layout_row_begin(ctx, NK_DYNAMIC, 25, 2);
                    nk_layout_row_push(ctx, 0.8f);
                    if (nk_combo_begin_label(ctx, current_workspace->name, nk_vec2(nk_widget_width(ctx), 200))) {
                        for (int w = 0; w < app_state.workspace_count; w++) {
                            if (nk_combo_item_label(ctx, app_state.workspaces[w].name, NK_TEXT_LEFT)) {
                                app_state.active_workspace = w;
                            }
                        }
                        nk_combo_end(ctx);
                    }
                    nk_layout_row_push(ctx, 0.2f);
                    if (nk_button_label(ctx, "+W")) {
                        app_state.show_new_workspace_popup = 1;
                        strcpy(app_state.new_workspace_name, "");
                    }
                    nk_layout_row_end(ctx);
                } else {
                    // No workspaces yet, show create button
                    nk_layout_row_dynamic(ctx, 25, 1);
                    if (nk_button_label(ctx, "Create workspace_t")) {
                        app_state.show_new_workspace_popup = 1;
                        strcpy(app_state.new_workspace_name, "");
                    }
                }
}

static void ui_collection_tree(struct nk_context *ctx) {
    // collection_t tree would be implemented here
    // For now, just a placeholder
    nk_layout_row_dynamic(ctx, 20, 1);
    nk_label(ctx, "collection_t tree placeholder...", NK_TEXT_LEFT);
}

static void ui_drag_preview(struct nk_context *ctx) {
    if (app_state.drag.active) {
        struct nk_vec2 mouse_pos = ctx->input.mouse.pos;
        nk_layout_space_begin(ctx, NK_STATIC, 20, 1);
        nk_layout_space_push(ctx, nk_rect(mouse_pos.x + 10, mouse_pos.y - 10, 200, 20));
        nk_label_colored(ctx, app_state.drag.preview, NK_TEXT_LEFT, nk_rgb(255, 255, 0));
        nk_layout_space_end(ctx);
    }
}

static void ui_settings_page(struct nk_context *ctx, int x, int width, int height) {
    if (nk_begin(ctx, "settings_t", nk_rect(x, 0, width, height), NK_WINDOW_NO_SCROLLBAR)) {
        nk_layout_row_dynamic(ctx, 40, 1);
        nk_label(ctx, "settings_t", NK_TEXT_CENTERED);
        
        nk_layout_row_dynamic(ctx, 10, 1); // Spacer
        
        // Data folder path section
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, "Data Folder Configuration", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Data Folder Path:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, app_state.settings.data_folder_path, 
                                     sizeof(app_state.settings.data_folder_path), nk_filter_default);
        
        nk_layout_row_dynamic(ctx, 20, 1); // Spacer
        
        // Theme section
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, "Appearance", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Theme:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 30, 1);
        static const char *themes[] = {"Default", "Dark", "Light"};
        app_state.settings.theme_selected = nk_combo(ctx, themes, 3, app_state.settings.theme_selected, 30, nk_vec2(200, 120));
        
        nk_layout_row_dynamic(ctx, 20, 1); // Spacer
        
        // Keyboard shortcuts section
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, "Keyboard Shortcuts", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(ctx, 30, 1);
        if (nk_checkbox_label(ctx, "Enable Keyboard Shortcuts", &app_state.settings.keybindings_enabled)) {
            save_settings();
        }
        
        if (app_state.settings.keybindings_enabled) {
            nk_layout_row_dynamic(ctx, 5, 1); // Small spacer
            
            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_checkbox_label(ctx, "Ctrl+B (Toggle Sidebar)", &app_state.settings.ctrl_b_enabled)) {
                save_settings();
            }
            
            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_checkbox_label(ctx, "Ctrl+F (Focus Search)", &app_state.settings.ctrl_f_enabled)) {
                save_settings();
            }
            
            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_checkbox_label(ctx, "Delete Key (Remove Items)", &app_state.settings.delete_key_enabled)) {
                save_settings();
            }
        }
        
        nk_layout_row_dynamic(ctx, 30, 1); // Spacer
        
        // Action buttons
        nk_layout_row_dynamic(ctx, 40, 3);
        if (nk_button_label(ctx, "Apply Theme")) {
            apply_theme();
            save_settings();
        }
        if (nk_button_label(ctx, "Save settings_t")) {
            save_settings();
        }
        if (nk_button_label(ctx, "Close settings_t")) {
            app_state.show_settings_page = 0;
        }
    }
    nk_end(ctx);
}

static void ui_main_panel(struct nk_context *ctx, http_client_t *client, int x, int width, int height) {
    if (nk_begin(ctx, "API Kit", nk_rect(x, 0, width, height), NK_WINDOW_NO_SCROLLBAR)) {
        // Method dropdown, URL input, and Send button in same row with fixed sizes
        static const char *methods[] = {"GET", "POST", "PUT", "DELETE", "PATCH"};
        nk_layout_row_begin(ctx, NK_STATIC, 40, 3);
        nk_layout_row_push(ctx, 100);  // Fixed 100px for dropdown
        app_state.method_selected = nk_combo(ctx, methods, 5, app_state.method_selected, 25, nk_vec2(120, 200));
        nk_layout_row_push(ctx, width - 180);  // Remaining space minus button and dropdown
        nk_flags url_flags = NK_EDIT_FIELD;
        if (nk_widget_has_mouse_click_down(ctx, NK_BUTTON_LEFT, nk_true)) {
            app_state.url_has_focus = 1;
        }
        nk_edit_string_zero_terminated(ctx, url_flags, app_state.url, 
                                       sizeof(app_state.url), nk_filter_default);
        nk_layout_row_push(ctx, 80);  // Fixed 80px for send button
        if (nk_button_label(ctx, "SEND") && !app_state.request_in_progress) {
            app_state.request_in_progress = 1;
            
            // Convert method selection to enum
            http_method_t method = HTTP_METHOD_GET;
            switch (app_state.method_selected) {
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
            strncpy(headers_copy, app_state.headers, sizeof(headers_copy));
            
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
                if (strlen(app_state.body) > 0) {
                    options.body = app_state.body;
                }
            }
            
            // Make HTTP request
            http_response_t* response = http_request(client, method, app_state.url, &options);
            
            if (response) {
                app_state.last_status_code = response->status_code;
                snprintf(app_state.response, sizeof(app_state.response),
                         "Status: %ld\n\n--- Headers ---\n%s\n--- Body ---\n%s", 
                         response->status_code,
                         response->headers ? response->headers : "No headers",
                         response->body ? response->body : "No response body");
                
                // Add to history
                const char* method_name = methods[app_state.method_selected];
                add_to_history(method_name, app_state.url, response->status_code);
                
                http_response_free(response);
            } else {
                strncpy(app_state.response, "Request failed!", sizeof(app_state.response));
                app_state.last_status_code = 0;
                
                // Add failed request to history
                const char* method_name = methods[app_state.method_selected];
                add_to_history(method_name, app_state.url, 0);
            }
            
            app_state.request_in_progress = 0;
        }
        nk_layout_row_end(ctx);
        
        // Status indicator
        if (app_state.request_in_progress) {
            nk_layout_row_static(ctx, 20, 200, 1);
            nk_label_colored(ctx, "Sending request...", NK_TEXT_LEFT, nk_rgb(255, 165, 0));
        } else if (app_state.last_status_code > 0) {
            nk_layout_row_static(ctx, 20, 300, 1);
            char status_text[100];
            snprintf(status_text, sizeof(status_text), "Last Status: %ld", app_state.last_status_code);
            struct nk_color color = nk_rgb(0, 255, 0); // Green for success
            if (app_state.last_status_code >= 400) {
                color = nk_rgb(255, 0, 0); // Red for errors
            } else if (app_state.last_status_code >= 300) {
                color = nk_rgb(255, 165, 0); // Orange for redirects
            }
            nk_label_colored(ctx, status_text, NK_TEXT_LEFT, color);
        }
        
        // Headers section
        nk_layout_row_static(ctx, 20, 100, 1);
        nk_label(ctx, "Headers:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 100, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, app_state.headers, 
                                       sizeof(app_state.headers), nk_filter_default);
        
        // Body section (only for POST/PUT/PATCH)
        if (app_state.method_selected == 1 || app_state.method_selected == 2 || app_state.method_selected == 4) {
            nk_layout_row_static(ctx, 20, 100, 1);
            nk_label(ctx, "Request Body:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 120, 1);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, app_state.body, 
                                           sizeof(app_state.body), nk_filter_default);
        }
        
        // Response section
        nk_layout_row_static(ctx, 20, 100, 1);
        nk_label(ctx, "Response:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 200, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX | NK_EDIT_READ_ONLY, 
                                       app_state.response, 
                                       sizeof(app_state.response), nk_filter_default);
    }
    nk_end(ctx);
}

// Root UI function that orchestrates all components
static void draw_ui(struct nk_context *ctx, http_client_t *client) {
    // Handle keyboard shortcuts first
    handle_keyboard_shortcuts(ctx);
    
    // Get window size
    int width, height;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
    
    int main_area_x = app_state.show_sidebar ? SIDEBAR_WIDTH : 0;
    int main_area_width = width - main_area_x;
    
    // Draw sidebar if visible
    if (app_state.show_sidebar) {
        ui_sidebar(ctx, SIDEBAR_WIDTH, height);
    }
    
    // Toggle button when sidebar is hidden
    if (!app_state.show_sidebar) {
        if (nk_begin(ctx, "ToggleBtn", nk_rect(5, 5, 40, 30), NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_static(ctx, 25, 35, 1);
            if (nk_button_label(ctx, "▶")) {
                app_state.show_sidebar = 1;
            }
        }
        nk_end(ctx);
    }
    
    // Show settings page or main API interface
    if (app_state.show_settings_page) {
        ui_settings_page(ctx, main_area_x, main_area_width, height);
    } else {
        ui_main_panel(ctx, client, main_area_x, main_area_width, height);
    }
}

/* ============================================================================
 * MAIN FUNCTION AND INITIALIZATION
 * ============================================================================ */

int main(int argc, char *argv[]) {
    // Load settings first
    load_settings();
    
    // Load saved data
    load_data();
    
    http_client_t *client = http_client_create();
    if (!client) {
        printf("Failed to create HTTP client\n");
        return -1;
    }
    
    // Initialize GLFW
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        http_client_destroy(client);
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
        draw_ui(ctx, client);
        
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
    return 0;
}
