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
#include <stdbool.h>

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#include "http_client.h"
#include "store.h"

/* ============================================================================
 * CONSTANTS AND CONFIGURATION
 * ============================================================================ */

#define SIDEBAR_WIDTH 300

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

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

static void apply_theme(void) {
    // Theme application would go here
    // For now, we just store the preference
}



/* ============================================================================
 * INPUT HANDLING
 * ============================================================================ */

static void handle_keyboard_shortcuts(struct nk_context *ctx) {
    app_state_t* state = store_get_state();
    if (!state->settings.keybindings_enabled) return;
    
    struct nk_input *input = &ctx->input;
    
    // Delete key: Delete selected item
    // if (state->settings.delete_key_enabled && nk_input_is_key_pressed(input, NK_KEY_DEL)) {
    //     store_delete_selected_item();
    // }
    
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
        if (state->settings.ctrl_b_enabled) {
            int ctrl_b_combo = ctrl_pressed && b_pressed;
            if (ctrl_b_combo && !state->keyboard.prev_ctrl_b_combo) {
                state->show_sidebar = !state->show_sidebar;
            }
            state->keyboard.prev_ctrl_b_combo = ctrl_b_combo;
        }
        
        // Ctrl+F combination  
        if (state->settings.ctrl_f_enabled) {
            int ctrl_f_combo = ctrl_pressed && f_pressed;
            if (ctrl_f_combo && !state->keyboard.prev_ctrl_f_combo) {
                state->show_sidebar = 1;
                state->active_tab = 1; // Switch to collections tab
            }
            state->keyboard.prev_ctrl_f_combo = ctrl_f_combo;
        }
    }
    
    // Ctrl+A, Ctrl+V, Ctrl+C are handled by Nuklear automatically for text inputs
}





/* ============================================================================
 * UI COMPONENTS (React-like structure)
 * ============================================================================ */

static void ui_sidebar(struct nk_context *ctx, int width, int height) {
    app_state_t* state = store_get_state();

		nk_flags sidebar_flags =  NK_WINDOW_NO_SCROLLBAR;
    if (nk_begin(ctx, "Sidebar", nk_rect(0, 0, width, height), sidebar_flags)) {

    		ui_workspace_dropdown(ctx);

        // Search field
        nk_layout_row_static(ctx, 25, width, 1);
        nk_edit_string_zero_terminated(ctx,
                                       NK_EDIT_FIELD,
                                       state->search_text, 
                                       sizeof(state->search_text),
                                       nk_filter_default);
        
        // Tabs
        nk_layout_row_dynamic(ctx, 30, 2);
        const char *history_tab_text = state->active_tab == 0 ? "[History]" : "History";
        if (nk_button_label(ctx, history_tab_text)) {
            state->active_tab = 0;
        }
        const char *collections_tab_text = state->active_tab == 1 ? "[collection_ts]" : "collection_ts";
        if (nk_button_label(ctx, collections_tab_text)) {
            state->active_tab = 1;
        }
        
        // Tab content (reduced height to make room for settings button and help)
        nk_layout_row_dynamic(ctx, height, 1);
        if (nk_group_begin(ctx, "tab_content", NK_WINDOW_BORDER)) {
            if (state->active_tab == 0) {
                ui_history_tab(ctx);
            } else if (state->active_tab == 1) {
                ui_collections_tab(ctx);
            }
            nk_group_end(ctx);
        }

				// Handle drag cancellation
				if (state->drag.active && nk_input_is_mouse_released(&ctx->input, NK_BUTTON_LEFT)) {
						state->drag.active = 0;
				}
				
				// Draw drag preview
				ui_drag_preview(ctx);
				
				// settings_t button at bottom
				nk_layout_row_dynamic(ctx, 30, 1);
				if (nk_button_label(ctx, "Settings")) {
						if (state->route == ROUTE_SETTINGS) {
								state->route = ROUTE_MAIN;
						} else {
								state->route = ROUTE_SETTINGS;
						}
				}
    }
    nk_end(ctx);
}

