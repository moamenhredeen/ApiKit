#include "http_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int http_parse_file(const char* filename, http_collection_t* collection) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return -1; // File doesn't exist or can't be opened
    }
    
    http_collection_clear(collection);
    
    char line[2048];
    char current_name[128] = "";
    char current_method[10] = "";
    char current_url[512] = "";
    char current_headers[1024] = "";
    char current_body[2048] = "";
    int reading_body = 0;
    int headers_finished = 0;
    
    while (fgets(line, sizeof(line), file) && collection->count < 50) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip comments that don't start with ###
        if (line[0] == '#' && !(line[0] == '#' && line[1] == '#' && line[2] == '#')) {
            continue;
        }
        
        // Check for request name (### Name)
        if (strncmp(line, "###", 3) == 0) {
            // Save previous request if any
            if (strlen(current_method) > 0 && strlen(current_url) > 0) {
                http_request_t request;
                strncpy(request.name, current_name, sizeof(request.name) - 1);
                strncpy(request.method, current_method, sizeof(request.method) - 1);
                strncpy(request.url, current_url, sizeof(request.url) - 1);
                strncpy(request.headers, current_headers, sizeof(request.headers) - 1);
                strncpy(request.body, current_body, sizeof(request.body) - 1);
                
                // Null terminate strings
                request.name[sizeof(request.name) - 1] = '\0';
                request.method[sizeof(request.method) - 1] = '\0';
                request.url[sizeof(request.url) - 1] = '\0';
                request.headers[sizeof(request.headers) - 1] = '\0';
                request.body[sizeof(request.body) - 1] = '\0';
                
                http_collection_add(collection, &request);
            }
            
            // Start new request
            strncpy(current_name, line + 4, sizeof(current_name) - 1); // Skip "### "
            current_name[sizeof(current_name) - 1] = '\0';
            current_method[0] = '\0';
            current_url[0] = '\0';
            current_headers[0] = '\0';
            current_body[0] = '\0';
            reading_body = 0;
            headers_finished = 0;
            continue;
        }
        
        // Check for separator (---)
        if (strncmp(line, "---", 3) == 0) {
            // Save current request
            if (strlen(current_method) > 0 && strlen(current_url) > 0) {
                http_request_t request;
                strncpy(request.name, current_name, sizeof(request.name) - 1);
                strncpy(request.method, current_method, sizeof(request.method) - 1);
                strncpy(request.url, current_url, sizeof(request.url) - 1);
                strncpy(request.headers, current_headers, sizeof(request.headers) - 1);
                strncpy(request.body, current_body, sizeof(request.body) - 1);
                
                // Null terminate strings
                request.name[sizeof(request.name) - 1] = '\0';
                request.method[sizeof(request.method) - 1] = '\0';
                request.url[sizeof(request.url) - 1] = '\0';
                request.headers[sizeof(request.headers) - 1] = '\0';
                request.body[sizeof(request.body) - 1] = '\0';
                
                http_collection_add(collection, &request);
            }
            
            // Reset for next request
            current_name[0] = '\0';
            current_method[0] = '\0';
            current_url[0] = '\0';
            current_headers[0] = '\0';
            current_body[0] = '\0';
            reading_body = 0;
            headers_finished = 0;
            continue;
        }
        
        // Skip empty lines
        if (strlen(line) == 0) {
            if (!headers_finished && strlen(current_method) > 0) {
                headers_finished = 1;
                reading_body = 1;
            }
            continue;
        }
        
        // Parse method and URL line
        if (strlen(current_method) == 0) {
            char *space = strchr(line, ' ');
            if (space) {
                *space = '\0';
                strncpy(current_method, line, sizeof(current_method) - 1);
                current_method[sizeof(current_method) - 1] = '\0';
                strncpy(current_url, space + 1, sizeof(current_url) - 1);
                current_url[sizeof(current_url) - 1] = '\0';
            }
            continue;
        }
        
        // Parse headers or body
        if (reading_body) {
            // Add to body
            if (strlen(current_body) > 0) {
                strncat(current_body, "\n", sizeof(current_body) - strlen(current_body) - 1);
            }
            strncat(current_body, line, sizeof(current_body) - strlen(current_body) - 1);
        } else {
            // Add to headers
            if (strlen(current_headers) > 0) {
                strncat(current_headers, "\n", sizeof(current_headers) - strlen(current_headers) - 1);
            }
            strncat(current_headers, line, sizeof(current_headers) - strlen(current_headers) - 1);
        }
    }
    
    // Save last request if any
    if (strlen(current_method) > 0 && strlen(current_url) > 0 && collection->count < 50) {
        http_request_t request;
        strncpy(request.name, current_name, sizeof(request.name) - 1);
        strncpy(request.method, current_method, sizeof(request.method) - 1);
        strncpy(request.url, current_url, sizeof(request.url) - 1);
        strncpy(request.headers, current_headers, sizeof(request.headers) - 1);
        strncpy(request.body, current_body, sizeof(request.body) - 1);
        
        // Null terminate strings
        request.name[sizeof(request.name) - 1] = '\0';
        request.method[sizeof(request.method) - 1] = '\0';
        request.url[sizeof(request.url) - 1] = '\0';
        request.headers[sizeof(request.headers) - 1] = '\0';
        request.body[sizeof(request.body) - 1] = '\0';
        
        http_collection_add(collection, &request);
    }
    
    fclose(file);
    return 0;
}

