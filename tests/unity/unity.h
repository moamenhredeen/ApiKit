/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-14 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#ifndef UNITY_FRAMEWORK_H
#define UNITY_FRAMEWORK_H

#include <setjmp.h>
#include <stdint.h>
#include <stdarg.h>

//-------------------------------------------------------
// Unity Attempt to Auto-Detect Integer Types
//-------------------------------------------------------

// Automatically detects C99 integer types
#include <stdint.h>
#ifndef UNITY_EXCLUDE_STDINT_H
#include <stdint.h>
#endif

//-------------------------------------------------------
// Configuration Options
//-------------------------------------------------------
#ifndef UNITY_OUTPUT_CHAR
#define UNITY_OUTPUT_CHAR(a)    putchar(a)
#endif

#ifndef UNITY_OUTPUT_FLUSH
#define UNITY_OUTPUT_FLUSH()    fflush(stdout)
#endif

#ifndef UNITY_OUTPUT_START
#define UNITY_OUTPUT_START()
#endif

#ifndef UNITY_OUTPUT_COMPLETE
#define UNITY_OUTPUT_COMPLETE()
#endif

#ifndef UNITY_FIXTURE_NO_EXTRAS
#define UNITY_FIXTURE_NO_EXTRAS
#endif

//-------------------------------------------------------
// Test Result Structure
//-------------------------------------------------------
struct _Unity
{
    const char* TestFile;
    const char* CurrentTestName;
    uint32_t CurrentTestLineNumber;
    uint32_t NumberOfTests;
    uint32_t TestFailures;
    uint32_t TestIgnores;
    uint32_t CurrentTestFailed;
    uint32_t CurrentTestIgnored;
    jmp_buf AbortFrame;
};

extern struct _Unity Unity;

//-------------------------------------------------------
// Test Setup and Teardown
//-------------------------------------------------------
typedef void (*UnityTestFunction)(void);

void setUp(void);
void tearDown(void);

//-------------------------------------------------------
// Test Running Functions
//-------------------------------------------------------
void UnityBegin(const char* file);
int  UnityEnd(void);
void UnityConcludeTest(void);
void UnityDefaultTestRun(UnityTestFunction Func, const char* FuncName, const int FuncLineNum);

//-------------------------------------------------------
// Test Assertion Functions
//-------------------------------------------------------
void UnityAssertEqualNumber(const long expected, const long actual, const char* msg, const unsigned short lineNumber, const char style);
void UnityAssertEqualIntArray(const int* expected, const int* actual, const uint32_t num_elements, const char* msg, const unsigned short lineNumber);
void UnityAssertEqualFloatArray(const float* expected, const float* actual, const uint32_t num_elements, const char* msg, const unsigned short lineNumber);
void UnityAssertFloatsWithin(const float delta, const float expected, const float actual, const char* msg, const unsigned short lineNumber);
void UnityAssertEqualString(const char* expected, const char* actual, const char* msg, const unsigned short lineNumber);
void UnityAssertEqualStringLen(const char* expected, const char* actual, const uint32_t length, const char* msg, const unsigned short lineNumber);
void UnityAssertEqualStringArray(const char** expected, const char** actual, const uint32_t num_elements, const char* msg, const unsigned short lineNumber);
void UnityAssertEqualMemory(const void* expected, const void* actual, const uint32_t length, const char* msg, const unsigned short lineNumber);
void UnityAssertNumbersWithin(const long delta, const long expected, const long actual, const char* msg, const unsigned short lineNumber);

void UnityFail(const char* message, const int line);
void UnityIgnore(const char* message, const int line);

//-------------------------------------------------------
// Helpful Test Macros
//-------------------------------------------------------
#define UNITY_TEST_ASSERT_EQUAL_INT(expected, actual, line, message)                          UnityAssertEqualNumber((long)(expected), (long)(actual), (message), (unsigned short)(line), 1)
#define UNITY_TEST_ASSERT_EQUAL_UINT(expected, actual, line, message)                         UnityAssertEqualNumber((long)(expected), (long)(actual), (message), (unsigned short)(line), 2)
#define UNITY_TEST_ASSERT_NOT_EQUAL_INT(expected, actual, line, message)                      UnityAssertEqualNumber((long)(expected), (long)(actual), (message), (unsigned short)(line), 3)
#define UNITY_TEST_ASSERT_EQUAL_STRING(expected, actual, line, message)                       UnityAssertEqualString((const char*)(expected), (const char*)(actual), (message), (unsigned short)(line))
#define UNITY_TEST_ASSERT_EQUAL_MEMORY(expected, actual, len, line, message)                  UnityAssertEqualMemory((void*)(expected), (void*)(actual), (uint32_t)(len), (message), (unsigned short)(line))
#define UNITY_TEST_ASSERT_NULL(pointer, line, message)                                        UnityAssertEqualNumber((long)0, (long)(pointer), (message), (unsigned short)(line), 1)
#define UNITY_TEST_ASSERT_NOT_NULL(pointer, line, message)                                    UnityAssertEqualNumber((long)0, (long)(pointer), (message), (unsigned short)(line), 3)

#define TEST_ASSERT(condition)                                                                do {if (!(condition)) {UnityFail( #condition, __LINE__);}} while(0)
#define TEST_ASSERT_TRUE(condition)                                                           TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition)                                                          TEST_ASSERT(!(condition))
#define TEST_ASSERT_NULL(pointer)                                                             UNITY_TEST_ASSERT_NULL(pointer, __LINE__, NULL)
#define TEST_ASSERT_NOT_NULL(pointer)                                                         UNITY_TEST_ASSERT_NOT_NULL(pointer, __LINE__, NULL)
#define TEST_ASSERT_EQUAL_INT(expected, actual)                                               UNITY_TEST_ASSERT_EQUAL_INT(expected, actual, __LINE__, NULL)
#define TEST_ASSERT_EQUAL_UINT(expected, actual)                                              UNITY_TEST_ASSERT_EQUAL_UINT(expected, actual, __LINE__, NULL)
#define TEST_ASSERT_NOT_EQUAL_INT(expected, actual)                                           UNITY_TEST_ASSERT_NOT_EQUAL_INT(expected, actual, __LINE__, NULL)
#define TEST_ASSERT_EQUAL_STRING(expected, actual)                                            UNITY_TEST_ASSERT_EQUAL_STRING(expected, actual, __LINE__, NULL)
#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len)                                       UNITY_TEST_ASSERT_EQUAL_MEMORY(expected, actual, len, __LINE__, NULL)

#define TEST_FAIL(message)                                                                    UnityFail( (message), __LINE__)
#define TEST_IGNORE()                                                                         UnityIgnore( NULL, __LINE__)
#define TEST_IGNORE_MESSAGE(message)                                                          UnityIgnore( (message), __LINE__)

//-------------------------------------------------------
// Test Runner Macros
//-------------------------------------------------------
#define RUN_TEST(func)                                                                        UnityDefaultTestRun(func, #func, __LINE__)

#endif // UNITY_FRAMEWORK_H
