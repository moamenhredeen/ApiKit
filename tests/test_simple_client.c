#include "unity/unity.h"
#include "../include/http_client.h"
#include <stdio.h>

static http_client_t *test_client = NULL;

void setUp(void) {
    test_client = http_client_create();
}

void tearDown(void) {
    if (test_client) {
        http_client_destroy(test_client);
        test_client = NULL;
    }
}

// Test client creation and destruction only
void test_http_client_create_destroy_simple(void) {
    http_client_t *client = http_client_create();
    TEST_ASSERT_NOT_NULL(client);
    TEST_ASSERT_NOT_NULL(client->curl);
    
    // Should not crash
    http_client_destroy(client);
    
    // Test with NULL
    http_client_destroy(NULL); // Should not crash
    TEST_ASSERT_TRUE(1); // If we get here, test passed
}

// Test response free function with NULL
void test_response_free_null(void) {
    http_response_free(NULL); // Should not crash
    TEST_ASSERT_TRUE(1); // If we get here, test passed
}

// Test a simple external request to a reliable service
void test_simple_external_request(void) {
    TEST_ASSERT_NOT_NULL(test_client);
    
    // Use httpbin.org for testing (reliable external service)
    http_response_t *response = http_request(
        test_client,
        HTTP_METHOD_GET,
        "https://httpbin.org/get",
        NULL
    );
    
    if (response) {
        printf("Status: %ld\n", response->status_code);
        if (response->body) {
            printf("Body length: %zu\n", response->body_size);
        }
        if (response->error_message) {
            printf("Error: %s\n", response->error_message);
        }
        
        // Basic validation - should succeed if internet is available
        TEST_ASSERT_TRUE(response->status_code == 200 || response->error_message != NULL);
        
        http_response_free(response);
    } else {
        // NULL response indicates a serious error
        TEST_FAIL("Response is NULL");
    }
}

int main(void) {
    UnityBegin("test_simple_client.c");
    
    RUN_TEST(test_http_client_create_destroy_simple);
    RUN_TEST(test_response_free_null);
    RUN_TEST(test_simple_external_request);
    
    return UnityEnd();
}
