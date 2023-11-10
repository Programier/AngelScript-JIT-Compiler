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

#define CONST_POOL_ZONE_SIZE 64

#define PTR_SIZE_1 static_cast<int32_t>(sizeof(void*) * 1)
#define PTR_SIZE_2 static_cast<int32_t>(sizeof(void*) * 2)
#define PTR_SIZE_3 static_cast<int32_t>(sizeof(void*) * 3)
#define PTR_SIZE_4 static_cast<int32_t>(sizeof(void*) * 4)
#define PTR_SIZE_5 static_cast<int32_t>(sizeof(void*) * 5)
#define PTR_SIZE_6 static_cast<int32_t>(sizeof(void*) * 6)
#define PTR_SIZE_7 static_cast<int32_t>(sizeof(void*) * 7)


#define VM_REGISTER (-8)
#define STACK_FRAME_POINTER_REGISTER (-8 + VM_REGISTER)
#define STACK_POINTER_REGISTER (-8 + STACK_FRAME_POINTER_REGISTER)
#define VALUE_REGISTER (-8 + STACK_POINTER_REGISTER)
#define OBJECT_REGISTER (-8 + VALUE_REGISTER)
#define OBJECT_TYPE_REGISTER (-8 + OBJECT_REGISTER)
#define LAST_REGISTER OBJECT_TYPE_REGISTER

#define FUNC_DECL
#define RETURN_CONTROL_TO_VM() return exec_asBC_RET(info)

#define arg_value_dword() asBC_DWORDARG(info->address)
#define arg_value_int() asBC_INTARG(info->address)
#define arg_value_qword() asBC_QWORDARG(info->address)
#define arg_value_float(offset) asBC_FLOATARG(info->address + offset)
#define arg_value_ptr() asBC_PTRARG(info->address)
#define arg_value_word(index) (*(((asWORD*) info->address) + index + 1))
#define arg_value_short(index) (*(((short*) info->address) + index + 1))

#define arg_offset(index) (-(*(((short*) info->address) + index + 1)) * sizeof(asDWORD))
#define new_instruction(x) catch_errors(info->assembler.x)


namespace JIT
{
    static float FUNC_DECL mod_float(float a, float b)
    {
        return std::fmod<float, float>(a, b);
    }

    static float FUNC_DECL mod_double(double a, double b)
    {
        return std::fmod<double, double>(a, b);
    }

    static void FUNC_DECL make_exception_nullptr_access()
    {
        throw std::runtime_error("Attempting to access a null pointer");
    }

    ////////////////////// MUST BE REMOVED IN FUTURE! //////////////////////
    static double FUNC_DECL uint_to_double(uint32_t value)
    {
        return static_cast<double>(value);
    }

    static double FUNC_DECL uint64_to_double(uint64_t value)
    {
        return static_cast<double>(value);
    }

    static float FUNC_DECL uint_to_float(uint32_t value)
    {
        return static_cast<float>(value);
    }


    static float FUNC_DECL uint64_to_float(uint64_t value)
    {
        return static_cast<float>(value);
    }


    static uint64_t FUNC_DECL double_to_uint64(double value)
    {
        return static_cast<uint64_t>(value);
    }

    static uint64_t FUNC_DECL float_to_uint64(float value)
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

    X86_64_Compiler::X86_64_Compiler()
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

#if WITH_LOG
        logf("ByteCodes: [");
        for (asUINT i = 0; i < info.byte_codes; i++)
        {
            logf("%u%s", info.begin[i], (i == info.byte_codes - 1 ? "]\n" : ", "));
        }

#endif

        CodeHolder code;
        code.init(_M_rt.environment(), _M_rt.cpuFeatures());
        new (&info.assembler) Assembler(&code);

        init(&info);
        info.address = info.begin;

        Zone zone(CONST_POOL_ZONE_SIZE);
        ConstPool const_pool(&zone);

        Label const_pool_label = info.assembler.newLabel();
        info.const_pool_label  = &const_pool_label;
        info.const_pool        = &const_pool;

        while (info.address < info.end)
        {
            info.instruction = static_cast<asEBCInstr>(*reinterpret_cast<asBYTE*>(info.address));
            info.address += process_instruction(&info);
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

            if (info->instruction == asBC_JitEntry || info->instruction == asBC_SUSPEND)
            {
                is_implemented = true;
            }

#if !SHOW_JIT_ENTRY
            if (info->instruction != asBC_JitEntry && info->instruction != asBC_SUSPEND)
#endif
            {
                logf("Process instuction: %-15s (%3zu with size = %u, %15s) -> [", code_names[index], index, size,
                     (is_implemented ? "IMPLEMENTED" : "NOT IMPLEMENTED"));

                for (decltype(size) i = 1; i < size; i++)
                {
                    logf("%u%s", info->address[i], (i == size - 1 ? "" : ", "));
                }
                printf("]\n");
            }
        }
#endif