static void ui_history_tab(struct nk_context *ctx) {
    app_state_t* state = store_get_state();
    
    for (int i = state->history_count - 1; i >= 0; i--) {
        history_item_t* item = &state->history[i];
        
        // Filter by search
        if (strlen(state->search_text) > 0 && 
            !strstr(item->url, state->search_text) && 
            !strstr(item->method, state->search_text)) {
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
                strncpy(state->url, item->url, sizeof(state->url));
                state->url[sizeof(state->url) - 1] = '\0';
                
                // Set method
                if (strcmp(item->method, "GET") == 0) state->method_selected = 0;
                else if (strcmp(item->method, "POST") == 0) state->method_selected = 1;
                else if (strcmp(item->method, "PUT") == 0) state->method_selected = 2;
                else if (strcmp(item->method, "DELETE") == 0) state->method_selected = 3;
                else if (strcmp(item->method, "PATCH") == 0) state->method_selected = 4;
            }
            nk_group_end(ctx);
        }
    }
}

static void ui_collections_tab(struct nk_context *ctx) {
    app_state_t* state = store_get_state();
    
    // Save current request button
    nk_layout_row_dynamic(ctx, 30, 1);
    if (nk_button_label(ctx, "+ Save Current Request")) {
        char name[128];
        snprintf(name, sizeof(name), "%s %s", 
                 state->method_selected == 0 ? "GET" :
                 state->method_selected == 1 ? "POST" :
                 state->method_selected == 2 ? "PUT" :
                 state->method_selected == 3 ? "DELETE" : "PATCH",
                 state->url);
        
        const char* method = state->method_selected == 0 ? "GET" :
                           state->method_selected == 1 ? "POST" :
                           state->method_selected == 2 ? "PUT" :
                           state->method_selected == 3 ? "DELETE" : "PATCH";
        
        store_add_to_collection("Default collection_t", name, method, state->url, state->headers, state->body);
    }
    
    // workspace_t dropdown
    ui_workspace_dropdown(ctx);
    
    // collection_t tree
    ui_collection_tree(ctx);
}

static void ui_workspace_dropdown(struct nk_context *ctx) {
	app_state_t* state = store_get_state();
	if (state->workspace_count > 0) {
		workspace_t* current_workspace = &state->workspaces[state->active_workspace];

		nk_layout_row_begin(ctx, NK_DYNAMIC, 25, 2);
		nk_layout_row_push(ctx, 0.8f);
		if (nk_combo_begin_label(ctx, current_workspace->name, nk_vec2(nk_widget_width(ctx), 200))) {
			for (int w = 0; w < state->workspace_count; w++) {
				if (nk_combo_item_label(ctx, state->workspaces[w].name, NK_TEXT_LEFT)) {
					state->active_workspace = w;
				}
			}
			nk_combo_end(ctx);
		}
		nk_layout_row_push(ctx, 0.2f);
		if (nk_button_label(ctx, "+")) {
			state->show_new_workspace_popup = 1;
			strcpy(state->new_workspace_name, "");
		}
		nk_layout_row_end(ctx);
	} else {
		// No workspaces yet, show create button
		nk_layout_row_dynamic(ctx, 25, 1);
		if (nk_button_label(ctx, "Create Workspace")) {
			state->show_new_workspace_popup = 1;
			strcpy(state->new_workspace_name, "");
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
    app_state_t* state = store_get_state();
    
    if (state->drag.active) {
        struct nk_vec2 mouse_pos = ctx->input.mouse.pos;
        nk_layout_space_begin(ctx, NK_STATIC, 20, 1);
        nk_layout_space_push(ctx, nk_rect(mouse_pos.x + 10, mouse_pos.y - 10, 200, 20));
        nk_label_colored(ctx, state->drag.preview, NK_TEXT_LEFT, nk_rgb(255, 255, 0));
        nk_layout_space_end(ctx);
    }
}

static void ui_settings_page(struct nk_context *ctx, int x, int width, int height) {
    app_state_t* state = store_get_state();
    
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
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, state->settings.data_folder_path, 
                                     sizeof(state->settings.data_folder_path), nk_filter_default);
        
        nk_layout_row_dynamic(ctx, 20, 1); // Spacer
        
        // Theme section
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, "Appearance", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Theme:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 30, 1);
        static const char *themes[] = {"Default", "Dark", "Light"};
        state->settings.theme_selected = nk_combo(ctx, themes, 3, state->settings.theme_selected, 30, nk_vec2(200, 120));
        
        nk_layout_row_dynamic(ctx, 20, 1); // Spacer
        
        // Keyboard shortcuts section
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, "Keyboard Shortcuts", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(ctx, 30, 1);
        if (nk_checkbox_label(ctx, "Enable Keyboard Shortcuts", &state->settings.keybindings_enabled)) {
            store_save_settings();
        }
        
        if (state->settings.keybindings_enabled) {
            nk_layout_row_dynamic(ctx, 5, 1); // Small spacer
            
            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_checkbox_label(ctx, "Ctrl+B (Toggle Sidebar)", &state->settings.ctrl_b_enabled)) {
                store_save_settings();
            }
            
            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_checkbox_label(ctx, "Ctrl+F (Focus Search)", &state->settings.ctrl_f_enabled)) {
                store_save_settings();
            }
            
            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_checkbox_label(ctx, "Delete Key (Remove Items)", &state->settings.delete_key_enabled)) {
                store_save_settings();
            }
        }
        
        nk_layout_row_dynamic(ctx, 30, 1); // Spacer
        
        // Action buttons
        nk_layout_row_dynamic(ctx, 40, 3);
        if (nk_button_label(ctx, "Apply Theme")) {
            apply_theme();
            store_save_settings();
        }
        if (nk_button_label(ctx, "Save settings_t")) {
            store_save_settings();
        }
        if (nk_button_label(ctx, "Close settings_t")) {
            state->show_settings_page = 0;
        }
    }
    nk_end(ctx);
}

