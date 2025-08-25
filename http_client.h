#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <curl/curl.h>

typedef struct {
  CURL *curl;
  struct curl_slist *headers;
} http_client_t;

typedef struct {
  const char **headers;  // Array of "Key: Value" strings, NULL-terminated
  const char *body;
  const char *content_type;
  long timeout_ms;       // Request timeout in milliseconds
} http_request_options_t;

typedef struct {
  char *body;
  size_t body_size;
  long status_code;
  char *headers;
  size_t headers_size;
  char *error_message;
} http_response_t;

typedef enum {
  HTTP_METHOD_GET,
  HTTP_METHOD_POST,
  HTTP_METHOD_PUT,
  HTTP_METHOD_DELETE,
  HTTP_METHOD_PATCH
} http_method_t;

/**
 * @brief Initialize global HTTP client (call once at startup)
 * @return 0 on success, -1 on failure
 */
int http_client_global_init(void);

/**
 * @brief Cleanup global HTTP client (call once at shutdown)
 */
void http_client_global_cleanup(void);

/**
 * @brief Create HTTP client (call once at startup)
 *
 * @return int 0 on success, -1 on failure
 */
http_client_t *http_client_create(void);

/**
 * @brief Destroy HTTP client (call once at shutdown)
 *
 * @return void
 */
void http_client_destroy(http_client_t *client);

/**
 * @brief Perform HTTP request
 *
 * @param url URL to request
 * @param method HTTP method
 * @param options Request options
 * @return * Pointer to response data
 */
http_response_t *http_request(
    http_client_t *client,
    http_method_t method, 
    const char *url, 
    const http_request_options_t *options);

/**
 * @brief Free response memory
 *
 * @param response Pointer to response data
 * @return void
 */
void http_response_free(http_response_t *response);

#endif