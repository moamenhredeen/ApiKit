/* ============================================================================
 * API Kit - Store Implementation
 * 
 * Application state management and data persistence.
 * Inspired by Svelte stores for reactive state management.
 * ============================================================================ */

#include "store.h"
#include "toml.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

/* ============================================================================
 * GLOBAL STATE (The Store)
 * ============================================================================ */

static app_state_t app_state = {
    .route = ROUTE_MAIN,
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

static void extract_workspace_name(const char* filename, char* workspace_name, size_t max_len);
static void load_workspace_from_file(const char* filename);

/* ============================================================================
 * STORE API - Global State Access
 * ============================================================================ */

app_state_t* store_get_state(void) {
    return &app_state;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

void store_ensure_data_directory(void) {
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
        // Parse requests and group them by collection name from the [collection] prefix
        for (int i = 0; i < workspace_collection.count; i++) {
            const http_request_t* request = &workspace_collection.requests[i];
            
            // Extract collection name from request name format "[collection] Request"
            char collection_name[128] = "Default collection";
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

/* ============================================================================
 * WORKSPACE MANAGEMENT
 * ============================================================================ */

void store_scan_and_load_workspaces(void) {
    store_ensure_data_directory();
    
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
        store_ensure_default_workspace();
    }
    
    // Set active workspace to first one
    app_state.active_workspace = 0;
}

void store_ensure_default_workspace(void) {
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
 * HISTORY MANAGEMENT
 * ============================================================================ */

void store_add_to_history(const char* method, const char* url, long status_code) {
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
        store_save_data();
    }
}

/* ============================================================================
 * COLLECTION MANAGEMENT
 * ============================================================================ */

void store_add_to_collection(const char* collection_name, const char* request_name, 
                            const char* method, const char* url, const char* headers, const char* body) {
    store_ensure_default_workspace();
    
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
        store_save_data();
    }
}

void store_move_request_to_collection(int src_workspace, int src_collection, int src_request, 
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
    store_save_data();
}

void store_delete_selected_item(void) {
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
    
    store_save_data();
}

/* ============================================================================
 * SETTINGS PERSISTENCE
 * ============================================================================ */

void store_save_settings(void) {
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

void store_load_settings(void) {
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

/* ============================================================================
 * DATA PERSISTENCE
 * ============================================================================ */

void store_save_data(void) {
    store_ensure_data_directory();
    
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

void store_load_data(void) {
    store_ensure_data_directory();
    
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
    store_scan_and_load_workspaces();
}