static void ui_main_panel(struct nk_context *ctx, http_client_t *client, int x, int width, int height) {
    app_state_t* state = store_get_state();
    
    if (nk_begin(ctx, "API Kit", nk_rect(x, 0, width, height), NK_WINDOW_NO_SCROLLBAR)) {
        // Method dropdown, URL input, and Send button in same row with fixed sizes
        static const char *methods[] = {"GET", "POST", "PUT", "DELETE", "PATCH"};
        nk_layout_row_begin(ctx, NK_STATIC, 40, 3);
        nk_layout_row_push(ctx, 100);  // Fixed 100px for dropdown
        state->method_selected = nk_combo(ctx, methods, 5, state->method_selected, 25, nk_vec2(120, 200));
        nk_layout_row_push(ctx, width - 180);  // Remaining space minus button and dropdown
        nk_edit_string_zero_terminated(ctx,
                                       NK_EDIT_FIELD,
                                       state->url, 
                                       sizeof(state->url),
                                       nk_filter_default);

        nk_layout_row_push(ctx, 80);
        if (nk_button_label(ctx, "SEND") && !state->request_in_progress) {
            state->request_in_progress = 1;
            
            // Convert method selection to enum
            http_method_t method = HTTP_METHOD_GET;
            switch (state->method_selected) {
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
            strncpy(headers_copy, state->headers, sizeof(headers_copy));
            
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
                if (strlen(state->body) > 0) {
                    options.body = state->body;
                }
            }
            
            // Make HTTP request
            http_response_t* response = http_request(client, method, state->url, &options);
            
            if (response) {
                state->last_status_code = response->status_code;
                snprintf(state->response, sizeof(state->response),
                         "Status: %ld\n\n--- Headers ---\n%s\n--- Body ---\n%s", 
                         response->status_code,
                         response->headers ? response->headers : "No headers",
                         response->body ? response->body : "No response body");
                
                // Add to history
                const char* method_name = methods[state->method_selected];
                store_add_to_history(method_name, state->url, response->status_code);
                
                http_response_free(response);
            } else {
                strncpy(state->response, "Request failed!", sizeof(state->response));
                state->last_status_code = 0;
                
                // Add failed request to history
                const char* method_name = methods[state->method_selected];
                store_add_to_history(method_name, state->url, 0);
            }
            
            state->request_in_progress = 0;
        }
        nk_layout_row_end(ctx);
        
        // Status indicator
        if (state->request_in_progress) {
            nk_layout_row_static(ctx, 20, 200, 1);
            nk_label_colored(ctx, "Sending request...", NK_TEXT_LEFT, nk_rgb(255, 165, 0));
        } else if (state->last_status_code > 0) {
            nk_layout_row_static(ctx, 20, 300, 1);
            char status_text[100];
            snprintf(status_text, sizeof(status_text), "Last Status: %ld", state->last_status_code);
            struct nk_color color = nk_rgb(0, 255, 0); // Green for success
            if (state->last_status_code >= 400) {
                color = nk_rgb(255, 0, 0); // Red for errors
            } else if (state->last_status_code >= 300) {
                color = nk_rgb(255, 165, 0); // Orange for redirects
            }
            nk_label_colored(ctx, status_text, NK_TEXT_LEFT, color);
        }
        
        // Headers section
        nk_layout_row_static(ctx, 20, 100, 1);
        nk_label(ctx, "Headers:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 100, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, state->headers, 
                                       sizeof(state->headers), nk_filter_default);
        
        // Body section (only for POST/PUT/PATCH)
        if (state->method_selected == 1 || state->method_selected == 2 || state->method_selected == 4) {
            nk_layout_row_static(ctx, 20, 100, 1);
            nk_label(ctx, "Request Body:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 120, 1);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, state->body, 
                                           sizeof(state->body), nk_filter_default);
        }
        
        // Response section
        nk_layout_row_static(ctx, 20, 100, 1);
        nk_label(ctx, "Response:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 200, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX | NK_EDIT_READ_ONLY, 
                                       state->response, 
                                       sizeof(state->response), nk_filter_default);
    }
    nk_end(ctx);
}

