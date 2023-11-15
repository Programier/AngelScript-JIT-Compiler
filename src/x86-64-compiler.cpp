// MIT License

// Copyright (c) 2023 Programier (Vladyslav Reminskyi)

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <x86-64/compiler.hpp>

#define WITH_LOG 1
#define SHOW_JIT_ENTRY 1

#if WITH_LOG
#define logf printf
#else
#define logf
#endif

#define STDCALL_DECL

#define RETURN_CONTROL_TO_VM() return exec_asBC_RET(info)
#define NEED_IMPLEMENTATION()                                                                                          \
    printf("Function %s need implementaion!\n", __FUNCTION__);                                                         \
    RETURN_CONTROL_TO_VM()
#define CHECK_IT() printf("Function %s marked for check\n", __FUNCTION__)
#define MAYBE_UNUSED __attribute__((unused))

#define arg_value_dword(offset) asBC_DWORDARG(info->address + offset)
#define arg_value_int() asBC_INTARG(info->address)
#define arg_value_qword() asBC_QWORDARG(info->address)
#define arg_value_float(offset) asBC_FLOATARG(info->address + offset)
#define arg_value_ptr() asBC_PTRARG(info->address)
#define arg_value_word(index) (*(((asWORD*) info->address) + index + 1))
#define arg_value_short(index) (*(((short*) info->address) + index + 1))
#define arg_offset(index) (-(*(((short*) info->address) + index + 1)) * sizeof(asDWORD))
#define new_instruction(x) catch_errors(info->assembler.x)


#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#else
#define PLATFORM_WINDOWS 0
#endif

#if defined(__linux__)
#define PLATFORM_LINUX 1
#else
#define PLATFORM_LINUX 0
#endif

#if !PLATFORM_WINDOWS && !PLATFORM_LINUX
#define PLATFORM_DEFAULT 1
#else
#define PLATFORM_DEFAULT 0
#endif

namespace JIT
{
    static constexpr inline int32_t half_ptr_size      = static_cast<int32_t>(sizeof(void*) / 2);
    static constexpr inline int32_t ptr_size_1         = static_cast<int32_t>(sizeof(void*) * 1);
    static constexpr inline int32_t vm_register_offset = -ptr_size_1;

    static constexpr inline size_t const_pool_size = 64;

#if PLATFORM_LINUX || PLATFORM_DEFAULT
    static constexpr inline Gpq stack_pointer MAYBE_UNUSED = rsp;
    static constexpr inline Gpq base_pointer MAYBE_UNUSED  = rbp;

    static constexpr inline Gpq qword_free_1 MAYBE_UNUSED = rax;
    static constexpr inline Gpq qword_free_2 MAYBE_UNUSED = rbx;
    static constexpr inline Gpq qword_free_3 MAYBE_UNUSED = r14;

    static constexpr inline Gpd dword_free_1 MAYBE_UNUSED = eax;
    static constexpr inline Gpd dword_free_2 MAYBE_UNUSED = ebx;
    static constexpr inline Gpd dword_free_3 MAYBE_UNUSED = r14d;

    static constexpr inline Gpw word_free_1 MAYBE_UNUSED = ax;
    static constexpr inline Gpw word_free_2 MAYBE_UNUSED = bx;
    static constexpr inline Gpw word_free_3 MAYBE_UNUSED = r14w;

    static constexpr inline Gpb byte_free_1 MAYBE_UNUSED = al;
    static constexpr inline Gpb byte_free_2 MAYBE_UNUSED = bl;
    static constexpr inline Gpb byte_free_3 MAYBE_UNUSED = r14b;

    static constexpr inline Xmm xmm_free_1 MAYBE_UNUSED = xmm0;
    static constexpr inline Xmm xmm_free_2 MAYBE_UNUSED = xmm1;

    static constexpr inline Gpd dword_div_first_arg MAYBE_UNUSED  = eax;
    static constexpr inline Gpd dword_div_mod_result MAYBE_UNUSED = edx;
    static constexpr inline Gpq qword_div_first_arg MAYBE_UNUSED  = rax;
    static constexpr inline Gpq qword_div_mod_result MAYBE_UNUSED = rdx;

    static constexpr inline Gpq qword_first_arg MAYBE_UNUSED  = rdi;
    static constexpr inline Gpq qword_second_arg MAYBE_UNUSED = rsi;
    static constexpr inline Gpq qword_third_arg MAYBE_UNUSED  = rdx;
    static constexpr inline Gpd dword_firts_arg MAYBE_UNUSED  = edi;
    static constexpr inline Gpd dword_second_arg MAYBE_UNUSED = esi;
    static constexpr inline Gpd dword_third_arg MAYBE_UNUSED  = edx;

    static constexpr inline Xmm float_firts_arg MAYBE_UNUSED   = xmm0;
    static constexpr inline Xmm float_second_arg MAYBE_UNUSED  = xmm1;
    static constexpr inline Xmm double_firts_arg MAYBE_UNUSED  = xmm0;
    static constexpr inline Xmm double_second_arg MAYBE_UNUSED = xmm1;

    static constexpr inline Gpd dword_return MAYBE_UNUSED  = eax;
    static constexpr inline Gpq qword_return MAYBE_UNUSED  = rax;
    static constexpr inline Xmm float_return MAYBE_UNUSED  = xmm0;
    static constexpr inline Xmm double_return MAYBE_UNUSED = xmm0;

    static constexpr inline Gpb shift_second_arg = cl;

    static constexpr inline Gpq restore_register = r13;

    static constexpr inline Gpq vm_stack_frame_pointer MAYBE_UNUSED = r8;
    static constexpr inline Gpq vm_stack_pointer MAYBE_UNUSED       = r9;
    static constexpr inline Gpq vm_value_q MAYBE_UNUSED             = r10;
    static constexpr inline Gpd vm_value_d MAYBE_UNUSED             = r10d;
    static constexpr inline Gpw vm_value_w MAYBE_UNUSED             = r10w;
    static constexpr inline Gpb vm_value_b MAYBE_UNUSED             = r10b;
    static constexpr inline Gpq vm_object MAYBE_UNUSED              = r11;
    static constexpr inline Gpq vm_object_type MAYBE_UNUSED         = r12;


#elif PLATFORM_WINDOWS
    static constexpr inline Gpq stack_pointer MAYBE_UNUSED = rsp;
    static constexpr inline Gpq base_pointer MAYBE_UNUSED  = rbp;

    static constexpr inline Gpq qword_free_1 MAYBE_UNUSED = rax;
    static constexpr inline Gpq qword_free_2 MAYBE_UNUSED = rbx;
    static constexpr inline Gpq qword_free_3 MAYBE_UNUSED = r14;

    static constexpr inline Gpd dword_free_1 MAYBE_UNUSED = eax;
    static constexpr inline Gpd dword_free_2 MAYBE_UNUSED = ebx;
    static constexpr inline Gpd dword_free_3 MAYBE_UNUSED = r14d;

    static constexpr inline Gpw word_free_1 MAYBE_UNUSED = ax;
    static constexpr inline Gpw word_free_2 MAYBE_UNUSED = bx;
    static constexpr inline Gpw word_free_3 MAYBE_UNUSED = r14w;

    static constexpr inline Gpb byte_free_1 MAYBE_UNUSED = al;
    static constexpr inline Gpb byte_free_2 MAYBE_UNUSED = bl;
    static constexpr inline Gpb byte_free_3 MAYBE_UNUSED = r14b;

    static constexpr inline Xmm xmm_free_1 MAYBE_UNUSED = xmm0;
    static constexpr inline Xmm xmm_free_2 MAYBE_UNUSED = xmm1;

    static constexpr inline Gpd dword_div_first_arg MAYBE_UNUSED  = eax;
    static constexpr inline Gpd dword_div_mod_result MAYBE_UNUSED = edx;
    static constexpr inline Gpq qword_div_first_arg MAYBE_UNUSED  = rax;
    static constexpr inline Gpq qword_div_mod_result MAYBE_UNUSED = rdx;

    static constexpr inline Gpq qword_first_arg MAYBE_UNUSED  = rcx;
    static constexpr inline Gpq qword_second_arg MAYBE_UNUSED = rdx;
    static constexpr inline Gpq qword_third_arg MAYBE_UNUSED  = r8;
    static constexpr inline Gpd dword_firts_arg MAYBE_UNUSED  = ecx;
    static constexpr inline Gpd dword_second_arg MAYBE_UNUSED = edx;
    static constexpr inline Gpd dword_third_arg MAYBE_UNUSED  = r8d;

