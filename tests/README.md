# API Kit Testing Guide

This document describes the testing framework and methodology for the API Kit project.

## Overview

The API Kit project uses a comprehensive testing strategy that includes:

- **Unit Tests** for HTTP parser and HTTP client components
- **Integration Tests** with mock servers
- **Memory Management Tests** to prevent leaks
- **Concurrent Testing** for thread safety
- **Error Handling Tests** for robust error conditions

## Testing Framework

We use the **Unity** testing framework, which is:
- Lightweight and designed for C projects
- Easy to integrate with CMake
- Provides clear assertion macros
- Supports test fixtures and setup/teardown

## Project Structure

```
tests/
├── unity/              # Unity testing framework
│   ├── unity.h
│   └── unity.c
├── fixtures/           # Test data files
│   ├── simple_request.http
│   ├── complex_request.http
│   └── malformed_request.http
├── test_http_parser.c  # HTTP parser unit tests
├── test_http_client.c  # HTTP client unit tests (with mock server)
└── README.md          # This file
```

## Running Tests

### Quick Start

All testing is done through standard CMake/CTest commands:

```bash
# Build and configure
mkdir -p build && cd build
cmake ..
make

# Run all tests
ctest
```

### CTest Commands

1. **Run all tests:**
   ```bash
   ctest                  # Run all tests
   ctest --verbose        # Verbose output
   ctest --output-on-failure  # Show output only on failure
   ```

2. **Run specific test suites:**
   ```bash
   ctest -R HttpParser    # Run parser tests only
   ctest -R HttpClient    # Run client tests only
   ctest -R SimpleClient  # Run simple client tests only
   ```

3. **Run individual tests:**
   ```bash
   ./test_http_parser     # Parser tests
   ./test_http_client     # Client tests (with mock server)
   ./test_simple_client   # Simple client tests
   ```

4. **Parallel execution:**
   ```bash
   ctest -j4              # Run tests in parallel (4 jobs)
   ```

5. **Test selection:**
   ```bash
   ctest -L quick         # Run tests labeled as 'quick' (if any)
   ctest --exclude-regex "Client"  # Exclude client tests
   ```

## Test Categories

### HTTP Parser Tests (`test_http_parser.c`)

Tests the HTTP request parsing and collection management functionality:

- **Collection Management:**
  - `test_http_collection_clear()` - Collection initialization and clearing
  - `test_http_collection_add_success()` - Adding requests to collection
  - `test_http_collection_add_full()` - Handling full collections
  - `test_collection_edge_cases()` - Boundary condition testing

- **File Parsing:**
  - `test_http_parse_file_simple()` - Basic HTTP file parsing
  - `test_http_parse_file_complex()` - Complex multi-request files
  - `test_http_parse_file_not_found()` - Error handling for missing files
  - `test_http_parse_malformed_requests()` - Malformed input handling

- **File Operations:**
  - `test_http_save_file()` - Saving collections to files
  - `test_http_format_request()` - Request formatting
  - `test_http_format_request_small_buffer()` - Buffer overflow protection

### HTTP Client Tests (`test_http_client.c`)

Tests the HTTP client functionality with a built-in mock server:

- **Client Management:**
  - `test_http_client_create_destroy()` - Client lifecycle management
  - `test_response_memory_management()` - Memory leak prevention

- **HTTP Methods:**
  - `test_http_get_request()` - GET request handling
  - `test_http_post_request_with_json()` - POST with JSON payload
  - `test_http_methods()` - PUT, DELETE, PATCH methods

- **Request Features:**
  - `test_http_request_with_headers()` - Custom headers support
  - `test_http_timeout()` - Timeout configuration

- **Response Handling:**
  - `test_http_404_response()` - 404 Not Found responses
  - `test_http_500_response()` - 500 Server Error responses

- **Error Handling:**
  - `test_http_invalid_url()` - Invalid URL handling
  - `test_http_null_parameters()` - NULL parameter validation

- **Concurrency:**
  - `test_concurrent_requests()` - Thread safety testing

## Mock Server

The HTTP client tests include a built-in mock server that:

