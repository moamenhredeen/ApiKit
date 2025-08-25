#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h>

// HTTP request structure
typedef struct {
    char name[128];
    char method[10];
    char url[512];
    char headers[1024];
    char body[2048];
} http_request_t;

// Collection structure
typedef struct {
    http_request_t requests[50];
    int count;
} http_collection_t;

/**
 * @brief Parse HTTP collection file
 * @param filename Path to the HTTP file
 * @param collection Pointer to collection structure to fill
 * @return 0 on success, -1 on error
 */
int http_parse_file(const char* filename, http_collection_t* collection);

/**
 * @brief Save collection to HTTP file
 * @param filename Path to the HTTP file
 * @param collection Pointer to collection structure to save
 * @return 0 on success, -1 on error
 */
int http_save_file(const char* filename, const http_collection_t* collection);

/**
 * @brief Parse a single HTTP request from string
 * @param content HTTP request content
 * @param request Pointer to request structure to fill
 * @return 0 on success, -1 on error
 */
int http_parse_request(const char* content, http_request_t* request);

/**
 * @brief Format a single HTTP request to string
 * @param request Pointer to request structure
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int http_format_request(const http_request_t* request, char* buffer, size_t buffer_size);

/**
 * @brief Add request to collection
 * @param collection Pointer to collection
 * @param request Pointer to request to add
 * @return 0 on success, -1 if collection is full
 */
int http_collection_add(http_collection_t* collection, const http_request_t* request);

/**
 * @brief Clear collection
 * @param collection Pointer to collection to clear
 */
void http_collection_clear(http_collection_t* collection);

#endif
