#include "unity/unity.h"
#include "../include/http_parser.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Test data directory
#define TEST_FIXTURES_DIR "tests/fixtures/"
#define TEST_OUTPUT_DIR "tests/output/"

// Global test collection
static http_collection_t test_collection;

void setUp(void) {
    // Initialize test collection before each test
    http_collection_clear(&test_collection);
}

void tearDown(void) {
    // Clean up after each test
    http_collection_clear(&test_collection);
}

// Helper function to create test output directory
void create_test_output_dir(void) {
    struct stat st = {0};
    if (stat(TEST_OUTPUT_DIR, &st) == -1) {
        mkdir(TEST_OUTPUT_DIR, 0755);
    }
}

// Test http_collection_clear function
void test_http_collection_clear(void) {
    // Add some dummy data
    test_collection.count = 5;
    strcpy(test_collection.requests[0].name, "Test Request");
    strcpy(test_collection.requests[0].method, "GET");
    
    // Clear the collection
    http_collection_clear(&test_collection);
    
    // Verify it's cleared
    TEST_ASSERT_EQUAL_INT(0, test_collection.count);
    TEST_ASSERT_EQUAL_STRING("", test_collection.requests[0].name);
    TEST_ASSERT_EQUAL_STRING("", test_collection.requests[0].method);
}

// Test http_collection_add function
void test_http_collection_add_success(void) {
    http_request_t request;
    
    // Initialize request
    strcpy(request.name, "Test Request");
    strcpy(request.method, "GET");
    strcpy(request.url, "https://api.example.com/test");
    strcpy(request.headers, "Content-Type: application/json");
    strcpy(request.body, "{\"test\": true}");
    
    // Add to collection
    int result = http_collection_add(&test_collection, &request);
    
    // Verify success
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, test_collection.count);
    TEST_ASSERT_EQUAL_STRING("Test Request", test_collection.requests[0].name);
    TEST_ASSERT_EQUAL_STRING("GET", test_collection.requests[0].method);
    TEST_ASSERT_EQUAL_STRING("https://api.example.com/test", test_collection.requests[0].url);
}

// Test http_collection_add when collection is full
void test_http_collection_add_full(void) {
    http_request_t request;
    strcpy(request.name, "Test Request");
    strcpy(request.method, "GET");
    strcpy(request.url, "https://api.example.com/test");
    
    // Fill the collection to capacity
    test_collection.count = 50;
    
    // Try to add one more
    int result = http_collection_add(&test_collection, &request);
    
    // Should fail
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_EQUAL_INT(50, test_collection.count);
}

// Test parsing a simple HTTP file
void test_http_parse_file_simple(void) {
    int result = http_parse_file(TEST_FIXTURES_DIR "simple_request.http", &test_collection);
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, test_collection.count);
    
    // Test first request
    TEST_ASSERT_EQUAL_STRING("Simple GET Request", test_collection.requests[0].name);
    TEST_ASSERT_EQUAL_STRING("GET", test_collection.requests[0].method);
    TEST_ASSERT_EQUAL_STRING("https://api.example.com/users", test_collection.requests[0].url);
    TEST_ASSERT_EQUAL_STRING("", test_collection.requests[0].headers);
    TEST_ASSERT_EQUAL_STRING("", test_collection.requests[0].body);
    
    // Test second request (POST with JSON)
    TEST_ASSERT_EQUAL_STRING("POST Request with JSON", test_collection.requests[1].name);
    TEST_ASSERT_EQUAL_STRING("POST", test_collection.requests[1].method);
    TEST_ASSERT_EQUAL_STRING("https://api.example.com/users", test_collection.requests[1].url);
    TEST_ASSERT_TRUE(strstr(test_collection.requests[1].headers, "Content-Type: application/json") != NULL);
    TEST_ASSERT_TRUE(strstr(test_collection.requests[1].body, "John Doe") != NULL);
    
    // Test third request (PUT with headers)
    TEST_ASSERT_EQUAL_STRING("PUT Request with Headers", test_collection.requests[2].name);
    TEST_ASSERT_EQUAL_STRING("PUT", test_collection.requests[2].method);
    TEST_ASSERT_EQUAL_STRING("https://api.example.com/users/123", test_collection.requests[2].url);
    TEST_ASSERT_TRUE(strstr(test_collection.requests[2].headers, "X-Custom-Header: custom-value") != NULL);
    TEST_ASSERT_TRUE(strstr(test_collection.requests[2].body, "Jane Doe") != NULL);
}

// Test parsing a complex HTTP file
void test_http_parse_file_complex(void) {
    int result = http_parse_file(TEST_FIXTURES_DIR "complex_request.http", &test_collection);
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(5, test_collection.count);
    
    // Test authentication request
    TEST_ASSERT_EQUAL_STRING("Authentication Request", test_collection.requests[0].name);
    TEST_ASSERT_EQUAL_STRING("POST", test_collection.requests[0].method);
    TEST_ASSERT_EQUAL_STRING("https://auth.example.com/login", test_collection.requests[0].url);
    
    // Test PATCH request
    TEST_ASSERT_EQUAL_STRING("Update User Settings", test_collection.requests[2].name);
    TEST_ASSERT_EQUAL_STRING("PATCH", test_collection.requests[2].method);
    TEST_ASSERT_TRUE(strstr(test_collection.requests[2].headers, "X-Request-ID: req-123") != NULL);
    
    // Test DELETE request
    TEST_ASSERT_EQUAL_STRING("Delete User Account", test_collection.requests[3].name);
    TEST_ASSERT_EQUAL_STRING("DELETE", test_collection.requests[3].method);
}