    static constexpr inline Xmm float_firts_arg MAYBE_UNUSED   = xmm0;
    static constexpr inline Xmm float_second_arg MAYBE_UNUSED  = xmm1;
    static constexpr inline Xmm double_firts_arg MAYBE_UNUSED  = xmm0;
    static constexpr inline Xmm double_second_arg MAYBE_UNUSED = xmm1;

    static constexpr inline Gpd dword_return MAYBE_UNUSED  = eax;
    static constexpr inline Gpq qword_return MAYBE_UNUSED  = rax;
    static constexpr inline Xmm float_return MAYBE_UNUSED  = xmm0;
    static constexpr inline Xmm double_return MAYBE_UNUSED = xmm0;

    static constexpr inline Gpb shift_second_arg = cl;

    static constexpr inline Gpq restore_register = r13;

    static constexpr inline Gpq vm_stack_frame_pointer MAYBE_UNUSED = r8;
    static constexpr inline Gpq vm_stack_pointer MAYBE_UNUSED       = r9;
    static constexpr inline Gpq vm_value_q MAYBE_UNUSED             = r10;
    static constexpr inline Gpd vm_value_d MAYBE_UNUSED             = r10d;
    static constexpr inline Gpw vm_value_w MAYBE_UNUSED             = r10w;
    static constexpr inline Gpb vm_value_b MAYBE_UNUSED             = r10b;
    static constexpr inline Gpq vm_object MAYBE_UNUSED              = r11;
    static constexpr inline Gpq vm_object_type MAYBE_UNUSED         = r12;
#endif


    static float STDCALL_DECL mod_float(float a, float b)
    {
        return fmodf(a, b);
    }

    static float STDCALL_DECL mod_double(double a, double b)
    {
        return std::fmod(a, b);
    }

    static int32_t STDCALL_DECL ipow(int32_t a, int32_t b)
    {
        return std::pow<int32_t, int32_t>(a, b);
    }

    static uint32_t STDCALL_DECL upow(uint32_t a, uint32_t b)
    {
        return std::pow<uint32_t, uint32_t>(a, b);
    }

    static int64_t STDCALL_DECL i64pow(int64_t a, int64_t b)
    {
        return std::pow<int64_t, int64_t>(a, b);
    }

    static uint64_t STDCALL_DECL u64pow(uint64_t a, uint64_t b)
    {
        return std::pow<uint64_t, uint64_t>(a, b);
    }

    static float STDCALL_DECL fpow(float a, float b)
    {
        return powf(a, b);
    }

    static double STDCALL_DECL dpow(double a, double b)
    {
        return std::pow(a, b);
    }

    static double STDCALL_DECL dipow(double a, int b)
    {
        return std::pow<double, int>(a, b);
    }

    static void STDCALL_DECL make_exception_nullptr_access()
    {
        throw std::runtime_error("Attempting to access a null pointer");
    }

    ////////////////////// MUST BE REMOVED IN FUTURE! //////////////////////
    static double STDCALL_DECL uint_to_double(uint32_t value)
    {
        return static_cast<double>(value);
    }

    static double STDCALL_DECL uint64_to_double(uint64_t value)
    {
        return static_cast<double>(value);
    }

    static float STDCALL_DECL uint_to_float(uint32_t value)
    {
        return static_cast<float>(value);
    }


    static float STDCALL_DECL uint64_to_float(uint64_t value)
    {
        return static_cast<float>(value);
    }


    static uint64_t STDCALL_DECL double_to_uint64(double value)
    {
        return static_cast<uint64_t>(value);
    }

    static uint64_t STDCALL_DECL float_to_uint64(float value)
    {
        return static_cast<uint64_t>(value);
    }

    ////////////////////// MUST BE REMOVED IN FUTURE! //////////////////////

    static void catch_errors(asmjit::Error error)
    {
        if (error != 0)
        {
            printf("AsmJit failed: %s\n", JIT::DebugUtils::errorAsString(error));
        }
    }

    X86_64_Compiler::X86_64_Compiler(bool with_suspend) : _M_with_suspend(with_suspend)
    {
#define register_code(name)                                                                                            \
    exec[static_cast<size_t>(name)]       = &X86_64_Compiler::exec_##name;                                             \
    code_names[static_cast<size_t>(name)] = #name;

        register_code(asBC_PopPtr);
        register_code(asBC_PshGPtr);
        register_code(asBC_PshC4);
        register_code(asBC_PshV4);
        register_code(asBC_PSF);
        register_code(asBC_SwapPtr);
        register_code(asBC_NOT);
        register_code(asBC_PshG4);
        register_code(asBC_LdGRdR4);
        register_code(asBC_CALL);
        register_code(asBC_RET);
        register_code(asBC_JMP);
        register_code(asBC_JZ);
        register_code(asBC_JNZ);
        register_code(asBC_JS);
        register_code(asBC_JNS);
        register_code(asBC_JP);
        register_code(asBC_JNP);
        register_code(asBC_TZ);
        register_code(asBC_TNZ);
        register_code(asBC_TS);
        register_code(asBC_TNS);
        register_code(asBC_TP);
        register_code(asBC_TNP);
        register_code(asBC_NEGi);
        register_code(asBC_NEGf);
        register_code(asBC_NEGd);
        register_code(asBC_INCi16);
        register_code(asBC_INCi8);
        register_code(asBC_DECi16);
        register_code(asBC_DECi8);
        register_code(asBC_INCi);
        register_code(asBC_DECi);
        register_code(asBC_INCf);
        register_code(asBC_DECf);
        register_code(asBC_INCd);
        register_code(asBC_DECd);
        register_code(asBC_IncVi);
        register_code(asBC_DecVi);
        register_code(asBC_BNOT);
        register_code(asBC_BAND);
        register_code(asBC_BOR);
        register_code(asBC_BXOR);
        register_code(asBC_BSLL);
        register_code(asBC_BSRL);
        register_code(asBC_BSRA);
        register_code(asBC_COPY);
        register_code(asBC_PshC8);
        register_code(asBC_PshVPtr);
        register_code(asBC_RDSPtr);
        register_code(asBC_CMPd);
        register_code(asBC_CMPu);
        register_code(asBC_CMPf);
        register_code(asBC_CMPi);
        register_code(asBC_CMPIi);
        register_code(asBC_CMPIf);
        register_code(asBC_CMPIu);
        register_code(asBC_JMPP);
        register_code(asBC_PopRPtr);
        register_code(asBC_PshRPtr);
        register_code(asBC_STR);
        register_code(asBC_CALLSYS);
        register_code(asBC_CALLBND);
        register_code(asBC_SUSPEND);
        register_code(asBC_ALLOC);
        register_code(asBC_FREE);
        register_code(asBC_LOADOBJ);
        register_code(asBC_STOREOBJ);
        register_code(asBC_GETOBJ);
        register_code(asBC_REFCPY);
        register_code(asBC_CHKREF);
        register_code(asBC_GETOBJREF);
        register_code(asBC_GETREF);
        register_code(asBC_PshNull);
        register_code(asBC_ClrVPtr);
        register_code(asBC_OBJTYPE);
        register_code(asBC_TYPEID);
        register_code(asBC_SetV4);
        register_code(asBC_SetV8);
        register_code(asBC_ADDSi);
        register_code(asBC_CpyVtoV4);
        register_code(asBC_CpyVtoV8);
        register_code(asBC_CpyVtoR4);
        register_code(asBC_CpyVtoR8);
        register_code(asBC_CpyVtoG4);
        register_code(asBC_CpyRtoV4);
        register_code(asBC_CpyRtoV8);
        register_code(asBC_CpyGtoV4);
        register_code(asBC_WRTV1);
        register_code(asBC_WRTV2);
        register_code(asBC_WRTV4);
        register_code(asBC_WRTV8);
        register_code(asBC_RDR1);
        register_code(asBC_RDR2);
        register_code(asBC_RDR4);
        register_code(asBC_RDR8);
        register_code(asBC_LDG);
        register_code(asBC_LDV);
        register_code(asBC_PGA);
        register_code(asBC_CmpPtr);
        register_code(asBC_VAR);
        register_code(asBC_iTOf);
        register_code(asBC_fTOi);
        register_code(asBC_uTOf);
        register_code(asBC_fTOu);
        register_code(asBC_sbTOi);
        register_code(asBC_swTOi);
        register_code(asBC_ubTOi);
        register_code(asBC_uwTOi);
        register_code(asBC_dTOi);
        register_code(asBC_dTOu);
        register_code(asBC_dTOf);
        register_code(asBC_iTOd);
        register_code(asBC_uTOd);
        register_code(asBC_fTOd);
        register_code(asBC_ADDi);
        register_code(asBC_SUBi);
        register_code(asBC_MULi);
        register_code(asBC_DIVi);
        register_code(asBC_MODi);
        register_code(asBC_ADDf);
        register_code(asBC_SUBf);
        register_code(asBC_MULf);
        register_code(asBC_DIVf);
        register_code(asBC_MODf);
        register_code(asBC_ADDd);
        register_code(asBC_SUBd);
        register_code(asBC_MULd);
        register_code(asBC_DIVd);
        register_code(asBC_MODd);
        register_code(asBC_ADDIi);
        register_code(asBC_SUBIi);
        register_code(asBC_MULIi);
        register_code(asBC_ADDIf);
        register_code(asBC_SUBIf);
        register_code(asBC_MULIf);
        register_code(asBC_SetG4);
        register_code(asBC_ChkRefS);
        register_code(asBC_ChkNullV);
        register_code(asBC_CALLINTF);
        register_code(asBC_iTOb);
        register_code(asBC_iTOw);
        register_code(asBC_SetV1);
        register_code(asBC_SetV2);
        register_code(asBC_Cast);
        register_code(asBC_i64TOi);
        register_code(asBC_uTOi64);
        register_code(asBC_iTOi64);
        register_code(asBC_fTOi64);
        register_code(asBC_dTOi64);
        register_code(asBC_fTOu64);
        register_code(asBC_dTOu64);
        register_code(asBC_i64TOf);
        register_code(asBC_u64TOf);
        register_code(asBC_i64TOd);
        register_code(asBC_u64TOd);
        register_code(asBC_NEGi64);
        register_code(asBC_INCi64);
        register_code(asBC_DECi64);
        register_code(asBC_BNOT64);
        register_code(asBC_ADDi64);
        register_code(asBC_SUBi64);
        register_code(asBC_MULi64);
        register_code(asBC_DIVi64);
        register_code(asBC_MODi64);
        register_code(asBC_BAND64);
        register_code(asBC_BOR64);
        register_code(asBC_BXOR64);
        register_code(asBC_BSLL64);
        register_code(asBC_BSRL64);
        register_code(asBC_BSRA64);
        register_code(asBC_CMPi64);
        register_code(asBC_CMPu64);
        register_code(asBC_ChkNullS);
        register_code(asBC_ClrHi);
        register_code(asBC_JitEntry);
        register_code(asBC_CallPtr);
        register_code(asBC_FuncPtr);
        register_code(asBC_LoadThisR);
        register_code(asBC_PshV8);
        register_code(asBC_DIVu);
        register_code(asBC_MODu);
        register_code(asBC_DIVu64);
        register_code(asBC_MODu64);
        register_code(asBC_LoadRObjR);
        register_code(asBC_LoadVObjR);
        register_code(asBC_RefCpyV);
        register_code(asBC_JLowZ);
        register_code(asBC_JLowNZ);
        register_code(asBC_AllocMem);
        register_code(asBC_SetListSize);
        register_code(asBC_PshListElmnt);
        register_code(asBC_SetListType);
        register_code(asBC_POWi);
        register_code(asBC_POWu);
        register_code(asBC_POWf);
        register_code(asBC_POWd);
        register_code(asBC_POWdi);
        register_code(asBC_POWi64);
        register_code(asBC_POWu64);
        register_code(asBC_Thiscall1);
#undef register_code
    }


