# Omnikarai v5.0 — Foundation Debugging Report
> **Status:** Pre-Beta  
> **Goal:** 100% Test Pass Rate & Stable x86-64 Foundation

This report details the four critical architectural bugs in the `omnicc` codegen that must be resolved to stabilize the language for AI workloads.

---

## 1. The "Upper-Bit Garbage" Comparison Flaw
*   **Symptom:** `while` loops skip or run indefinitely; `if` statements evaluate incorrectly.
*   **Root Cause:** The `setcc` instruction (used for `==`, `<`, `>`, etc.) only modifies the `AL` register (lowest 8 bits of `RAX`). The subsequent `test rax, rax` checks the full 64-bit register. If `RAX` contains leftover data in bits 8-63, the condition result is corrupted.
*   **Technical Detail (ASM):**
    ```asm
    cmp rax, rcx
    setl al         ; Only sets bits 0-7
    ; Error: bits 8-63 are still "garbage" from previous math
    test rax, rax   ; Checks all 64 bits!
    je loop_exit
    ```
*   **The Fix:** Immediately follow every `setcc` with a zero-extension:
    ```asm
    setl al
    movzx rax, al   ; Clears bits 8-63, making RAX exactly 0 or 1
    ```

---

## 2. Windows x64 ABI Shadow Space Violation
*   **Symptom:** Immediate crash (Access Violation `C0000005`) when calling modules (`time`, `math`, `os`).
*   **Root Cause:** The Windows x64 calling convention requires the caller to allocate **32 bytes of "Shadow Space"** (Home Space) on the stack before any `call`. The current codegen does not reserve this.
*   **Technical Detail:** When a Win32 function (like `WriteFile` or `GetSystemTime`) starts, it expects to be able to "spill" its register arguments (`RCX`, `RDX`, `R8`, `R9`) onto the stack at `[RSP+8]` to `[RSP+32]`. Without this space, the OS overwrites Omnikarai's return addresses or local variables.
*   **The Fix:** Wrap external calls with stack adjustment:
    ```asm
    sub rsp, 32    ; Reserve shadow space
    call <address>
    add rsp, 32    ; Clean up
    ```

---

## 3. Stack Alignment Corruption (The 16-Byte Rule)
*   **Symptom:** Random crashes during deep recursion or floating-point math.
*   **Root Cause:** The x86-64 ABI requires the stack pointer (`RSP`) to be **16-byte aligned** before any `call`.
*   **Technical Detail:** Every `push` moves `RSP` by 8 bytes. If a function has an odd number of local variables or arguments, `RSP` will end in `0x...8` instead of `0x...0`. Modern CPU instructions (SSE/AVX) will trigger a hardware exception if executed on a misaligned stack.
*   **The Fix:** The `cg_prologue` must calculate the total stack frame size and pad it:
    ```c
    int total_stack = cg->stack_size + 32; // locals + shadow space
    if (total_stack % 16 != 0) total_stack += 8; // Align to 16
    emit_sub_rsp(&cg->code, total_stack);
    ```

---

## 4. User Function Relative Jump Precision
*   **Symptom:** `t05_functions.ok` crashes on the first user-defined function call.
*   **Root Cause:** The displacement calculation for the `0xE8` (CALL) instruction is off by a few bytes.
*   **Technical Detail:** A relative call displacement is calculated from the *end* of the instruction. 
    `displacement = target_address - (current_instruction_pointer + 5)`.
    If the `+ 5` (size of the call instruction) is missing, the CPU jumps to the wrong byte, often in the middle of another instruction.
*   **The Fix:** Audit the `resolve_fwd` and `cg_call_user` functions to ensure the 5-byte instruction size is accounted for in all relative offsets.

---

## Next Steps for Beta
Once these four "Surgical Fixes" are applied:
1.  **Run full test suite:** Verify 15/15 PASS.
2.  **Enable Register Pinning:** Implement the "Ephemeral" logic.
3.  **Implement AVX2/FMA:** Move AI math from standard `imul` to 256-bit vectors.
