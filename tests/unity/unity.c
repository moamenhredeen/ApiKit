/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-14 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//-------------------------------------------------------
// Global Unity Structure
//-------------------------------------------------------
struct _Unity Unity;

//-------------------------------------------------------
// Unity Internal Functions
//-------------------------------------------------------

void UnityBegin(const char* filename)
{
    Unity.TestFile = filename;
    Unity.CurrentTestName = NULL;
    Unity.CurrentTestLineNumber = 0;
    Unity.NumberOfTests = 0;
    Unity.TestFailures = 0;
    Unity.TestIgnores = 0;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
    
    UNITY_OUTPUT_START();
}

int UnityEnd(void)
{
    UNITY_OUTPUT_CHAR('\n');
    if (Unity.TestFailures == 0U)
    {
        printf("OK (%u tests)\n", Unity.NumberOfTests);
    }
    else
    {
        printf("FAIL (%u failures, %u tests)\n", Unity.TestFailures, Unity.NumberOfTests);
    }
    UNITY_OUTPUT_COMPLETE();
    return (int)(Unity.TestFailures);
}

void UnityConcludeTest(void)
{
    if (Unity.CurrentTestIgnored)
    {
        Unity.TestIgnores++;
        UNITY_OUTPUT_CHAR('I');
    }
    else if (Unity.CurrentTestFailed)
    {
        Unity.TestFailures++;
        UNITY_OUTPUT_CHAR('F');
    }
    else
    {
        UNITY_OUTPUT_CHAR('.');
    }
    
    Unity.NumberOfTests++;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
    UNITY_OUTPUT_FLUSH();
}

void UnityDefaultTestRun(UnityTestFunction Func, const char* FuncName, const int FuncLineNum)
{
    Unity.CurrentTestName = FuncName;
    Unity.CurrentTestLineNumber = (uint32_t)FuncLineNum;
    Unity.NumberOfTests++;
    
    if (setjmp(Unity.AbortFrame) == 0)
    {
        setUp();
        Func();
    }
    tearDown();
    UnityConcludeTest();
}

//-------------------------------------------------------
// Assertion Functions
//-------------------------------------------------------

void UnityAssertEqualNumber(const long expected, const long actual, const char* msg, const unsigned short lineNumber, const char style)
{
    if ((style == 1 && expected == actual) ||
        (style == 2 && (unsigned long)expected == (unsigned long)actual) ||
        (style == 3 && expected != actual))
    {
        return;
    }
    
    Unity.CurrentTestFailed = 1;
    printf("\n%s:%d:%s: Expected %ld but was %ld", Unity.TestFile, lineNumber, Unity.CurrentTestName, expected, actual);
    if (msg != NULL)
    {
        printf(" (%s)", msg);
    }
    printf("\n");
    longjmp(Unity.AbortFrame, 1);
}

void UnityAssertEqualString(const char* expected, const char* actual, const char* msg, const unsigned short lineNumber)
{
    if (expected == actual) return; // Both NULL
    if (expected == NULL || actual == NULL)
    {
        Unity.CurrentTestFailed = 1;
        printf("\n%s:%d:%s: Expected '%s' but was '%s'", Unity.TestFile, lineNumber, Unity.CurrentTestName, 
               expected ? expected : "NULL", actual ? actual : "NULL");
        if (msg != NULL) printf(" (%s)", msg);
        printf("\n");
        longjmp(Unity.AbortFrame, 1);
    }
    
    if (strcmp(expected, actual) != 0)
    {
        Unity.CurrentTestFailed = 1;
        printf("\n%s:%d:%s: Expected '%s' but was '%s'", Unity.TestFile, lineNumber, Unity.CurrentTestName, expected, actual);
        if (msg != NULL) printf(" (%s)", msg);
        printf("\n");
        longjmp(Unity.AbortFrame, 1);
    }
}

void UnityAssertEqualMemory(const void* expected, const void* actual, const uint32_t length, const char* msg, const unsigned short lineNumber)
{
    if (expected == actual) return;
    if (expected == NULL || actual == NULL)
    {
        Unity.CurrentTestFailed = 1;
        printf("\n%s:%d:%s: Memory comparison failed - one pointer is NULL", Unity.TestFile, lineNumber, Unity.CurrentTestName);
        if (msg != NULL) printf(" (%s)", msg);
        printf("\n");
        longjmp(Unity.AbortFrame, 1);
    }
    
    if (memcmp(expected, actual, length) != 0)
    {
        Unity.CurrentTestFailed = 1;
        printf("\n%s:%d:%s: Memory comparison failed", Unity.TestFile, lineNumber, Unity.CurrentTestName);
        if (msg != NULL) printf(" (%s)", msg);
        printf("\n");
        longjmp(Unity.AbortFrame, 1);
    }
}

void UnityFail(const char* message, const int line)
{
    Unity.CurrentTestFailed = 1;
    printf("\n%s:%d:%s: %s", Unity.TestFile, line, Unity.CurrentTestName, message);
    printf("\n");
    longjmp(Unity.AbortFrame, 1);
}

void UnityIgnore(const char* message, const int line)
{
    Unity.CurrentTestIgnored = 1;
    printf("\n%s:%d:%s: IGNORED", Unity.TestFile, line, Unity.CurrentTestName);
    if (message != NULL) printf(" (%s)", message);
    printf("\n");
    longjmp(Unity.AbortFrame, 1);
}

// Stub implementations for optional functions
void UnityAssertEqualIntArray(const int* expected, const int* actual, const uint32_t num_elements, const char* msg, const unsigned short lineNumber) { (void)expected; (void)actual; (void)num_elements; (void)msg; (void)lineNumber; }
void UnityAssertEqualFloatArray(const float* expected, const float* actual, const uint32_t num_elements, const char* msg, const unsigned short lineNumber) { (void)expected; (void)actual; (void)num_elements; (void)msg; (void)lineNumber; }
void UnityAssertFloatsWithin(const float delta, const float expected, const float actual, const char* msg, const unsigned short lineNumber) { (void)delta; (void)expected; (void)actual; (void)msg; (void)lineNumber; }
void UnityAssertEqualStringLen(const char* expected, const char* actual, const uint32_t length, const char* msg, const unsigned short lineNumber) { (void)expected; (void)actual; (void)length; (void)msg; (void)lineNumber; }
void UnityAssertEqualStringArray(const char** expected, const char** actual, const uint32_t num_elements, const char* msg, const unsigned short lineNumber) { (void)expected; (void)actual; (void)num_elements; (void)msg; (void)lineNumber; }
void UnityAssertNumbersWithin(const long delta, const long expected, const long actual, const char* msg, const unsigned short lineNumber) { (void)delta; (void)expected; (void)actual; (void)msg; (void)lineNumber; }