    int X86_64_Compiler::CompileFunction(asIScriptFunction* function, asJITFunction* output)
    {
        if (std::strstr(function->GetName(), "nojit") != nullptr)
            return -1;


        logf("=================================================================\nBegin compile function '%s'\n",
             function->GetName());

        CompileInfo info;
        info.address = info.begin = function->GetByteCode(&info.byte_codes);
        if (info.begin == nullptr || info.byte_codes == 0)
            return -1;

        info.end = info.begin + info.byte_codes;

        CodeHolder code;
        code.init(_M_rt.environment(), _M_rt.cpuFeatures());
        new (&info.assembler) Assembler(&code);

        init(&info);
        info.address = info.begin;

        Zone zone(const_pool_size);
        ConstPool const_pool(&zone);

        Label const_pool_label = info.assembler.newLabel();
        info.const_pool_label  = &const_pool_label;
        info.const_pool        = &const_pool;


        unsigned int index              = 0;
        std::set<unsigned int>& skip_it = _M_skip_instructions[function->GetName()];

        while (info.address < info.end)
        {
            index++;
            info.instruction = static_cast<asEBCInstr>(*reinterpret_cast<asBYTE*>(info.address));

            if (skip_it.contains(index))
            {
                exec_asBC_RET(&info);
                info.address += instruction_size(info.instruction);
            }
            else
            {
                logf("%3d: ", index);
                info.address += process_instruction(&info);
            }
        }

        info.assembler.embedConstPool(const_pool_label, const_pool);

        info.assembler.finalize();
        _M_rt.add(output, &code);

        logf("End of compile function '%s'\n=================================================================\n\n",
             function->GetName());
        return 0;
    }

    void X86_64_Compiler::ReleaseJITFunction(asJITFunction func)
    {
        _M_rt.release(func);
    }

    void X86_64_Compiler::push_instruction_index_for_skip(const std::string& name, unsigned int index)
    {
        _M_skip_instructions[name].insert(index);
    }

    asUINT X86_64_Compiler::process_instruction(CompileInfo* info)
    {
        bind_label_if_required(info);
        size_t index = static_cast<size_t>(info->instruction);

#if WITH_LOG
        auto current_offset = info->assembler.offset();
#endif


        ((*this).*exec[index])(info);

#if WITH_LOG
        {
            auto size           = instruction_size(info->instruction);
            bool is_implemented = current_offset != info->assembler.offset();

            if (info->instruction == asBC_JitEntry || info->instruction == asBC_SUSPEND ||
                info->instruction == asBC_iTOb)
            {
                is_implemented = true;
            }

#if !SHOW_JIT_ENTRY
            if (info->instruction != asBC_JitEntry && info->instruction != asBC_SUSPEND)
#endif
            {
                logf("%-15s (%3zu with size = %u, %15s) -> [", code_names[index], index, size,
                     (is_implemented ? "IMPLEMENTED" : "NOT IMPLEMENTED"));

                for (decltype(size) i = 1; i < size; i++)
                {
                    logf("%u%s", static_cast<unsigned int>(info->address[i]), (i == size - 1 ? "" : ", "));
                }
                printf("]\n");
            }
        }
#endif

        return instruction_size(info->instruction);
    }


    void X86_64_Compiler::init(CompileInfo* info)
    {
        new_instruction(push(base_pointer));
        new_instruction(mov(base_pointer, stack_pointer));
        new_instruction(sub(stack_pointer, -vm_register_offset));

        new_instruction(mov(qword_ptr(base_pointer, vm_register_offset), qword_first_arg));
        restore_registers(info);

        // Restore position of execution
        new_instruction(lea(qword_free_1, qword_ptr(rip)));
        info->header_size = info->assembler.offset();
        new_instruction(add(qword_free_1, qword_second_arg));
        new_instruction(jmp(qword_free_1));

        asDWORD* start = info->begin;
        asDWORD* end   = info->end;

        while (start < end)
        {
            asEBCInstr op = asEBCInstr(*(asBYTE*) start);
            switch (op)
            {
                case asBC_JMP:
                case asBC_JLowZ:
                case asBC_JZ:
                case asBC_JLowNZ:
                case asBC_JNZ:
                case asBC_JS:
                case asBC_JNS:
                case asBC_JP:
                case asBC_JNP:
                {
                    info->labels.emplace_back();
                    info->labels.back().byte_code_address = start + asBC_INTARG(start) + instruction_size(op);
                    info->labels.back().label             = info->assembler.newLabel();
                    break;
                }


                default:
                    break;
            }
            start += instruction_size(op);
        }
    }

