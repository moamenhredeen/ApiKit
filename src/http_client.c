#include "http_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static size_t write_callback(void *contents, size_t size, size_t nmemb, http_response_t *response) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(response->body, response->body_size + realsize + 1);

    if (!ptr) {
        return 0;  // Out of memory
    }

    response->body = ptr;
    memcpy(&(response->body[response->body_size]), contents, realsize);
    response->body_size += realsize;
    response->body[response->body_size] = 0;

    return realsize;
}

static size_t header_callback(void *contents, size_t size, size_t nmemb, http_response_t *response) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(response->headers, response->headers_size + realsize + 1);

    if (!ptr) {
        return 0;
    }

    response->headers = ptr;
    memcpy(&(response->headers[response->headers_size]), contents, realsize);
    response->headers_size += realsize;
    response->headers[response->headers_size] = 0;

    return realsize;
}

// Instance management
http_client_t *http_client_create(void) {
    // TODO: handle error gracefully (what should we do if we can not init curl)
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return NULL;
    }
    http_client_t *client = malloc(sizeof(http_client_t));
    if (!client) {
        return NULL;
    }

    client->curl = curl_easy_init();
    if (!client->curl) {
        free(client);
        return NULL;
    }

    client->headers = NULL;
    return client;
}

void http_client_destroy(http_client_t *client) {
    if (!client) return;
    
    if (client->headers) {
        curl_slist_free_all(client->headers);
    }
    if (client->curl) {
        curl_easy_cleanup(client->curl);
    }
    free(client);
    curl_global_cleanup();
}

// Request handling
http_response_t *http_request(
    http_client_t *client,
    http_method_t method,
    const char *url,
    const http_request_options_t *options) {

    if (!client || !client->curl || !url) {
        return NULL;
    }

    // Reset any previous headers
    if (client->headers) {
        curl_slist_free_all(client->headers);
        client->headers = NULL;
    }

    // Initialize response
    http_response_t *response = calloc(1, sizeof(http_response_t));
    if (!response) {
        return NULL;
    }

    response->body = malloc(1);
    response->headers = malloc(1);
    if (!response->body || !response->headers) {
        http_response_free(response);
        return NULL;
    }

    // Basic curl options
    curl_easy_setopt(client->curl, CURLOPT_URL, url);
    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(client->curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(client->curl, CURLOPT_HEADERDATA, response);
    curl_easy_setopt(client->curl, CURLOPT_USERAGENT, "apikit/1.0");

    // Set HTTP method
    switch (method) {
        case HTTP_METHOD_GET:
            curl_easy_setopt(client->curl, CURLOPT_HTTPGET, 1L);
            break;
        case HTTP_METHOD_POST:
            curl_easy_setopt(client->curl, CURLOPT_POST, 1L);
            break;
        case HTTP_METHOD_PUT:
            curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "PUT");
            break;
        case HTTP_METHOD_DELETE:
            curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        case HTTP_METHOD_PATCH:
            curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            break;
    }

    // Handle options
    if (options) {
        // Set timeout
        if (options->timeout_ms > 0) {
            curl_easy_setopt(client->curl, CURLOPT_TIMEOUT_MS, options->timeout_ms);
        }

        // Set request body
        if (options->body) {
            curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, options->body);
        }

        // Set content type
        if (options->content_type) {
            char header[256];
            snprintf(header, sizeof(header), "Content-Type: %s", options->content_type);
            client->headers = curl_slist_append(client->headers, header);
        }

        // Set custom headers
        if (options->headers) {
            for (int i = 0; options->headers[i]; i++) {
                client->headers = curl_slist_append(client->headers, options->headers[i]);
            }
        }

        // Apply headers
        if (client->headers) {
            curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, client->headers);
        }
    }

    // Perform request
    CURLcode res = curl_easy_perform(client->curl);
    
    // Get response code
    curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &response->status_code);

    // Handle curl errors
    if (res != CURLE_OK) {
        response->error_message = strdup(curl_easy_strerror(res));
    }

    return response;
}

void http_response_free(http_response_t *response) {
    if (!response) return;
    
    free(response->body);
    free(response->headers);
    free(response->error_message);
    free(response);
}
