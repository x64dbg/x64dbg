#ifndef _SCRIPTAPI_MISC_H
#define _SCRIPTAPI_MISC_H

#include "_scriptapi.h"

namespace Script
{
    namespace Misc
    {
        /// <summary>
        /// Evaluates an expression and returns the result. Analagous to using the Command field in x64dbg.
        ///
        /// Expressions can consist of memory locations, registers, flags, API names, labels, symbols, variables etc.
        /// <example>
        /// Shows how to read from stack at esp+8
        /// <code>
        ///     bool success = ParseExpression("[esp+8]", &amp;val)
        /// </code>
        /// </example>
        /// </summary>
        /// <param name="expression">The expression to evaluate.</param>
        /// <param name="value">The result of the expression.</param>
        /// <returns>True on success, False on failure.</returns>
        SCRIPT_EXPORT bool ParseExpression(const char* expression, duint* value);

        /// <summary>
        /// Returns the address of a function in the debuggee's memory space.
        /// </summary>
        /// <example>
        /// <code>
        ///     duint addr = RemoteGetProcAddress("kernel32.dll", "GetProcAddress")
        /// </code>
        /// </example>
        /// <param name="module">The name of the module.</param>
        /// <param name="api">The name of the function.</param>
        /// <returns>The address of the function in the debuggee.</returns>
        SCRIPT_EXPORT duint RemoteGetProcAddress(const char* module, const char* api);

        /// <summary>
        /// Returns the address for a label created in the disassembly window.
        /// </summary>
        /// <example>
        /// <code>
        ///     duint addr = ResolveLabel("sneaky_crypto")
        /// </code>
        /// </example>
        /// <param name="label">The name of the label to resolve.</param>
        /// <returns>The memory address for the label.</returns>
        SCRIPT_EXPORT duint ResolveLabel(const char* label);

        /// <summary>
        /// Allocates the requested number of bytes from x64dbg's default process heap.
        ///
        /// Note: this allocation is in the debugger, not the debuggee.
        ///
        /// Memory allocated using this function should be Free'd after use.
        /// </summary>
        /// <example>
        /// <code>
        ///     void* addr = Alloc(0x100000)
        /// </code>
        /// </example>
        /// <param name="size">Number of bytes to allocate.</param>
        /// <returns>A pointer to the newly allocated memory.</returns>
        SCRIPT_EXPORT void* Alloc(duint size);

        /// <summary>
        /// Frees memory previously allocated by Alloc.
        /// </summary>
        /// <example>
        /// <code>
        ///     Free(addr)
        /// </code>
        /// </example>
        /// <param name="ptr">Pointer returned by Alloc.</param>
        /// <returns>Nothing.</returns>
        SCRIPT_EXPORT void Free(void* ptr);
    }; //Misc
}; //Script

#endif //_SCRIPTAPI_MISC_H