// Test parsing non-existent file
void test_http_parse_file_not_found(void) {
    int result = http_parse_file("non_existent_file.http", &test_collection);
    
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_EQUAL_INT(0, test_collection.count);
}

// Test saving collection to file
void test_http_save_file(void) {
    // Create test requests
    http_request_t request1, request2;
    
    strcpy(request1.name, "GET Request");
    strcpy(request1.method, "GET");
    strcpy(request1.url, "https://api.example.com/users");
    strcpy(request1.headers, "Authorization: Bearer token123");
    strcpy(request1.body, "");
    
    strcpy(request2.name, "POST Request");
    strcpy(request2.method, "POST");
    strcpy(request2.url, "https://api.example.com/users");
    strcpy(request2.headers, "Content-Type: application/json");
    strcpy(request2.body, "{\"name\": \"John\"}");
    
    // Add to collection
    http_collection_add(&test_collection, &request1);
    http_collection_add(&test_collection, &request2);
    
    // Save to file
    create_test_output_dir();
    int result = http_save_file(TEST_OUTPUT_DIR "test_output.http", &test_collection);
    
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify file was created by trying to parse it back
    http_collection_t loaded_collection;
    http_collection_clear(&loaded_collection);
    int parse_result = http_parse_file(TEST_OUTPUT_DIR "test_output.http", &loaded_collection);
    
    TEST_ASSERT_EQUAL_INT(0, parse_result);
    TEST_ASSERT_EQUAL_INT(2, loaded_collection.count);
    TEST_ASSERT_EQUAL_STRING("GET Request", loaded_collection.requests[0].name);
    TEST_ASSERT_EQUAL_STRING("POST Request", loaded_collection.requests[1].name);
    
    // Clean up
    unlink(TEST_OUTPUT_DIR "test_output.http");
}

// Test http_format_request function
void test_http_format_request(void) {
    http_request_t request;
    char buffer[4096];
    
    strcpy(request.name, "Test Request");
    strcpy(request.method, "POST");
    strcpy(request.url, "https://api.example.com/test");
    strcpy(request.headers, "Content-Type: application/json");
    strcpy(request.body, "{\"test\": true}");
    
    int result = http_format_request(&request, buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(strstr(buffer, "### Test Request") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "POST https://api.example.com/test") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "Content-Type: application/json") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "{\"test\": true}") != NULL);
}

// Test http_format_request with small buffer (should fail)
void test_http_format_request_small_buffer(void) {
    http_request_t request;
    char small_buffer[10];
    
    strcpy(request.name, "Test Request");
    strcpy(request.method, "GET");
    strcpy(request.url, "https://api.example.com/test");
    strcpy(request.headers, "");
    strcpy(request.body, "");
    
    int result = http_format_request(&request, small_buffer, sizeof(small_buffer));
    
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test parsing malformed requests
void test_http_parse_malformed_requests(void) {
    // This should handle malformed requests gracefully
    int result = http_parse_file(TEST_FIXTURES_DIR "malformed_request.http", &test_collection);
    
    // Should not crash and should parse what it can
    TEST_ASSERT_EQUAL_INT(0, result);
    // The malformed file might result in 0 or partial requests parsed
    // The key is that it doesn't crash
}

// Test edge cases for collection management
void test_collection_edge_cases(void) {
    http_request_t request;
    
    // Test with very long strings (boundary testing)
    memset(&request, 0, sizeof(request));
    
    // Fill with maximum length strings
    memset(request.name, 'A', sizeof(request.name) - 1);
    request.name[sizeof(request.name) - 1] = '\0';
    
    memset(request.method, 'B', sizeof(request.method) - 1);
    request.method[sizeof(request.method) - 1] = '\0';
    
    memset(request.url, 'C', sizeof(request.url) - 1);
    request.url[sizeof(request.url) - 1] = '\0';
    
    int result = http_collection_add(&test_collection, &request);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, test_collection.count);
}

// Main test runner
int main(void) {
    UnityBegin("test_http_parser.c");
    
    // Collection management tests
    RUN_TEST(test_http_collection_clear);
    RUN_TEST(test_http_collection_add_success);
    RUN_TEST(test_http_collection_add_full);
    RUN_TEST(test_collection_edge_cases);
    
    // File parsing tests
    RUN_TEST(test_http_parse_file_simple);
    RUN_TEST(test_http_parse_file_complex);
    RUN_TEST(test_http_parse_file_not_found);
    RUN_TEST(test_http_parse_malformed_requests);
    
    // File saving tests
    RUN_TEST(test_http_save_file);
    
    // Formatting tests
    RUN_TEST(test_http_format_request);
    RUN_TEST(test_http_format_request_small_buffer);
    
    return UnityEnd();
}