    void X86_64_Compiler::restore_registers(CompileInfo* info)
    {
        new_instruction(mov(restore_register, qword_ptr(base_pointer, vm_register_offset)));

        new_instruction(
                mov(vm_stack_frame_pointer, qword_ptr(restore_register, offsetof(asSVMRegisters, stackFramePointer))));
        new_instruction(mov(vm_stack_pointer, qword_ptr(restore_register, offsetof(asSVMRegisters, stackPointer))));
        new_instruction(mov(vm_value_q, qword_ptr(restore_register, offsetof(asSVMRegisters, valueRegister))));
        new_instruction(mov(vm_object, qword_ptr(restore_register, offsetof(asSVMRegisters, objectRegister))));
        new_instruction(mov(vm_object_type, qword_ptr(restore_register, offsetof(asSVMRegisters, objectType))));
    }

    void X86_64_Compiler::save_registers(CompileInfo* info, bool ret)
    {
        new_instruction(mov(restore_register, qword_ptr(base_pointer, vm_register_offset)));
        if (ret)
        {
            new_instruction(movabs(qword_free_1, info->address));
            new_instruction(mov(qword_ptr(restore_register, offsetof(asSVMRegisters, programPointer)), qword_free_1));
        }

        new_instruction(
                mov(qword_ptr(restore_register, offsetof(asSVMRegisters, stackFramePointer)), vm_stack_frame_pointer));
        new_instruction(mov(qword_ptr(restore_register, offsetof(asSVMRegisters, stackPointer)), vm_stack_pointer));
        new_instruction(mov(qword_ptr(restore_register, offsetof(asSVMRegisters, valueRegister)), vm_value_q));
        new_instruction(mov(qword_ptr(restore_register, offsetof(asSVMRegisters, objectRegister)), vm_object));
        new_instruction(mov(qword_ptr(restore_register, offsetof(asSVMRegisters, objectType)), vm_object_type));
    }

    void X86_64_Compiler::bind_label_if_required(CompileInfo* info)
    {
        for (LabelInfo& label_info : info->labels)
        {
            if (label_info.byte_code_address == info->address)
            {
                info->assembler.bind(label_info.label);
                return;
            }
        }
    }

    size_t X86_64_Compiler::find_label_for_jump(CompileInfo* info)
    {
        asDWORD* address = info->address + asBC_INTARG(info->address) + instruction_size(info->instruction);

        size_t index = 0;
        for (; index < info->labels.size(); index++)
        {
            if (info->labels[index].byte_code_address == address)
                return index;
        }

        throw std::runtime_error("Undefined label");
    }

    asUINT X86_64_Compiler::instruction_size(asEBCInstr instruction)
    {
        asUINT size = asBCTypeSize[asBCInfo[instruction].type];
        return size;
    }

    ///////////////////////////////////// IMPLEMENTATION OF INSTRUCTIONS /////////////////////////////////////

    void X86_64_Compiler::exec_asBC_PopPtr(CompileInfo* info)
    {
        new_instruction(add(vm_stack_pointer, ptr_size_1));
    }

