/* ============================================================================
 * API Kit - Store Module
 * 
 * Application state management and data persistence.
 * Inspired by Svelte stores for reactive state management.
 * ============================================================================ */

#ifndef STORE_H
#define STORE_H

#include "http_client.h"
#include "http_parser.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MAX_WORKSPACES 5
#define MAX_COLLECTIONS_PER_WORKSPACE 10
#define MAX_REQUESTS_PER_COLLECTION 20
#define MAX_HISTORY_ITEMS 100

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

// HTTP request history item
typedef struct {
    char url[512];
    long status_code;
    char method[10];
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

// Collection of requests (tree node in GUI)
typedef struct {
    char name[128];
    int request_count;
    int expanded;
    request_item_t requests[MAX_REQUESTS_PER_COLLECTION];
} collection_t;

// Workspace containing collections
typedef struct {
    char name[128];
    char filename[256];
    int collection_count;
    collection_t collections[MAX_COLLECTIONS_PER_WORKSPACE];
} workspace_t;

// Drag and drop state
typedef struct {
    int active;
    int workspace_index;
    int collection_index;
    int request_index;
    char preview[256];
} drag_state_t;

// Selection state for operations
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


typedef enum {
  ROUTE_MAIN,
  ROUTE_SETTINGS
} routes_t;


// Main application state (Svelte-like store)
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

    routes_t route;
} app_state_t;

/* ============================================================================
 * STORE API - Global State Access
 * ============================================================================ */

// Get reference to the global app state (like a Svelte store)
app_state_t* store_get_state(void);

/* ============================================================================
 * PERSISTENCE API
 * ============================================================================ */

// Data persistence functions
void store_save_data(void);
void store_load_data(void);

// Settings persistence functions
void store_save_settings(void);
void store_load_settings(void);

/* ============================================================================
 * WORKSPACE MANAGEMENT API
 * ============================================================================ */

// Workspace operations
void store_ensure_default_workspace(void);
void store_scan_and_load_workspaces(void);
void store_ensure_data_directory(void);

/* ============================================================================
 * HISTORY MANAGEMENT API
 * ============================================================================ */

// History operations
void store_add_to_history(const char* method, const char* url, long status_code);

/* ============================================================================
 * COLLECTION MANAGEMENT API
 * ============================================================================ */

// Collection operations
void store_add_to_collection(const char* collection_name, const char* request_name, 
                            const char* method, const char* url, const char* headers, const char* body);
void store_move_request_to_collection(int src_workspace, int src_collection, int src_request, 
                                     int dest_workspace, int dest_collection);
void store_delete_selected_item(void);

#endif // STORE_H