// Root UI function that orchestrates all components
static void draw_ui(struct nk_context *ctx, http_client_t *client) {
    app_state_t* state = store_get_state();
    
    // Handle keyboard shortcuts first
    // handle_keyboard_shortcuts(ctx);
    
    // Get window size
    int width = 0.0F;
		int height = 0.0F;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
    
    int main_area_x = state->show_sidebar ? SIDEBAR_WIDTH : 0;
    int main_area_width = width - main_area_x;
    
    // Draw sidebar if visible
    if (state->show_sidebar) {
        ui_sidebar(ctx, SIDEBAR_WIDTH, height);
    }
    
    // Toggle button when sidebar is hidden
    // if (!state->show_sidebar) {
    //     if (nk_begin(ctx, "ToggleBtn", nk_rect(5, 5, 40, 30), NK_WINDOW_NO_SCROLLBAR)) {
    //         nk_layout_row_static(ctx, 25, 35, 1);
    //         if (nk_button_label(ctx, "▶")) {
    //             state->show_sidebar = 1;
    //         }
    //     }
    //     nk_end(ctx);
    // }
    
    // routing
    switch(state->route){
        case ROUTE_MAIN:
            ui_main_panel(ctx, client, main_area_x, main_area_width, height);
        break;
        case ROUTE_SETTINGS:
            ui_settings_page(ctx, main_area_x, main_area_width, height);
        break;        
    }
}

/* ============================================================================
 * MAIN FUNCTION AND INITIALIZATION
 * ============================================================================ */

int main() {
    // Load settings first
    store_load_settings();
    
    // Load saved data
    store_load_data();
    
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
    struct nk_font *jetbrains_mono_regular = nk_font_atlas_add_from_file(atlas, "JetBrainsMonoNL-Regular.ttf", 24.0f, 0);
    nk_glfw3_font_stash_end(&glfw);


    nk_init_default(ctx, &jetbrains_mono_regular->handle);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        nk_glfw3_new_frame(&glfw);
        
        draw_ui(ctx, client);
        
        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.10F, 0.18F, 0.24F, 1.0F);
        
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, 512 * 1024, 128 * 1024);
        glfwSwapBuffers(window);
    }

    store_save_data();
    
    // Cleanup
    nk_glfw3_shutdown(&glfw);
    glfwDestroyWindow(window);
    glfwTerminate();
    http_client_destroy(client);
    return 0;
}