    void X86_64_Compiler::exec_asBC_PshGPtr(CompileInfo* info)
    {
        asPWORD value = *(asPWORD*) asBC_PTRARG(info->address);

        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(movabs(qword_free_1, value));
        new_instruction(mov(dword_ptr(vm_stack_pointer), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_PshC4(CompileInfo* info)
    {
        new_instruction(sub(vm_stack_pointer, half_ptr_size));
        new_instruction(mov(dword_ptr(vm_stack_pointer), arg_value_dword(0)));
    }

    void X86_64_Compiler::exec_asBC_PshV4(CompileInfo* info)
    {
        new_instruction(sub(vm_stack_pointer, half_ptr_size));
        short offset = arg_offset(0);
        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset)));
        new_instruction(mov(dword_ptr(vm_stack_pointer), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_PSF(CompileInfo* info)
    {
        short offset = arg_offset(0);

        new_instruction(sub(vm_stack_pointer, ptr_size_1));

        new_instruction(mov(qword_free_1, vm_stack_frame_pointer));
        new_instruction(add(qword_free_1, offset));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_SwapPtr(CompileInfo* info)
    {
        CHECK_IT();
        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer)));
        new_instruction(mov(qword_free_2, qword_ptr(vm_stack_frame_pointer, ptr_size_1)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer), qword_free_2));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, ptr_size_1), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_NOT(CompileInfo* info)
    {
        Label not_equal = info->assembler.newLabel();
        Label end       = info->assembler.newLabel();


        new_instruction(cmp(vm_value_q, 0));
        new_instruction(jne(not_equal));
        new_instruction(mov(vm_value_q, 1));
        new_instruction(jmp(end));

        new_instruction(bind(not_equal));
        new_instruction(mov(vm_value_q, 0));

        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_PshG4(CompileInfo* info)
    {
        CHECK_IT();
        asDWORD value = *(asDWORD*) arg_value_ptr();

        new_instruction(sub(vm_stack_pointer, half_ptr_size));
        new_instruction(mov(dword_free_1, value));
        new_instruction(mov(dword_ptr(vm_stack_pointer), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_LdGRdR4(CompileInfo* info)
    {
        CHECK_IT();
        short offset = arg_offset(0);
        new_instruction(mov(vm_value_q, arg_value_ptr()));
        new_instruction(mov(dword_free_1, dword_ptr(vm_value_q)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_CALL(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_RET(CompileInfo* info)
    {
        save_registers(info, true);
        new_instruction(nop());
        new_instruction(leave());
        new_instruction(ret());
    }

    void X86_64_Compiler::exec_asBC_JMP(CompileInfo* info)
    {
        LabelInfo& label_info = info->labels[find_label_for_jump(info)];
        new_instruction(jmp(label_info.label));
    }

    void X86_64_Compiler::exec_asBC_JZ(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(vm_value_d, 0));
        new_instruction(je(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JNZ(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(vm_value_d, 0));
        new_instruction(jne(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JS(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(vm_value_d, 0));
        new_instruction(jl(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JNS(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(vm_value_d, 0));
        new_instruction(jge(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JP(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(vm_value_d, 0));
        new_instruction(jg(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JNP(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(vm_value_d, 0));
        new_instruction(jle(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_TZ(CompileInfo* info)
    {
        CHECK_IT();
        Label is_zero = info->assembler.newLabel();
        Label end     = info->assembler.newLabel();


        new_instruction(cmp(vm_value_d, 0));
        new_instruction(je(is_zero));
        new_instruction(mov(vm_value_q, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_zero));
        new_instruction(mov(vm_value_q, 1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_TNZ(CompileInfo* info)
    {
        // If the value in the register is not 0, then set the register to 1, else to 0
        CHECK_IT();
        Label is_zero = info->assembler.newLabel();
        Label end     = info->assembler.newLabel();

        new_instruction(cmp(vm_value_q, 0));
        new_instruction(je(is_zero));
        new_instruction(mov(vm_value_q, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_zero));
        new_instruction(mov(vm_value_q, 0));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_TS(CompileInfo* info)
    {
        // If the value in the register is negative, then set the register to 1, else to 0
        CHECK_IT();
        Label is_negative = info->assembler.newLabel();
        Label end         = info->assembler.newLabel();

        new_instruction(cmp(vm_value_d, 0));
        new_instruction(jl(is_negative));
        new_instruction(mov(vm_value_q, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_negative));
        new_instruction(mov(vm_value_q, 1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_TNS(CompileInfo* info)
    {
        // If the value in the register is not negative, then set the register to 1, else to 0
        CHECK_IT();
        Label is_negative = info->assembler.newLabel();
        Label end         = info->assembler.newLabel();

        new_instruction(cmp(vm_value_d, 0));
        new_instruction(jl(is_negative));
        new_instruction(mov(vm_value_q, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_negative));
        new_instruction(mov(vm_value_q, 0));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_TP(CompileInfo* info)
    {
        // If the value in the register is not negative, then set the register to 1, else to 0
        CHECK_IT();
        Label is_negative = info->assembler.newLabel();
        Label end         = info->assembler.newLabel();

        new_instruction(cmp(vm_value_d, 0));
        new_instruction(jl(is_negative));
        new_instruction(mov(vm_value_q, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_negative));
        new_instruction(mov(vm_value_q, 0));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_TNP(CompileInfo* info)
    {
        // If the value in the register is not greater than 0, then set the register to 1, else to 0
        CHECK_IT();
        Label is_le = info->assembler.newLabel();
        Label end   = info->assembler.newLabel();

        new_instruction(cmp(vm_value_d, 0));
        new_instruction(jle(is_le));
        new_instruction(mov(vm_value_q, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_le));
        new_instruction(mov(vm_value_q, 1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_NEGi(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(neg(dword_ptr(vm_stack_frame_pointer, offset)));
    }

    void X86_64_Compiler::exec_asBC_NEGf(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset)));
        new_instruction(xorps(xmm_free_1, info->insert_constant<int64_t>(-2147483648)));
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_NEGd(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(movsd(xmm_free_1, qword_ptr(vm_stack_frame_pointer, offset)));
        new_instruction(movq(xmm_free_2, info->insert_constant<int64_t>(-2147483648 << 32)));
        new_instruction(xorpd(xmm_free_1, xmm_free_2));
        new_instruction(movsd(qword_ptr(vm_stack_frame_pointer, offset), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_INCi16(CompileInfo* info)
    {
        new_instruction(inc(word_ptr(vm_value_w)));
    }

    void X86_64_Compiler::exec_asBC_INCi8(CompileInfo* info)
    {
        new_instruction(inc(byte_ptr(vm_value_q)));
    }

    void X86_64_Compiler::exec_asBC_DECi16(CompileInfo* info)
    {
        new_instruction(dec(word_ptr(vm_value_q)));
    }

    void X86_64_Compiler::exec_asBC_DECi8(CompileInfo* info)
    {
        new_instruction(dec(byte_ptr(vm_value_q)));
    }

    void X86_64_Compiler::exec_asBC_INCi(CompileInfo* info)
    {
        new_instruction(inc(dword_ptr(vm_value_q)));
    }

    void X86_64_Compiler::exec_asBC_DECi(CompileInfo* info)
    {
        new_instruction(dec(dword_ptr(vm_value_q)));
    }

    void X86_64_Compiler::exec_asBC_INCf(CompileInfo* info)
    {
        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(movss(xmm_free_1, qword_ptr(vm_value_q)));
        new_instruction(addss(xmm_free_1, info->insert_constant<float>(1.0)));
        new_instruction(movss(qword_ptr(vm_value_q), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_DECf(CompileInfo* info)
    {
        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(movss(xmm_free_1, qword_ptr(vm_value_q)));
        new_instruction(subss(xmm_free_1, info->insert_constant<float>(1.0)));
        new_instruction(movss(qword_ptr(vm_value_q), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_INCd(CompileInfo* info)
    {
        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(movsd(xmm_free_1, qword_ptr(vm_value_q)));
        new_instruction(addsd(xmm_free_1, info->insert_constant<double>(1.0)));
        new_instruction(movsd(qword_ptr(vm_value_q), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_DECd(CompileInfo* info)
    {
        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(movsd(xmm_free_1, qword_ptr(vm_value_q)));
        new_instruction(subsd(xmm_free_1, info->insert_constant<double>(1.0)));
        new_instruction(movsd(qword_ptr(vm_value_q), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_IncVi(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(inc(dword_ptr(vm_stack_frame_pointer, offset)));
    }

    void X86_64_Compiler::exec_asBC_DecVi(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(dec(dword_ptr(vm_stack_frame_pointer, offset)));
    }

    void X86_64_Compiler::exec_asBC_BNOT(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(not_(dword_ptr(vm_stack_frame_pointer, offset)));
    }

    void X86_64_Compiler::exec_asBC_BAND(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(and_(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_BOR(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(or_(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_BXOR(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(xor_(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_BSLL(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(shift_second_arg, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(sal(dword_free_1, shift_second_arg));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_BSRL(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(shift_second_arg, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(shr(dword_free_1, shift_second_arg));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_BSRA(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(shift_second_arg, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(sar(dword_free_1, shift_second_arg));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_COPY(CompileInfo* info)
    {
        asDWORD size = arg_value_dword(0) * 4;

        new_instruction(mov(qword_first_arg, qword_ptr(vm_stack_pointer)));
        new_instruction(add(vm_stack_pointer, ptr_size_1));
        new_instruction(mov(qword_second_arg, qword_ptr(vm_stack_pointer)));

        Label nullptr_access = info->assembler.newLabel();
        Label is_ok          = info->assembler.newLabel();
        Label end            = info->assembler.newLabel();

        new_instruction(cmp(qword_first_arg, 0));
        new_instruction(je(nullptr_access));
        new_instruction(cmp(qword_second_arg, 0));
        new_instruction(je(nullptr_access));
        new_instruction(jmp(is_ok));

        new_instruction(bind(nullptr_access));
        new_instruction(call(make_exception_nullptr_access));
        new_instruction(jmp(end));

        new_instruction(bind(is_ok));
        new_instruction(mov(dword_third_arg, size));
        save_registers(info);
        new_instruction(call(std::memcpy));
        restore_registers(info);

        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_PshC8(CompileInfo* info)
    {
        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(mov(qword_free_1, arg_value_qword()));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_PshVPtr(CompileInfo* info)
    {
        short offset = arg_offset(0);

        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(mov(qword_free_2, qword_ptr(vm_stack_frame_pointer, offset)));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_2));
    }

    void X86_64_Compiler::exec_asBC_RDSPtr(CompileInfo* info)
    {
        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_pointer)));
        new_instruction(cmp(qword_free_1, 0));
        Label is_valid = info->assembler.newLabel();

        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));

        new_instruction(bind(is_valid));
        new_instruction(mov(qword_free_1, qword_ptr(qword_free_1)));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_CMPd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(movsd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset0)));

        asmjit::Label is_less    = info->assembler.newLabel();
        asmjit::Label is_greater = info->assembler.newLabel();
        asmjit::Label end        = info->assembler.newLabel();

        new_instruction(comisd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(jne(is_greater));
        new_instruction(mov(vm_value_d, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater));
        new_instruction(jb(is_less));
        new_instruction(mov(vm_value_d, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(vm_value_d, -1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_CMPu(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset0)));

        asmjit::Label is_less    = info->assembler.newLabel();
        asmjit::Label is_greater = info->assembler.newLabel();
        asmjit::Label end        = info->assembler.newLabel();

        new_instruction(cmp(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(jne(is_greater));
        new_instruction(mov(vm_value_d, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater));
        new_instruction(jb(is_less));
        new_instruction(mov(vm_value_d, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(vm_value_d, -1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_CMPf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset0)));

        asmjit::Label is_less    = info->assembler.newLabel();
        asmjit::Label is_greater = info->assembler.newLabel();
        asmjit::Label end        = info->assembler.newLabel();

        new_instruction(comiss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(jne(is_greater));
        new_instruction(mov(vm_value_d, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater));
        new_instruction(jb(is_less));
        new_instruction(mov(vm_value_d, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(vm_value_d, -1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_CMPi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset0)));

        asmjit::Label is_less    = info->assembler.newLabel();
        asmjit::Label is_greater = info->assembler.newLabel();
        asmjit::Label end        = info->assembler.newLabel();

        new_instruction(cmp(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(jne(is_greater));
        new_instruction(mov(vm_value_d, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater));
        new_instruction(jl(is_less));
        new_instruction(mov(vm_value_d, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(vm_value_d, -1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_CMPIi(CompileInfo* info)
    {
        int value     = arg_value_int();
        short offset0 = arg_offset(0);

        new_instruction(mov(qword_free_1, dword_ptr(vm_stack_frame_pointer, offset0)));

        asmjit::Label is_less         = info->assembler.newLabel();
        asmjit::Label is_greater_than = info->assembler.newLabel();
        asmjit::Label end             = info->assembler.newLabel();

        new_instruction(cmp(dword_free_1, value));
        new_instruction(jne(is_greater_than));
        new_instruction(mov(vm_value_d, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater_than));
        new_instruction(jle(is_less));
        new_instruction(mov(vm_value_d, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(vm_value_d, -1));
        new_instruction(bind(end));
    }


    void X86_64_Compiler::exec_asBC_CMPIf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        float value   = arg_value_float(0);

        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset0)));

        asmjit::Label is_less    = info->assembler.newLabel();
        asmjit::Label is_greater = info->assembler.newLabel();
        asmjit::Label end        = info->assembler.newLabel();

        new_instruction(comiss(xmm_free_1, info->insert_constant<float>(value)));
        new_instruction(jne(is_greater));
        new_instruction(mov(vm_value_d, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater));
        new_instruction(jb(is_less));
        new_instruction(mov(vm_value_d, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(vm_value_d, -1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_CMPIu(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        asDWORD value = arg_value_dword(0);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset0)));

        asmjit::Label is_less    = info->assembler.newLabel();
        asmjit::Label is_greater = info->assembler.newLabel();
        asmjit::Label end        = info->assembler.newLabel();

        new_instruction(cmp(dword_free_1, value));
        new_instruction(jne(is_greater));
        new_instruction(mov(vm_value_d, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater));
        new_instruction(jb(is_less));
        new_instruction(mov(vm_value_d, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(vm_value_d, -1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_JMPP(CompileInfo* info)
    {
        NEED_IMPLEMENTATION();
    }

    void X86_64_Compiler::exec_asBC_PopRPtr(CompileInfo* info)
    {
        new_instruction(mov(vm_value_q, qword_ptr(vm_stack_pointer)));
        new_instruction(add(vm_stack_pointer, ptr_size_1));
    }

    void X86_64_Compiler::exec_asBC_PshRPtr(CompileInfo* info)
    {
        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(mov(dword_ptr(vm_stack_pointer), vm_value_q));
    }

    void X86_64_Compiler::exec_asBC_STR(CompileInfo* info)
    {
        // Byte code is deprecated!
        throw std::runtime_error("Byte code asBC_STR is deprecated!");
    }

    void X86_64_Compiler::exec_asBC_CALLSYS(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_CALLBND(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_SUSPEND(CompileInfo* info)
    {
        if (_M_with_suspend)
        {
            RETURN_CONTROL_TO_VM();
        }
    }

    void X86_64_Compiler::exec_asBC_ALLOC(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_FREE(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_LOADOBJ(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(vm_object_type, 0));

        new_instruction(mov(vm_object, qword_ptr(vm_stack_frame_pointer, offset)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset), 0));
    }

    void X86_64_Compiler::exec_asBC_STOREOBJ(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), vm_object));
        new_instruction(mov(vm_object, static_cast<uintptr_t>(0)));
    }

    void X86_64_Compiler::exec_asBC_GETOBJ(CompileInfo* info)
    {
        short offset = -arg_offset(0);
        new_instruction(mov(qword_free_1, vm_stack_pointer));
        new_instruction(add(qword_free_1, offset));

        new_instruction(mov(qword_free_2, qword_ptr(qword_free_1)));
        new_instruction(imul(qword_free_2, -sizeof(asDWORD)));

        new_instruction(mov(qword_free_3, qword_ptr(vm_stack_pointer, qword_free_2)));
        new_instruction(mov(qword_ptr(qword_free_1), qword_free_3));
        new_instruction(mov(qword_ptr(vm_stack_pointer, qword_free_2), 0));
    }

    void X86_64_Compiler::exec_asBC_REFCPY(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_CHKREF(CompileInfo* info)
    {
        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_pointer)));
        Label is_valid = info->assembler.newLabel();

        new_instruction(cmp(qword_free_1, 0));
        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));
        new_instruction(bind(is_valid));
    }

    void X86_64_Compiler::exec_asBC_GETOBJREF(CompileInfo* info)
    {
        short offset = -arg_offset(0);

        // TODO: Maybe it can be optimized?
        new_instruction(mov(qword_free_1, vm_stack_pointer));
        new_instruction(add(qword_free_1, offset));
        new_instruction(mov(qword_free_3, qword_ptr(qword_free_1)));
        new_instruction(imul(qword_free_3, sizeof(asDWORD)));

        new_instruction(mov(qword_free_2, vm_stack_frame_pointer));
        new_instruction(sub(qword_free_2, qword_free_3));
        new_instruction(mov(qword_free_2, qword_ptr(qword_free_2)));
        new_instruction(mov(qword_ptr(qword_free_1), qword_free_2));
    }

    void X86_64_Compiler::exec_asBC_GETREF(CompileInfo* info)
    {
        asWORD offset = arg_value_word() * sizeof(asDWORD);
        new_instruction(mov(qword_free_1, vm_stack_pointer));
        new_instruction(add(qword_free_1, offset));
        new_instruction(mov(dword_free_2, dword_ptr(qword_free_1)));
        new_instruction(imul(dword_free_2, sizeof(int)));
        new_instruction(mov(qword_ptr(qword_free_1), vm_stack_frame_pointer));
        new_instruction(sub(qword_ptr(qword_free_1), dword_free_2));
    }

    void X86_64_Compiler::exec_asBC_PshNull(CompileInfo* info)
    {
        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(mov(qword_ptr(vm_stack_pointer), 0));
    }

    void X86_64_Compiler::exec_asBC_ClrVPtr(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset), 0));
    }

    void X86_64_Compiler::exec_asBC_OBJTYPE(CompileInfo* info)
    {
        asPWORD ptr = arg_value_ptr();
        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(movabs(qword_free_1, ptr));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_TYPEID(CompileInfo* info)
    {
        return exec_asBC_PshC4(info);
    }

    void X86_64_Compiler::exec_asBC_SetV4(CompileInfo* info)
    {
        short offset  = arg_offset(0);
        asDWORD value = arg_value_dword(0);
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset), value));
    }

    void X86_64_Compiler::exec_asBC_SetV8(CompileInfo* info)
    {
        asQWORD value = arg_value_qword();
        short offset  = arg_offset(0);

        new_instruction(mov(qword_free_2, value));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset), qword_free_2));
    }

    void X86_64_Compiler::exec_asBC_ADDSi(CompileInfo* info)
    {
        new_instruction(mov(qword_free_2, qword_ptr(vm_stack_pointer)));
        new_instruction(cmp(qword_free_2, 0));

        Label is_valid = info->assembler.newLabel();

        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));

        short offset = arg_value_short(0);
        new_instruction(bind(is_valid));
        new_instruction(add(qword_free_2, offset));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_2));
    }

    void X86_64_Compiler::exec_asBC_CpyVtoV4(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(dword_free_2, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_2));
    }

    void X86_64_Compiler::exec_asBC_CpyVtoV8(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(qword_free_2, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), qword_free_2));
    }

    void X86_64_Compiler::exec_asBC_CpyVtoR4(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        new_instruction(mov(vm_value_d, dword_ptr(vm_stack_frame_pointer, offset0)));
    }

    void X86_64_Compiler::exec_asBC_CpyVtoR8(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(vm_value_q, qword_ptr(vm_stack_frame_pointer, offset)));
    }

    void X86_64_Compiler::exec_asBC_CpyVtoG4(CompileInfo* info)
    {
        asPWORD ptr  = arg_value_ptr();
        short offset = arg_offset(0);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset)));
        new_instruction(movabs(qword_free_2, ptr));
        new_instruction(mov(dword_ptr(qword_free_2), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_CpyRtoV4(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset), vm_value_d));
    }

    void X86_64_Compiler::exec_asBC_CpyRtoV8(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset), vm_value_q));
    }

    void X86_64_Compiler::exec_asBC_CpyGtoV4(CompileInfo* info)
    {
        asPWORD ptr  = arg_value_ptr();
        short offset = arg_offset(0);
        new_instruction(movabs(qword_free_1, ptr));
        new_instruction(mov(dword_free_1, dword_ptr(qword_free_1)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset), dword_free_1));
    }


    void X86_64_Compiler::exec_asBC_WRTV1(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(byte_free_1, byte_ptr(vm_stack_frame_pointer, offset)));
        new_instruction(mov(byte_ptr(vm_value_q), byte_free_1));
    }

    void X86_64_Compiler::exec_asBC_WRTV2(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(word_free_1, word_ptr(vm_stack_frame_pointer, offset)));
        new_instruction(mov(word_ptr(vm_value_q), word_free_1));
    }

    void X86_64_Compiler::exec_asBC_WRTV4(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(mov(dword_ptr(vm_value_q), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_WRTV8(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(mov(qword_ptr(vm_value_q), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_RDR1(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(byte_free_1, byte_ptr(vm_value_q)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset), byte_free_1));
    }

    void X86_64_Compiler::exec_asBC_RDR2(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(word_free_1, word_ptr(vm_value_q)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset), word_free_1));
    }

    void X86_64_Compiler::exec_asBC_RDR4(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(dword_free_1, dword_ptr(vm_value_q)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_RDR8(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        new_instruction(mov(qword_free_1, qword_ptr(vm_value_q)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_LDG(CompileInfo* info)
    {
        asPWORD ptr = arg_value_ptr();
        new_instruction(mov(vm_value_q, ptr));
    }

    void X86_64_Compiler::exec_asBC_LDV(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(qword_free_1, vm_stack_frame_pointer));
        new_instruction(add(qword_free_1, offset));
        new_instruction(mov(vm_value_q, dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_PGA(CompileInfo* info)
    {
        asPWORD value = arg_value_ptr();
        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(movabs(qword_free_1, value));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_CmpPtr(CompileInfo* info)
    {
        return exec_asBC_CMPu64(info);
    }

    void X86_64_Compiler::exec_asBC_VAR(CompileInfo* info)
    {
        asPWORD value = static_cast<asPWORD>(arg_value_short(0));
        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(movabs(qword_free_1, value));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_iTOf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(cvtsi2ss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(movss(qword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_fTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(cvttss2si(dword_free_2, dword_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_2));
    }

    void X86_64_Compiler::exec_asBC_uTOf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        save_registers(info);
        new_instruction(mov(dword_firts_arg, dword_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(call(uint_to_float));
        restore_registers(info);
        new_instruction(movss(dword_ptr(qword_free_1, offset0), float_return));
    }

    void X86_64_Compiler::exec_asBC_fTOu(CompileInfo* info)
    {
        short offset = arg_offset(0);

        new_instruction(cvttss2si(qword_free_2, dword_ptr(vm_stack_frame_pointer, offset)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset), qword_free_2));
    }

    void X86_64_Compiler::exec_asBC_sbTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(movsx(dword_free_2, byte_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_2));
    }

    void X86_64_Compiler::exec_asBC_swTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(movsx(dword_free_2, word_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_2));
    }

    void X86_64_Compiler::exec_asBC_ubTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(movzx(dword_free_2, byte_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_2));
    }

    void X86_64_Compiler::exec_asBC_uwTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(movzx(dword_free_2, word_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_2));
    }

    void X86_64_Compiler::exec_asBC_dTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(cvttsd2si(dword_free_2, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_2));
    }

    void X86_64_Compiler::exec_asBC_dTOu(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(cvttsd2si(qword_free_2, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_2));
    }

    void X86_64_Compiler::exec_asBC_dTOf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(cvtsd2ss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(movss(qword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_iTOd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(cvtsi2sd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(movsd(qword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_uTOd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        save_registers(info);
        new_instruction(mov(dword_firts_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(call(uint_to_double));
        restore_registers(info);
        new_instruction(movsd(dword_ptr(vm_stack_frame_pointer, offset0), double_return));
    }

    void X86_64_Compiler::exec_asBC_fTOd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(cvtss2sd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(movsd(qword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_ADDi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(add(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_SUBi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(sub(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_MULi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(imul(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_DIVi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_div_first_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(cdq());
        new_instruction(idiv(dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_div_first_arg));
    }

    void X86_64_Compiler::exec_asBC_MODi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_div_first_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(cdq());
        new_instruction(idiv(dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_div_mod_result));
    }

    void X86_64_Compiler::exec_asBC_ADDf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(addss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_SUBf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(subss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_MULf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mulss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_DIVf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(divss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_MODf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        save_registers(info);
        new_instruction(movss(float_firts_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(movss(float_second_arg, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(call(mod_float));
        restore_registers(info);
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), float_return));
    }

    void X86_64_Compiler::exec_asBC_ADDd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(movsd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(addsd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(movsd(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_SUBd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(movsd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(subsd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(movsd(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_MULd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(movsd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mulsd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(movsd(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_DIVd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(movsd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(divsd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(movsd(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_MODd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        save_registers(info);
        new_instruction(movsd(double_firts_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(movsd(double_second_arg, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(call(mod_double));
        restore_registers(info);
        new_instruction(movsd(dword_ptr(vm_stack_frame_pointer, offset0), double_return));
    }

    void X86_64_Compiler::exec_asBC_ADDIi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        int value     = (asBC_INTARG(info->address + 1));

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(add(dword_free_1, value));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_SUBIi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        int value     = (asBC_INTARG(info->address + 1));

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(sub(dword_free_1, value));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_MULIi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        int value     = (asBC_INTARG(info->address + 1));

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(imul(dword_free_1, value));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_ADDIf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        float value   = arg_value_float(1);

        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(addss(xmm_free_1, info->insert_constant(value)));
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_SUBIf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        float value   = arg_value_float(1);

        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(subss(xmm_free_1, info->insert_constant(value)));
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_MULIf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        float value   = arg_value_float(1);

        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mulss(xmm_free_1, info->insert_constant(value)));
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_SetG4(CompileInfo* info)
    {
        asPWORD ptr   = arg_value_ptr();
        asDWORD value = asBC_DWORDARG(info->address + AS_PTR_SIZE);
        new_instruction(movabs(qword_free_1, ptr));
        new_instruction(mov(dword_ptr(qword_free_1), value));
    }

    void X86_64_Compiler::exec_asBC_ChkRefS(CompileInfo* info)
    {
        Label is_valid = info->assembler.newLabel();

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_pointer)));
        new_instruction(mov(qword_free_1, qword_ptr(qword_free_1)));
        new_instruction(cmp(qword_free_1, 0));
        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));
        new_instruction(bind(is_valid));
    }

    void X86_64_Compiler::exec_asBC_ChkNullV(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset)));

        Label is_valid = info->assembler.newLabel();
        new_instruction(cmp(dword_free_1, 0));
        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));
        new_instruction(bind(is_valid));
    }

    void X86_64_Compiler::exec_asBC_CALLINTF(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }


    void X86_64_Compiler::exec_asBC_iTOb(CompileInfo* info)
    {
        new_instruction(mov(vm_value_d, vm_value_b));
    }

    void X86_64_Compiler::exec_asBC_iTOw(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        info->assembler.mov(word_ptr(vm_stack_frame_pointer, offset0 + 2), static_cast<asWORD>(0));
    }

    void X86_64_Compiler::exec_asBC_SetV1(CompileInfo* info)
    {
        short value   = arg_value_dword(0);
        short offset0 = arg_offset(0);

        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), value));
    }

    void X86_64_Compiler::exec_asBC_SetV2(CompileInfo* info)
    {
        short value   = arg_value_dword(0);
        short offset0 = arg_offset(0);

        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), value));
    }

    void X86_64_Compiler::exec_asBC_Cast(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_i64TOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(qword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), dword_free_1));
    }

    void X86_64_Compiler::exec_asBC_uTOi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_iTOi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(dword_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(cdqe());
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_fTOi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(movss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(cvttss2si(qword_free_1, xmm_free_1));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_dTOi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(cvttsd2si(qword_free_2, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), qword_free_2));
    }

    void X86_64_Compiler::exec_asBC_fTOu64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        save_registers(info);
        new_instruction(movss(float_firts_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(call(float_to_uint64));
        restore_registers(info);
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), qword_return));
    }

    void X86_64_Compiler::exec_asBC_dTOu64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        save_registers(info);
        new_instruction(movsd(double_firts_arg, dword_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(call(double_to_uint64));
        restore_registers(info);
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), qword_return));
    }

    void X86_64_Compiler::exec_asBC_i64TOf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(cvtsi2ss(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_u64TOf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        save_registers(info);
        new_instruction(mov(qword_first_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(call(uint64_to_float));
        restore_registers(info);
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), float_return));
    }

    void X86_64_Compiler::exec_asBC_i64TOd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(pxor(xmm_free_1, xmm_free_1));
        new_instruction(cvtsi2sd(xmm_free_1, dword_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(movsd(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_u64TOd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        save_registers(info);
        new_instruction(mov(qword_first_arg, dword_ptr(vm_stack_frame_pointer, offset0)));
        new_instruction(call(uint64_to_double));
        restore_registers(info);
        new_instruction(movsd(dword_ptr(vm_stack_frame_pointer, offset0), xmm_free_1));
    }

    void X86_64_Compiler::exec_asBC_NEGi64(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(neg(qword_ptr(vm_stack_frame_pointer, offset)));
    }

    void X86_64_Compiler::exec_asBC_INCi64(CompileInfo* info)
    {
        new_instruction(inc(qword_ptr(vm_value_q)));
    }

    void X86_64_Compiler::exec_asBC_DECi64(CompileInfo* info)
    {
        new_instruction(dec(qword_ptr(vm_value_q)));
    }

    void X86_64_Compiler::exec_asBC_BNOT64(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(not_(qword_ptr(vm_stack_frame_pointer, offset)));
    }

    void X86_64_Compiler::exec_asBC_ADDi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(add(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_SUBi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(sub(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_MULi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(imul(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_DIVi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_div_first_arg, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(cdq());
        new_instruction(idiv(qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_div_first_arg));
    }

    void X86_64_Compiler::exec_asBC_MODi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_div_first_arg, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(cdq());
        new_instruction(idiv(qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_div_mod_result));
    }

    void X86_64_Compiler::exec_asBC_BAND64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(and_(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_BOR64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(or_(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_BXOR64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(xor_(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_BSLL64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(shift_second_arg, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(sal(dword_free_1, shift_second_arg));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_BSRL64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(shift_second_arg, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(shr(qword_free_1, shift_second_arg));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_BSRA64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(shift_second_arg, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(sar(qword_free_1, shift_second_arg));
        new_instruction(mov(qword_ptr(vm_stack_frame_pointer, offset0), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_CMPi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset0)));

        asmjit::Label is_less    = info->assembler.newLabel();
        asmjit::Label is_greater = info->assembler.newLabel();
        asmjit::Label end        = info->assembler.newLabel();

        new_instruction(cmp(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(jne(is_greater));
        new_instruction(mov(vm_value_d, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater));
        new_instruction(jl(is_less));
        new_instruction(mov(vm_value_d, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(vm_value_d, -1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_CMPu64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset0)));

        asmjit::Label is_less    = info->assembler.newLabel();
        asmjit::Label is_greater = info->assembler.newLabel();
        asmjit::Label end        = info->assembler.newLabel();

        new_instruction(cmp(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(jne(is_greater));
        new_instruction(mov(vm_value_d, 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater));
        new_instruction(jb(is_less));
        new_instruction(mov(vm_value_d, 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(vm_value_d, -1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_ChkNullS(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(cmp(qword_ptr(vm_stack_pointer, offset), 0));
        Label is_valid = info->assembler.newLabel();
        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));
        new_instruction(bind(is_valid));
    }

    void X86_64_Compiler::exec_asBC_ClrHi(CompileInfo* info)
    {
        new_instruction(mov(vm_value_q, vm_value_b));
    }

    void X86_64_Compiler::exec_asBC_JitEntry(CompileInfo* info)
    {
        asDWORD offset               = static_cast<asDWORD>(info->assembler.offset()) - info->header_size;
        asBC_DWORDARG(info->address) = offset;
    }

    void X86_64_Compiler::exec_asBC_CallPtr(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_FuncPtr(CompileInfo* info)
    {
        asPWORD ptr = arg_value_ptr();
        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(mov(qword_free_1, ptr));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_1));
    }

    void X86_64_Compiler::exec_asBC_LoadThisR(CompileInfo* info)
    {
        short value0 = arg_value_short(0);

        Label is_valid = info->assembler.newLabel();

        new_instruction(mov(vm_value_q, qword_ptr(vm_stack_frame_pointer)));
        new_instruction(cmp(vm_value_q, 0));

        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));

        new_instruction(bind(is_valid));
        new_instruction(add(vm_value_q, value0));
    }

    void X86_64_Compiler::exec_asBC_PshV8(CompileInfo* info)
    {
        short offset = arg_offset(0);

        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(mov(qword_free_2, qword_ptr(vm_stack_frame_pointer, offset)));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_2));
    }

    void X86_64_Compiler::exec_asBC_DIVu(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_div_first_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(dword_div_mod_result, 0));
        new_instruction(div(dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_div_mod_result));
    }

    void X86_64_Compiler::exec_asBC_MODu(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(dword_div_first_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(dword_div_mod_result, 0));
        new_instruction(div(dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_div_mod_result));
    }

    void X86_64_Compiler::exec_asBC_DIVu64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_div_first_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(qword_div_mod_result, 0));
        new_instruction(div(dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), qword_div_mod_result));
    }

    void X86_64_Compiler::exec_asBC_MODu64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(qword_div_first_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(qword_div_mod_result, 0));
        new_instruction(div(dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), qword_div_mod_result));
    }

    void X86_64_Compiler::exec_asBC_LoadRObjR(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_value_short(1);

        Label is_valid = info->assembler.newLabel();

        new_instruction(mov(qword_free_1, vm_stack_frame_pointer));
        new_instruction(add(qword_free_1, offset0));
        new_instruction(mov(qword_free_2, qword_ptr(qword_free_1)));
        new_instruction(cmp(qword_free_2, 0));

        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));

        new_instruction(bind(is_valid));
        new_instruction(add(qword_free_2, offset1));

        new_instruction(mov(vm_value_q, qword_free_2));
    }

    void X86_64_Compiler::exec_asBC_LoadVObjR(CompileInfo* info)
    {
        short offset  = arg_offset(0);
        short offset2 = arg_value_short(1);

        new_instruction(mov(vm_value_q, vm_stack_frame_pointer));
        new_instruction(add(vm_value_q, offset + offset2));
    }

    void X86_64_Compiler::exec_asBC_RefCpyV(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_JLowZ(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(vm_value_b, 0));
        new_instruction(je(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JLowNZ(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(vm_value_b, 0));
        new_instruction(jne(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_AllocMem(CompileInfo* info)
    {
        // Since the virtual machine is used to clear the memory,
        // the memory allocation must also be performed by the virtual machine
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_SetListSize(CompileInfo* info)
    {
        short offset = arg_offset(0);
        asUINT off   = arg_value_dword(0);
        asUINT size  = arg_value_dword(1);

        Label is_valid = info->assembler.newLabel();
        Label end      = info->assembler.newLabel();

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset)));

        new_instruction(cmp(qword_free_1, 0));
        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));
        new_instruction(jmp(end));

        new_instruction(bind(is_valid));
        new_instruction(mov(dword_ptr(qword_free_1, off), size));

        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_PshListElmnt(CompileInfo* info)
    {
        short offset = arg_offset(0);
        asUINT off   = arg_value_dword(0);

        Label is_valid = info->assembler.newLabel();
        Label end      = info->assembler.newLabel();

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset)));

        new_instruction(cmp(qword_free_1, 0));
        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));
        new_instruction(jmp(end));

        new_instruction(bind(is_valid));
        new_instruction(add(qword_free_1, off));
        new_instruction(sub(vm_stack_pointer, ptr_size_1));
        new_instruction(mov(qword_ptr(vm_stack_pointer), qword_free_1));

        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_SetListType(CompileInfo* info)
    {
        short offset = arg_offset(0);
        asUINT type  = arg_value_dword(1);
        asUINT off   = arg_value_dword(0);

        Label is_valid = info->assembler.newLabel();
        Label end      = info->assembler.newLabel();

        new_instruction(mov(qword_free_1, qword_ptr(vm_stack_frame_pointer, offset)));

        new_instruction(cmp(qword_free_1, 0));
        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));
        new_instruction(jmp(end));

        new_instruction(bind(is_valid));
        new_instruction(mov(dword_ptr(qword_free_1, off), type));

        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_POWi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        save_registers(info);
        new_instruction(mov(dword_firts_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(dword_second_arg, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(call(ipow));
        restore_registers(info);
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_return));
    }

    void X86_64_Compiler::exec_asBC_POWu(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        save_registers(info);
        new_instruction(mov(dword_firts_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(dword_second_arg, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(call(upow));
        restore_registers(info);
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), dword_return));
    }

    void X86_64_Compiler::exec_asBC_POWf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        save_registers(info);
        new_instruction(movss(float_firts_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(movss(float_second_arg, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(call(fpow));
        restore_registers(info);
        new_instruction(movss(dword_ptr(vm_stack_frame_pointer, offset0), float_return));
    }

    void X86_64_Compiler::exec_asBC_POWd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        save_registers(info);
        new_instruction(movsd(double_firts_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(movsd(double_second_arg, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(call(dpow));
        restore_registers(info);
        new_instruction(movsd(dword_ptr(vm_stack_frame_pointer, offset0), double_return));
    }

    void X86_64_Compiler::exec_asBC_POWdi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        save_registers(info);
        new_instruction(movsd(double_firts_arg, dword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(dword_firts_arg, dword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(call(dipow));
        restore_registers(info);
        new_instruction(movsd(dword_ptr(vm_stack_frame_pointer, offset0), double_return));
    }

    void X86_64_Compiler::exec_asBC_POWi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        save_registers(info);
        new_instruction(mov(qword_first_arg, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(qword_second_arg, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(call(i64pow));
        restore_registers(info);
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), qword_return));
    }

    void X86_64_Compiler::exec_asBC_POWu64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        save_registers(info);
        new_instruction(mov(qword_first_arg, qword_ptr(vm_stack_frame_pointer, offset1)));
        new_instruction(mov(qword_second_arg, qword_ptr(vm_stack_frame_pointer, offset2)));
        new_instruction(call(u64pow));
        restore_registers(info);
        new_instruction(mov(dword_ptr(vm_stack_frame_pointer, offset0), qword_return));
    }

    void X86_64_Compiler::exec_asBC_Thiscall1(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }
}// namespace JIT