- Runs on `localhost:8888` during tests
- Supports all HTTP methods (GET, POST, PUT, DELETE, PATCH)
- Returns different responses based on URL paths:
  - `/users` → 200 OK with success message
  - `/notfound` → 404 Not Found
  - `/error` → 500 Internal Server Error
  - `/timeout` → Delayed response for timeout testing

## Test Data

### Fixtures

The `tests/fixtures/` directory contains HTTP files for testing:

- **`simple_request.http`** - Basic GET, POST, PUT requests
- **`complex_request.http`** - Complex scenarios with authentication, file uploads
- **`malformed_request.http`** - Intentionally malformed requests for error testing

### Example Test Data Format

```http
### Simple GET Request
GET https://api.example.com/users

---

### POST Request with JSON
POST https://api.example.com/users
Content-Type: application/json
Authorization: Bearer token123

{
  "name": "John Doe",
  "email": "john@example.com"
}
```

## Writing New Tests

### Adding HTTP Parser Tests

1. Add test function to `test_http_parser.c`:
   ```c
   void test_my_new_feature(void) {
       // Setup
       http_collection_t collection;
       http_collection_clear(&collection);
       
       // Test logic
       int result = my_function(&collection);
       
       // Assertions
       TEST_ASSERT_EQUAL_INT(0, result);
       TEST_ASSERT_EQUAL_INT(1, collection.count);
   }
   ```

2. Add to main test runner:
   ```c
   RUN_TEST(test_my_new_feature);
   ```

### Adding HTTP Client Tests

1. Add test function to `test_http_client.c`:
   ```c
   void test_my_client_feature(void) {
       TEST_ASSERT_NOT_NULL(test_client);
       
       http_response_t *response = http_request(
           test_client,
           HTTP_METHOD_GET,
           TEST_URL_BASE "/my-endpoint",
           NULL
       );
       
       TEST_ASSERT_NOT_NULL(response);
       TEST_ASSERT_EQUAL_INT(200, response->status_code);
       
       http_response_free(response);
   }
   ```

2. Add to main test runner:
   ```c
   RUN_TEST(test_my_client_feature);
   ```

## Test Best Practices

1. **Always clean up resources** - Use `setUp()` and `tearDown()` functions
2. **Test edge cases** - Empty inputs, NULL pointers, boundary conditions
3. **Test error conditions** - Invalid inputs, network failures, memory exhaustion
4. **Use descriptive test names** - Function names should clearly indicate what's being tested
5. **Keep tests independent** - Each test should work in isolation
6. **Use appropriate assertions** - Choose the right `TEST_ASSERT_*` macro for the check

## Continuous Integration

The test suite is designed to be CI-friendly:

- **Exit codes** - Tests return non-zero on failure
- **Timeouts** - Tests have reasonable timeout limits
- **Deterministic** - Tests don't rely on external services
- **Fast execution** - Most tests complete in under 30 seconds

## Troubleshooting

### Common Issues

1. **Mock server port conflicts:**
   - The mock server uses port 8888
   - Ensure no other services are using this port
   - Tests will fail if the port is unavailable

2. **Missing test fixtures:**
   - Ensure `tests/fixtures/` directory exists
   - CMake should copy fixtures to build directory automatically

3. **Memory leaks:**
   - Use tools like `valgrind` to check for memory leaks
   - All allocated memory should be freed in tests

4. **Timing issues:**
   - Some tests involve network operations and may be sensitive to timing
   - Increase timeouts if tests fail intermittently

### Debugging Tests

1. **Run individual tests:**
   ```bash
   # Enable verbose output
   ./test_http_parser
   ./test_http_client
   ```

2. **Use debugger:**
   ```bash
   gdb ./test_http_parser
   (gdb) run
   ```

3. **Check test fixtures:**
   ```bash
   cat tests/fixtures/simple_request.http
   ```

## Contributing

When adding new features to API Kit:

1. Write tests first (TDD approach)
2. Ensure all existing tests pass
3. Add new tests for new functionality
4. Update this documentation if needed
5. Run the full test suite before submitting changes

## Performance Considerations

- Tests are designed to run quickly (< 1 minute total)
- Mock server minimizes network overhead
- File I/O tests use small test files
- Memory allocations are kept minimal

The testing framework ensures API Kit remains reliable, maintainable, and bug-free as it evolves.