        info->prev_instruction = info->instruction;
        return instruction_size(info->instruction);
    }

    void X86_64_Compiler::init(CompileInfo* info)
    {
        new_instruction(push(rbp));
        new_instruction(mov(rbp, rsp));
        new_instruction(sub(rsp, -LAST_REGISTER));

        // Copy register values
        new_instruction(mov(qword_ptr(rbp, VM_REGISTER), rdi));

        new_instruction(mov(rax, qword_ptr(rdi, offsetof(asSVMRegisters, stackFramePointer))));
        new_instruction(mov(qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER), rax));

        new_instruction(mov(rax, qword_ptr(rdi, offsetof(asSVMRegisters, stackPointer))));
        new_instruction(mov(qword_ptr(rbp, STACK_POINTER_REGISTER), rax));

        new_instruction(mov(rax, qword_ptr(rdi, offsetof(asSVMRegisters, valueRegister))));
        new_instruction(mov(qword_ptr(rbp, VALUE_REGISTER), rax));

        new_instruction(mov(rax, qword_ptr(rdi, offsetof(asSVMRegisters, objectRegister))));
        new_instruction(mov(qword_ptr(rbp, OBJECT_REGISTER), rax));

        new_instruction(mov(rax, qword_ptr(rdi, offsetof(asSVMRegisters, objectType))));
        new_instruction(mov(qword_ptr(rbp, OBJECT_TYPE_REGISTER), rax));

        // Restore position of execution
        new_instruction(lea(rax, qword_ptr(rip)));
        info->header_size = info->assembler.offset();
        new_instruction(add(rax, rsi));
        new_instruction(jmp(rax));

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
                }

                default:
                    break;
            }
            start += instruction_size(op);
        }
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
        new_instruction(add(dword_ptr(rbp, STACK_POINTER_REGISTER), PTR_SIZE_1));
    }

    void X86_64_Compiler::exec_asBC_PshGPtr(CompileInfo* info)
    {
        asPWORD value = *(asPWORD*) asBC_PTRARG(info->address);

        new_instruction(sub(dword_ptr(rbp, STACK_POINTER_REGISTER), PTR_SIZE_1));
        new_instruction(mov(rax, qword_ptr(rbp, STACK_POINTER_REGISTER)));
        new_instruction(mov(dword_ptr(rax), value));
    }

    void X86_64_Compiler::exec_asBC_PshC4(CompileInfo* info)
    {
        new_instruction(sub(dword_ptr(rbp, STACK_POINTER_REGISTER), PTR_SIZE_1 / 2));
        new_instruction(mov(rax, dword_ptr(rbp, STACK_POINTER_REGISTER)));
        new_instruction(mov(dword_ptr(rax), asBC_DWORDARG(info->address)));
    }

    void X86_64_Compiler::exec_asBC_PshV4(CompileInfo* info)
    {
        new_instruction(sub(dword_ptr(rbp, STACK_POINTER_REGISTER), PTR_SIZE_1 / 2));
        new_instruction(mov(rax, dword_ptr(rbp, STACK_POINTER_REGISTER)));

        short offset = arg_offset(0);
        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(ebx, dword_ptr(rbx, offset)));
        new_instruction(mov(dword_ptr(rax), ebx));
    }

    void X86_64_Compiler::exec_asBC_PSF(CompileInfo* info)
    {
        short offset = arg_offset(0);

        new_instruction(sub(dword_ptr(rbp, STACK_POINTER_REGISTER), PTR_SIZE_1));
        new_instruction(mov(rax, dword_ptr(rbp, STACK_POINTER_REGISTER)));

        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(add(rbx, offset));
        new_instruction(mov(qword_ptr(rax), rbx));
    }

    void X86_64_Compiler::exec_asBC_SwapPtr(CompileInfo* info)
    {}
    void X86_64_Compiler::exec_asBC_NOT(CompileInfo* info)
    {}
    void X86_64_Compiler::exec_asBC_PshG4(CompileInfo* info)
    {}
    void X86_64_Compiler::exec_asBC_LdGRdR4(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_CALL(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_RET(CompileInfo* info)
    {
        new_instruction(mov(rax, qword_ptr(rbp, VM_REGISTER)));

        new_instruction(mov(rbx, info->address));
        new_instruction(mov(qword_ptr(rax, offsetof(asSVMRegisters, programPointer)), rbx));

        new_instruction(mov(rbx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(qword_ptr(rax, offsetof(asSVMRegisters, stackFramePointer)), rbx));

        new_instruction(mov(rbx, qword_ptr(rbp, STACK_POINTER_REGISTER)));
        new_instruction(mov(qword_ptr(rax, offsetof(asSVMRegisters, stackPointer)), rbx));

        new_instruction(mov(rbx, qword_ptr(rbp, VALUE_REGISTER)));
        new_instruction(mov(qword_ptr(rax, offsetof(asSVMRegisters, valueRegister)), rbx));

        new_instruction(mov(rbx, qword_ptr(rbp, OBJECT_REGISTER)));
        new_instruction(mov(qword_ptr(rax, offsetof(asSVMRegisters, objectRegister)), rbx));

        new_instruction(mov(rbx, qword_ptr(rbp, OBJECT_TYPE_REGISTER)));
        new_instruction(mov(qword_ptr(rax, offsetof(asSVMRegisters, objectType)), rbx));

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
        new_instruction(cmp(dword_ptr(rbp, VALUE_REGISTER), 0));
        new_instruction(je(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JNZ(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(dword_ptr(rbp, VALUE_REGISTER), 0));
        new_instruction(jne(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JS(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(dword_ptr(rbp, VALUE_REGISTER), 0));
        new_instruction(jl(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JNS(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(dword_ptr(rbp, VALUE_REGISTER), 0));
        new_instruction(jge(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JP(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(dword_ptr(rbp, VALUE_REGISTER), 0));
        new_instruction(jg(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_JNP(CompileInfo* info)
    {
        size_t label_index = find_label_for_jump(info);
        new_instruction(cmp(dword_ptr(rbp, VALUE_REGISTER), 0));
        new_instruction(jle(info->labels[label_index].label));
    }

    void X86_64_Compiler::exec_asBC_TZ(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_TNZ(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_TS(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_TNS(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_TP(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_TNP(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_NEGi(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_NEGf(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_NEGd(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_INCi16(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_INCi8(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_DECi16(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_DECi8(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_INCi(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_DECi(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_INCf(CompileInfo* info)
    {
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(mov(rax, qword_ptr(rbp, VALUE_REGISTER)));
        new_instruction(movss(xmm0, qword_ptr(rax)));
        new_instruction(addss(xmm0, info->insert_constant<float>(1.0)));
        new_instruction(movss(qword_ptr(rax), xmm0));
    }

    void X86_64_Compiler::exec_asBC_DECf(CompileInfo* info)
    {
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(mov(rax, qword_ptr(rbp, VALUE_REGISTER)));
        new_instruction(movss(xmm0, qword_ptr(rax)));
        new_instruction(subss(xmm0, info->insert_constant<float>(1.0)));
        new_instruction(movss(qword_ptr(rax), xmm0));
    }

    void X86_64_Compiler::exec_asBC_INCd(CompileInfo* info)
    {
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(mov(rax, qword_ptr(rbp, VALUE_REGISTER)));
        new_instruction(movsd(xmm0, qword_ptr(rax)));
        new_instruction(addsd(xmm0, info->insert_constant<double>(1.0)));
        new_instruction(movsd(qword_ptr(rax), xmm0));
    }

    void X86_64_Compiler::exec_asBC_DECd(CompileInfo* info)
    {
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(mov(rax, qword_ptr(rbp, VALUE_REGISTER)));
        new_instruction(movsd(xmm0, qword_ptr(rax)));
        new_instruction(subsd(xmm0, info->insert_constant<double>(1.0)));
        new_instruction(movsd(qword_ptr(rax), xmm0));
    }

    void X86_64_Compiler::exec_asBC_IncVi(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(inc(dword_ptr(rax, offset)));
    }

    void X86_64_Compiler::exec_asBC_DecVi(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(dec(dword_ptr(rax, offset)));
    }

    void X86_64_Compiler::exec_asBC_BNOT(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BAND(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BOR(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BXOR(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BSLL(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BSRL(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BSRA(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_COPY(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_PshC8(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_PshVPtr(CompileInfo* info)
    {
        // FIX IT
        return exec_asBC_RET(info);
        short offset = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_POINTER_REGISTER)));
        new_instruction(sub(dword_ptr(rax), PTR_SIZE_1));
        new_instruction(mov(rax, dword_ptr(rax)));

        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rbx, qword_ptr(rbx, offset)));
        new_instruction(mov(qword_ptr(rax), rbx));
    }

    void X86_64_Compiler::exec_asBC_RDSPtr(CompileInfo* info)
    {
        new_instruction(mov(rax, dword_ptr(rbp, STACK_POINTER_REGISTER)));
        new_instruction(mov(rbx, qword_ptr(rax)));
        new_instruction(cmp(rbx, 0));

        Label is_valid = info->assembler.newLabel();

        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));

        new_instruction(bind(is_valid));
        new_instruction(mov(rbx, qword_ptr(rbx)));
        new_instruction(mov(qword_ptr(rax), rbx));
    }

    void X86_64_Compiler::exec_asBC_CMPd(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_CMPu(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_CMPf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rcx, offset0)));
        new_instruction(movss(xmm1, dword_ptr(rcx, offset1)));

        asmjit::Label is_less    = info->assembler.newLabel();
        asmjit::Label is_greater = info->assembler.newLabel();
        asmjit::Label end        = info->assembler.newLabel();

        new_instruction(comiss(xmm0, xmm1));
        new_instruction(jnp(is_greater));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater));
        new_instruction(comiss(xmm0, xmm1));
        new_instruction(jb(is_less));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), -1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_CMPi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rax, dword_ptr(rcx, offset0)));

        asmjit::Label is_less    = info->assembler.newLabel();
        asmjit::Label is_greater = info->assembler.newLabel();
        asmjit::Label end        = info->assembler.newLabel();

        new_instruction(cmp(eax, dword_ptr(rcx, offset1)));
        new_instruction(jne(is_greater));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater));
        new_instruction(jl(is_less));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), -1));
        new_instruction(bind(end));
    }

    void X86_64_Compiler::exec_asBC_CMPIi(CompileInfo* info)
    {
        int value     = asBC_INTARG(info->address);
        short offset0 = arg_offset(0);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rax, dword_ptr(rcx, offset0)));

        asmjit::Label is_less         = info->assembler.newLabel();
        asmjit::Label is_greater_than = info->assembler.newLabel();
        asmjit::Label end             = info->assembler.newLabel();

        new_instruction(cmp(eax, value));
        new_instruction(jne(is_greater_than));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), 0));
        new_instruction(jmp(end));

        new_instruction(bind(is_greater_than));
        new_instruction(jle(is_less));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), 1));
        new_instruction(jmp(end));

        new_instruction(bind(is_less));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), -1));
        new_instruction(bind(end));
    }


    void X86_64_Compiler::exec_asBC_CMPIf(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_CMPIu(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_JMPP(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_PopRPtr(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_PshRPtr(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_STR(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_CALLSYS(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_CALLBND(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_SUSPEND(CompileInfo* info)
    {}

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
        new_instruction(mov(dword_ptr(rbp, OBJECT_TYPE_REGISTER), 0));

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(add(rax, offset));
        new_instruction(mov(rbx, qword_ptr(rax)));
        new_instruction(mov(qword_ptr(rbp, OBJECT_REGISTER), rbx));
        new_instruction(mov(qword_ptr(rax), 0));
    }

    void X86_64_Compiler::exec_asBC_STOREOBJ(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rbx, qword_ptr(rbp, OBJECT_REGISTER)));
        new_instruction(mov(dword_ptr(rax, offset0), rbx));

        new_instruction(mov(qword_ptr(rbp, OBJECT_REGISTER), static_cast<uintptr_t>(0)));
    }

    void X86_64_Compiler::exec_asBC_GETOBJ(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_REFCPY(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_CHKREF(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_GETOBJREF(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_GETREF(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_PshNull(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_ClrVPtr(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_OBJTYPE(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_TYPEID(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_SetV4(CompileInfo* info)
    {
        short offset  = arg_offset(0);
        asDWORD value = asBC_DWORDARG(info->address);
        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(dword_ptr(rax, offset), value));
    }

    void X86_64_Compiler::exec_asBC_SetV8(CompileInfo* info)
    {
        asQWORD value = arg_value_qword();
        short offset  = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rbx, value));
        new_instruction(mov(dword_ptr(rax, offset), rbx));
    }


    void X86_64_Compiler::exec_asBC_ADDSi(CompileInfo* info)
    {
        new_instruction(mov(rax, dword_ptr(rbp, STACK_POINTER_REGISTER)));
        new_instruction(mov(rbx, qword_ptr(rax)));
        new_instruction(cmp(rbx, 0));

        Label is_valid = info->assembler.newLabel();

        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));

        short offset = arg_offset(0);
        new_instruction(bind(is_valid));
        new_instruction(add(rbx, offset));
        new_instruction(mov(qword_ptr(rax), rbx));
    }

    void X86_64_Compiler::exec_asBC_CpyVtoV4(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(ebx, dword_ptr(rax, offset1)));
        new_instruction(mov(dword_ptr(rax, offset0), ebx));
    }

    void X86_64_Compiler::exec_asBC_CpyVtoV8(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rbx, dword_ptr(rax, offset1)));
        new_instruction(mov(dword_ptr(rax, offset0), rbx));
    }

    void X86_64_Compiler::exec_asBC_CpyVtoR4(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rax, offset0)));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), eax));
    }

    void X86_64_Compiler::exec_asBC_CpyVtoR8(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rax, dword_ptr(rax, offset)));
        new_instruction(mov(qword_ptr(rbp, VALUE_REGISTER), rax));
    }

    void X86_64_Compiler::exec_asBC_CpyVtoG4(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_CpyRtoV4(CompileInfo* info)
    {
        short offset = arg_offset(0);

        new_instruction(mov(eax, dword_ptr(rbp, VALUE_REGISTER)));
        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(dword_ptr(rbx, offset), eax));
    }

    void X86_64_Compiler::exec_asBC_CpyRtoV8(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(rax, dword_ptr(rbp, VALUE_REGISTER)));
        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(dword_ptr(rbx, offset), rax));
    }

    void X86_64_Compiler::exec_asBC_CpyGtoV4(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_WRTV1(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_WRTV2(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_WRTV4(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rax, dword_ptr(rax, offset0)));

        new_instruction(mov(rbx, dword_ptr(rbp, VALUE_REGISTER)));
        new_instruction(mov(dword_ptr(rbx), eax));
    }

    void X86_64_Compiler::exec_asBC_WRTV8(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_RDR1(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_RDR2(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_RDR4(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(rax, dword_ptr(rbp, VALUE_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rax)));
        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(dword_ptr(rbx, offset), eax));
    }

    void X86_64_Compiler::exec_asBC_RDR8(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        new_instruction(mov(rax, qword_ptr(rbp, VALUE_REGISTER)));
        new_instruction(mov(rax, qword_ptr(rax)));
        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(qword_ptr(rbx, offset0), rax));
    }

    void X86_64_Compiler::exec_asBC_LDG(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_LDV(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(add(rax, offset));
        new_instruction(mov(qword_ptr(rbp, VALUE_REGISTER), eax));
    }

    void X86_64_Compiler::exec_asBC_PGA(CompileInfo* info)
    {
        asPWORD value = asBC_PTRARG(info->address);
        new_instruction(sub(dword_ptr(rbp, STACK_POINTER_REGISTER), PTR_SIZE_1));
        new_instruction(mov(rax, dword_ptr(rbp, STACK_POINTER_REGISTER)));
        new_instruction(mov(qword_ptr(rax), value));
    }

    void X86_64_Compiler::exec_asBC_CmpPtr(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_VAR(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_iTOf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(cvtsi2ss(xmm0, dword_ptr(rax, offset0)));
        new_instruction(movss(qword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_fTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(cvttss2si(ebx, dword_ptr(rax, offset0)));
        new_instruction(mov(dword_ptr(rax, offset0), ebx));
    }

    void X86_64_Compiler::exec_asBC_uTOf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(edi, dword_ptr(rax, offset0)));
        new_instruction(call(uint_to_float));
        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(dword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_fTOu(CompileInfo* info)
    {
        short offset = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(cvttss2si(rbx, dword_ptr(rax, offset)));
        new_instruction(mov(dword_ptr(rax, offset), rbx));
    }

    void X86_64_Compiler::exec_asBC_sbTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsx(ebx, byte_ptr(rax, offset0)));
        new_instruction(mov(dword_ptr(rax, offset0), ebx));
    }

    void X86_64_Compiler::exec_asBC_swTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsx(ebx, word_ptr(rax, offset0)));
        new_instruction(mov(dword_ptr(rax, offset0), ebx));
    }

    void X86_64_Compiler::exec_asBC_ubTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movzx(ebx, byte_ptr(rax, offset0)));
        new_instruction(mov(dword_ptr(rax, offset0), ebx));
    }

    void X86_64_Compiler::exec_asBC_uwTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movzx(ebx, word_ptr(rax, offset0)));
        new_instruction(mov(dword_ptr(rax, offset0), ebx));
    }

    void X86_64_Compiler::exec_asBC_dTOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(cvttsd2si(ebx, dword_ptr(rax, offset1)));
        new_instruction(mov(dword_ptr(rax, offset0), ebx));
    }

    void X86_64_Compiler::exec_asBC_dTOu(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(cvttsd2si(rbx, dword_ptr(rax, offset1)));
        new_instruction(mov(dword_ptr(rax, offset0), ebx));
    }

    void X86_64_Compiler::exec_asBC_dTOf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(cvtsd2ss(xmm0, dword_ptr(rax, offset1)));
        new_instruction(movss(qword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_iTOd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(cvtsi2sd(xmm0, dword_ptr(rax, offset1)));
        new_instruction(movsd(qword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_uTOd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(edi, dword_ptr(rax, offset1)));
        new_instruction(call(uint_to_double));
        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsd(dword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_fTOd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(cvtss2sd(xmm0, dword_ptr(rax, offset1)));
        new_instruction(movsd(qword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_ADDi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rcx, offset1)));
        new_instruction(add(eax, dword_ptr(rcx, offset2)));
        new_instruction(mov(dword_ptr(rcx, offset0), eax));
    }

    void X86_64_Compiler::exec_asBC_SUBi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rcx, offset1)));
        new_instruction(sub(eax, dword_ptr(rcx, offset2)));
        new_instruction(mov(dword_ptr(rcx, offset0), eax));
    }

    void X86_64_Compiler::exec_asBC_MULi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rcx, offset1)));
        new_instruction(imul(eax, dword_ptr(rcx, offset2)));
        new_instruction(mov(dword_ptr(rcx, offset0), eax));
    }

    void X86_64_Compiler::exec_asBC_DIVi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rcx, offset1)));
        new_instruction(cdq());
        new_instruction(idiv(dword_ptr(rcx, offset2)));
        new_instruction(mov(dword_ptr(rcx, offset0), eax));
    }

    void X86_64_Compiler::exec_asBC_MODi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rcx, offset1)));
        new_instruction(mov(ebx, dword_ptr(rcx, offset2)));
        new_instruction(cdq());
        new_instruction(idiv(ebx));
        new_instruction(mov(eax, edx));
        ;
        new_instruction(mov(dword_ptr(rcx, offset0), eax));
    }

    void X86_64_Compiler::exec_asBC_ADDf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rcx, offset1)));
        new_instruction(addss(xmm0, dword_ptr(rcx, offset2)));
        new_instruction(movss(dword_ptr(rcx, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_SUBf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rcx, offset1)));
        new_instruction(subss(xmm0, dword_ptr(rcx, offset2)));
        new_instruction(movss(dword_ptr(rcx, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_MULf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rcx, offset1)));
        new_instruction(mulss(xmm0, dword_ptr(rcx, offset2)));
        new_instruction(movss(dword_ptr(rcx, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_DIVf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rcx, offset1)));
        new_instruction(divss(xmm0, dword_ptr(rcx, offset2)));
        new_instruction(movss(dword_ptr(rcx, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_MODf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rcx, offset1)));
        new_instruction(movss(xmm1, dword_ptr(rcx, offset2)));
        new_instruction(call(mod_float));
        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(dword_ptr(rcx, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_ADDd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsd(xmm0, dword_ptr(rax, offset1)));
        new_instruction(addsd(xmm0, dword_ptr(rax, offset2)));
        new_instruction(movsd(dword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_SUBd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsd(xmm0, dword_ptr(rax, offset1)));
        new_instruction(subsd(xmm0, dword_ptr(rax, offset2)));
        new_instruction(movsd(dword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_MULd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsd(xmm0, dword_ptr(rax, offset1)));
        new_instruction(mulsd(xmm0, dword_ptr(rax, offset2)));
        new_instruction(movsd(dword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_DIVd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsd(xmm0, dword_ptr(rax, offset1)));
        new_instruction(divsd(xmm0, dword_ptr(rax, offset2)));
        new_instruction(movsd(dword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_MODd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsd(xmm0, dword_ptr(rax, offset1)));
        new_instruction(movsd(xmm1, dword_ptr(rax, offset2)));
        new_instruction(call(mod_double));
        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsd(dword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_ADDIi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        int value     = (asBC_INTARG(info->address + 1));

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rcx, offset1)));
        new_instruction(add(eax, value));
        new_instruction(mov(qword_ptr(rcx, offset0), eax));
    }

    void X86_64_Compiler::exec_asBC_SUBIi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        int value     = (asBC_INTARG(info->address + 1));

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rcx, offset1)));
        new_instruction(sub(eax, value));
        new_instruction(mov(qword_ptr(rcx, offset0), eax));
    }

    void X86_64_Compiler::exec_asBC_MULIi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        int value     = (asBC_INTARG(info->address + 1));

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rcx, offset1)));
        new_instruction(imul(eax, value));
        new_instruction(mov(qword_ptr(rcx, offset0), eax));
    }

    void X86_64_Compiler::exec_asBC_ADDIf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        float value   = arg_value_float(1);

        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rax, offset1)));
        new_instruction(addss(xmm0, info->insert_constant(value)));
        new_instruction(movss(dword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_SUBIf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        float value   = arg_value_float(1);

        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rax, offset1)));
        new_instruction(subss(xmm0, info->insert_constant(value)));
        new_instruction(movss(dword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_MULIf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        float value   = arg_value_float(1);

        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rax, offset1)));
        new_instruction(mulss(xmm0, info->insert_constant(value)));
        new_instruction(movss(dword_ptr(rax, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_SetG4(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_ChkRefS(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_ChkNullV(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_CALLINTF(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }


    void X86_64_Compiler::exec_asBC_iTOb(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        info->assembler.mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER));
        info->assembler.add(rax, offset0);

        for (int i = 0; i < 3; i++)
        {
            info->assembler.add(rax, 1);
            info->assembler.mov(byte_ptr(rax), static_cast<asBYTE>(0));
        }
    }

    void X86_64_Compiler::exec_asBC_iTOw(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        info->assembler.mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER));
        info->assembler.add(rax, offset0 + 2);
        info->assembler.mov(word_ptr(rax), static_cast<asWORD>(0));
    }

    void X86_64_Compiler::exec_asBC_SetV1(CompileInfo* info)
    {
        short value   = arg_value_dword();
        short offset0 = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(dword_ptr(rax, offset0), value));
    }

    void X86_64_Compiler::exec_asBC_SetV2(CompileInfo* info)
    {
        short value   = arg_value_dword();
        short offset0 = arg_offset(0);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(dword_ptr(rax, offset0), value));
    }

    void X86_64_Compiler::exec_asBC_Cast(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_i64TOi(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rdx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rax, dword_ptr(rdx, offset1)));
        new_instruction(mov(qword_ptr(rdx, offset0), eax));
    }

    void X86_64_Compiler::exec_asBC_uTOi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rdx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rdx, offset1)));
        new_instruction(mov(qword_ptr(rdx, offset0), rax));
    }

    void X86_64_Compiler::exec_asBC_iTOi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rdx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rdx, offset1)));
        new_instruction(cdqe());
        new_instruction(mov(qword_ptr(rdx, offset0), rax));
    }

    void X86_64_Compiler::exec_asBC_fTOi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rdx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rdx, offset1)));
        new_instruction(cvttss2si(rax, xmm0));
        new_instruction(mov(qword_ptr(rdx, offset0), rax));
    }

    void X86_64_Compiler::exec_asBC_dTOi64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(cvttsd2si(rbx, dword_ptr(rax, offset1)));
        new_instruction(mov(dword_ptr(rax, offset0), rbx));
    }

    void X86_64_Compiler::exec_asBC_fTOu64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(xmm0, dword_ptr(rbx, offset1)));
        new_instruction(call(float_to_uint64));
        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(dword_ptr(rbx, offset0), rax));
    }

    void X86_64_Compiler::exec_asBC_dTOu64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsd(xmm0, dword_ptr(rbx, offset0)));
        new_instruction(call(double_to_uint64));
        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(dword_ptr(rbx, offset0), rax));
    }

    void X86_64_Compiler::exec_asBC_i64TOf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(cvtsi2ss(xmm0, dword_ptr(rbx, offset1)));
        new_instruction(movss(dword_ptr(rbx, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_u64TOf(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);

        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rdi, dword_ptr(rbx, offset1)));
        new_instruction(call(uint64_to_float));
        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movss(dword_ptr(rbx, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_i64TOd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(pxor(xmm0, xmm0));
        new_instruction(cvtsi2sd(xmm0, dword_ptr(rbx, offset0)));
        new_instruction(movsd(dword_ptr(rbx, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_u64TOd(CompileInfo* info)
    {
        short offset0 = arg_offset(0);

        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rdi, dword_ptr(rbx, offset0)));
        new_instruction(call(uint64_to_double));
        new_instruction(mov(rbx, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(movsd(dword_ptr(rbx, offset0), xmm0));
    }

    void X86_64_Compiler::exec_asBC_NEGi64(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(neg(qword_ptr(rax, offset)));
    }

    void X86_64_Compiler::exec_asBC_INCi64(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(inc(qword_ptr(rax, offset)));
    }

    void X86_64_Compiler::exec_asBC_DECi64(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(dec(qword_ptr(rax, offset)));
    }

    void X86_64_Compiler::exec_asBC_BNOT64(CompileInfo* info)
    {
        short offset = arg_offset(0);
        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(not_(qword_ptr(rax, offset)));
    }

    void X86_64_Compiler::exec_asBC_ADDi64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_SUBi64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_MULi64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_DIVi64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_MODi64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BAND64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BOR64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BXOR64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BSLL64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BSRL64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_BSRA64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_CMPi64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_CMPu64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_ChkNullS(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_ClrHi(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_JitEntry(CompileInfo* info)
    {
        asUINT offset                = static_cast<asUINT>(info->assembler.offset()) - info->header_size;
        asBC_DWORDARG(info->address) = offset;
    }

    void X86_64_Compiler::exec_asBC_CallPtr(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }

    void X86_64_Compiler::exec_asBC_FuncPtr(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_LoadThisR(CompileInfo* info)
    {
        short value0 = arg_value_short(0);

        Label is_valid = info->assembler.newLabel();

        new_instruction(mov(rax, dword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rax, qword_ptr(rax)));
        new_instruction(cmp(rax, 0));

        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));

        new_instruction(bind(is_valid));
        new_instruction(add(rax, value0));
        new_instruction(mov(dword_ptr(rbp, VALUE_REGISTER), rax));
    }

    void X86_64_Compiler::exec_asBC_PshV8(CompileInfo* info)
    {
        short offset = arg_offset(0);

        new_instruction(sub(qword_ptr(rbp, STACK_POINTER_REGISTER), PTR_SIZE_1));
        new_instruction(mov(rax, qword_ptr(rbp, STACK_POINTER_REGISTER)));

        new_instruction(mov(rbx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rbx, qword_ptr(rbx, offset)));
        new_instruction(mov(qword_ptr(rax), rbx));
    }

    void X86_64_Compiler::exec_asBC_DIVu(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rcx, offset1)));
        new_instruction(mov(edx, 0));
        new_instruction(div(dword_ptr(rcx, offset2)));
        new_instruction(mov(dword_ptr(rcx, offset0), eax));
    }

    void X86_64_Compiler::exec_asBC_MODu(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(eax, dword_ptr(rcx, offset1)));
        new_instruction(mov(edx, 0));
        new_instruction(div(dword_ptr(rcx, offset2)));
        new_instruction(mov(dword_ptr(rcx, offset0), edx));
    }

    void X86_64_Compiler::exec_asBC_DIVu64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rax, dword_ptr(rcx, offset1)));
        new_instruction(mov(rdx, 0));
        new_instruction(div(dword_ptr(rcx, offset2)));
        new_instruction(mov(dword_ptr(rcx, offset0), rax));
    }

    void X86_64_Compiler::exec_asBC_MODu64(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_offset(1);
        short offset2 = arg_offset(2);

        new_instruction(mov(rcx, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(mov(rax, dword_ptr(rcx, offset1)));
        new_instruction(mov(rdx, 0));
        new_instruction(div(dword_ptr(rcx, offset2)));
        new_instruction(mov(dword_ptr(rcx, offset0), rdx));
    }

    void X86_64_Compiler::exec_asBC_LoadRObjR(CompileInfo* info)
    {
        short offset0 = arg_offset(0);
        short offset1 = arg_value_short(1);

        Label is_valid = info->assembler.newLabel();

        new_instruction(mov(rax, qword_ptr(rbp, STACK_FRAME_POINTER_REGISTER)));
        new_instruction(add(rax, offset0));
        new_instruction(mov(rbx, qword_ptr(rax)));
        new_instruction(cmp(rbx, 0));

        new_instruction(jne(is_valid));
        new_instruction(call(make_exception_nullptr_access));

        new_instruction(bind(is_valid));
        new_instruction(add(rbx, offset1));

        new_instruction(mov(ptr(rbp, VALUE_REGISTER), rbx));
    }

    void X86_64_Compiler::exec_asBC_LoadVObjR(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_RefCpyV(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_JLowZ(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_JLowNZ(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_AllocMem(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_SetListSize(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_PshListElmnt(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_SetListType(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_POWi(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_POWu(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_POWf(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_POWd(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_POWdi(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_POWi64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_POWu64(CompileInfo* info)
    {}

    void X86_64_Compiler::exec_asBC_Thiscall1(CompileInfo* info)
    {
        RETURN_CONTROL_TO_VM();
    }
}// namespace JIT