int http_save_file(const char* filename, const http_collection_t* collection) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "# API Kit Collection\n");
    fprintf(file, "# Generated by API Kit - HTTP Client\n\n");
    
    for (int i = 0; i < collection->count; i++) {
        const http_request_t* request = &collection->requests[i];
        
        // Add request name as comment
        fprintf(file, "### %s\n", request->name);
        
        // Add method and URL
        fprintf(file, "%s %s\n", request->method, request->url);
        
        // Add headers if any
        if (strlen(request->headers) > 0) {
            // Parse headers line by line
            char headers_copy[1024];
            strncpy(headers_copy, request->headers, sizeof(headers_copy));
            headers_copy[sizeof(headers_copy) - 1] = '\0';
            
            char *line = strtok(headers_copy, "\n");
            while (line) {
                // Skip empty lines
                if (strlen(line) > 0) {
                    fprintf(file, "%s\n", line);
                }
                line = strtok(NULL, "\n");
            }
        }
        
        // Add body if any (for POST/PUT/PATCH)
        if (strlen(request->body) > 0 && 
            (strcmp(request->method, "POST") == 0 || 
             strcmp(request->method, "PUT") == 0 || 
             strcmp(request->method, "PATCH") == 0)) {
            fprintf(file, "\n%s\n", request->body);
        }
        
        // Add separator between requests
        if (i < collection->count - 1) {
            fprintf(file, "\n---\n\n");
        }
    }
    
    fclose(file);
    return 0;
}

int http_parse_request(const char* content, http_request_t* request) {
    // Simple implementation - could be expanded
    // For now, just clear the request
    memset(request, 0, sizeof(http_request_t));
    strncpy(request->name, "Parsed Request", sizeof(request->name) - 1);
    return 0;
}

int http_format_request(const http_request_t* request, char* buffer, size_t buffer_size) {
    int written = snprintf(buffer, buffer_size,
        "### %s\n%s %s\n%s\n\n%s",
        request->name,
        request->method,
        request->url,
        request->headers,
        request->body);
    
    return (written < (int)buffer_size) ? 0 : -1;
}

int http_collection_add(http_collection_t* collection, const http_request_t* request) {
    if (collection->count >= 50) {
        return -1; // Collection is full
    }
    
    collection->requests[collection->count] = *request;
    collection->count++;
    return 0;
}

void http_collection_clear(http_collection_t* collection) {
    collection->count = 0;
    memset(collection->requests, 0, sizeof(collection->requests));
}
