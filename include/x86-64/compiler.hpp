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


#pragma once
#include <angelscript.h>
#include <asmjit/asmjit.h>
#include <functional>
#include <vector>


#include <map>
#include <set>

namespace JIT
{
    using namespace asmjit::x86;
    using namespace asmjit;


    class X86_64_Compiler : public asIJITCompiler
    {
    private:
        struct LabelInfo {
            asDWORD* byte_code_address;
            Label label;
        };

        struct CompileInfo {
            Assembler assembler;
            ConstPool* const_pool;
            Label* const_pool_label;

            std::vector<LabelInfo> labels;

            asDWORD* address;
            asDWORD* begin;
            asDWORD* end;

            asUINT byte_codes;
            asUINT header_size;

            asEBCInstr instruction;

            template<typename T>
            asmjit::x86::Mem insert_constant(T value)
            {
                size_t value_offset;
                const_pool->add(&value, sizeof(value), value_offset);
                return x86::ptr(*const_pool_label, value_offset);
            }
        };

        JitRuntime _M_rt;
        void (X86_64_Compiler::*exec[static_cast<size_t>(asBC_MAXBYTECODE)])(CompileInfo*);
        const char* code_names[static_cast<size_t>(asBC_MAXBYTECODE)];
        bool _M_with_suspend;

        std::map<std::string, std::set<unsigned int>> _M_skip_instructions;

    public:
        X86_64_Compiler(bool with_suspend = false);

        int CompileFunction(asIScriptFunction* function, asJITFunction* output) override;
        void ReleaseJITFunction(asJITFunction func) override;

        void push_instruction_index_for_skip(const std::string& name, unsigned int index);

    private:
        asUINT process_instruction(CompileInfo* info);
        void init(CompileInfo* info);
        void restore_registers(CompileInfo* info);
        void save_registers(CompileInfo* info);

        size_t find_label_for_jump(CompileInfo* info);
        void bind_label_if_required(CompileInfo* info);

        asUINT instruction_size(asEBCInstr instruction);

        void exec_asBC_PopPtr(CompileInfo* info);
        void exec_asBC_PshGPtr(CompileInfo* info);
        void exec_asBC_PshC4(CompileInfo* info);
        void exec_asBC_PshV4(CompileInfo* info);
        void exec_asBC_PSF(CompileInfo* info);
        void exec_asBC_SwapPtr(CompileInfo* info);
        void exec_asBC_NOT(CompileInfo* info);
        void exec_asBC_PshG4(CompileInfo* info);
        void exec_asBC_LdGRdR4(CompileInfo* info);
        void exec_asBC_CALL(CompileInfo* info);
        void exec_asBC_RET(CompileInfo* info);
        void exec_asBC_JMP(CompileInfo* info);
        void exec_asBC_JZ(CompileInfo* info);
        void exec_asBC_JNZ(CompileInfo* info);
        void exec_asBC_JS(CompileInfo* info);
        void exec_asBC_JNS(CompileInfo* info);
        void exec_asBC_JP(CompileInfo* info);
        void exec_asBC_JNP(CompileInfo* info);
        void exec_asBC_TZ(CompileInfo* info);
        void exec_asBC_TNZ(CompileInfo* info);
        void exec_asBC_TS(CompileInfo* info);
        void exec_asBC_TNS(CompileInfo* info);
        void exec_asBC_TP(CompileInfo* info);
        void exec_asBC_TNP(CompileInfo* info);
        void exec_asBC_NEGi(CompileInfo* info);
        void exec_asBC_NEGf(CompileInfo* info);
        void exec_asBC_NEGd(CompileInfo* info);
        void exec_asBC_INCi16(CompileInfo* info);
        void exec_asBC_INCi8(CompileInfo* info);
        void exec_asBC_DECi16(CompileInfo* info);
        void exec_asBC_DECi8(CompileInfo* info);
        void exec_asBC_INCi(CompileInfo* info);
        void exec_asBC_DECi(CompileInfo* info);
        void exec_asBC_INCf(CompileInfo* info);
        void exec_asBC_DECf(CompileInfo* info);
        void exec_asBC_INCd(CompileInfo* info);
        void exec_asBC_DECd(CompileInfo* info);
        void exec_asBC_IncVi(CompileInfo* info);
        void exec_asBC_DecVi(CompileInfo* info);
        void exec_asBC_BNOT(CompileInfo* info);
        void exec_asBC_BAND(CompileInfo* info);
        void exec_asBC_BOR(CompileInfo* info);
        void exec_asBC_BXOR(CompileInfo* info);
        void exec_asBC_BSLL(CompileInfo* info);
        void exec_asBC_BSRL(CompileInfo* info);
        void exec_asBC_BSRA(CompileInfo* info);
        void exec_asBC_COPY(CompileInfo* info);
        void exec_asBC_PshC8(CompileInfo* info);
        void exec_asBC_PshVPtr(CompileInfo* info);
        void exec_asBC_RDSPtr(CompileInfo* info);
        void exec_asBC_CMPd(CompileInfo* info);
        void exec_asBC_CMPu(CompileInfo* info);
        void exec_asBC_CMPf(CompileInfo* info);
        void exec_asBC_CMPi(CompileInfo* info);
        void exec_asBC_CMPIi(CompileInfo* info);
        void exec_asBC_CMPIf(CompileInfo* info);
        void exec_asBC_CMPIu(CompileInfo* info);
        void exec_asBC_JMPP(CompileInfo* info);
        void exec_asBC_PopRPtr(CompileInfo* info);
        void exec_asBC_PshRPtr(CompileInfo* info);
        void exec_asBC_STR(CompileInfo* info);
        void exec_asBC_CALLSYS(CompileInfo* info);
        void exec_asBC_CALLBND(CompileInfo* info);
        void exec_asBC_SUSPEND(CompileInfo* info);
        void exec_asBC_ALLOC(CompileInfo* info);
        void exec_asBC_FREE(CompileInfo* info);
        void exec_asBC_LOADOBJ(CompileInfo* info);
        void exec_asBC_STOREOBJ(CompileInfo* info);
        void exec_asBC_GETOBJ(CompileInfo* info);
        void exec_asBC_REFCPY(CompileInfo* info);
        void exec_asBC_CHKREF(CompileInfo* info);
        void exec_asBC_GETOBJREF(CompileInfo* info);
        void exec_asBC_GETREF(CompileInfo* info);
        void exec_asBC_PshNull(CompileInfo* info);
        void exec_asBC_ClrVPtr(CompileInfo* info);
        void exec_asBC_OBJTYPE(CompileInfo* info);
        void exec_asBC_TYPEID(CompileInfo* info);
        void exec_asBC_SetV4(CompileInfo* info);
        void exec_asBC_SetV8(CompileInfo* info);
        void exec_asBC_ADDSi(CompileInfo* info);
        void exec_asBC_CpyVtoV4(CompileInfo* info);
        void exec_asBC_CpyVtoV8(CompileInfo* info);
        void exec_asBC_CpyVtoR4(CompileInfo* info);
        void exec_asBC_CpyVtoR8(CompileInfo* info);
        void exec_asBC_CpyVtoG4(CompileInfo* info);
        void exec_asBC_CpyRtoV4(CompileInfo* info);
        void exec_asBC_CpyRtoV8(CompileInfo* info);
        void exec_asBC_CpyGtoV4(CompileInfo* info);
        void exec_asBC_WRTV1(CompileInfo* info);
        void exec_asBC_WRTV2(CompileInfo* info);
        void exec_asBC_WRTV4(CompileInfo* info);
        void exec_asBC_WRTV8(CompileInfo* info);
        void exec_asBC_RDR1(CompileInfo* info);
        void exec_asBC_RDR2(CompileInfo* info);
        void exec_asBC_RDR4(CompileInfo* info);
        void exec_asBC_RDR8(CompileInfo* info);
        void exec_asBC_LDG(CompileInfo* info);
        void exec_asBC_LDV(CompileInfo* info);
        void exec_asBC_PGA(CompileInfo* info);
        void exec_asBC_CmpPtr(CompileInfo* info);
        void exec_asBC_VAR(CompileInfo* info);
        void exec_asBC_iTOf(CompileInfo* info);
        void exec_asBC_fTOi(CompileInfo* info);
        void exec_asBC_uTOf(CompileInfo* info);
        void exec_asBC_fTOu(CompileInfo* info);
        void exec_asBC_sbTOi(CompileInfo* info);
        void exec_asBC_swTOi(CompileInfo* info);
        void exec_asBC_ubTOi(CompileInfo* info);
        void exec_asBC_uwTOi(CompileInfo* info);
        void exec_asBC_dTOi(CompileInfo* info);
        void exec_asBC_dTOu(CompileInfo* info);
        void exec_asBC_dTOf(CompileInfo* info);
        void exec_asBC_iTOd(CompileInfo* info);
        void exec_asBC_uTOd(CompileInfo* info);
        void exec_asBC_fTOd(CompileInfo* info);
        void exec_asBC_ADDi(CompileInfo* info);
        void exec_asBC_SUBi(CompileInfo* info);
        void exec_asBC_MULi(CompileInfo* info);
        void exec_asBC_DIVi(CompileInfo* info);
        void exec_asBC_MODi(CompileInfo* info);
        void exec_asBC_ADDf(CompileInfo* info);
        void exec_asBC_SUBf(CompileInfo* info);
        void exec_asBC_MULf(CompileInfo* info);
        void exec_asBC_DIVf(CompileInfo* info);
        void exec_asBC_MODf(CompileInfo* info);
        void exec_asBC_ADDd(CompileInfo* info);
        void exec_asBC_SUBd(CompileInfo* info);
        void exec_asBC_MULd(CompileInfo* info);
        void exec_asBC_DIVd(CompileInfo* info);
        void exec_asBC_MODd(CompileInfo* info);
        void exec_asBC_ADDIi(CompileInfo* info);
        void exec_asBC_SUBIi(CompileInfo* info);
        void exec_asBC_MULIi(CompileInfo* info);
        void exec_asBC_ADDIf(CompileInfo* info);
        void exec_asBC_SUBIf(CompileInfo* info);
        void exec_asBC_MULIf(CompileInfo* info);
        void exec_asBC_SetG4(CompileInfo* info);
        void exec_asBC_ChkRefS(CompileInfo* info);
        void exec_asBC_ChkNullV(CompileInfo* info);
        void exec_asBC_CALLINTF(CompileInfo* info);
        void exec_asBC_iTOb(CompileInfo* info);
        void exec_asBC_iTOw(CompileInfo* info);
        void exec_asBC_SetV1(CompileInfo* info);
        void exec_asBC_SetV2(CompileInfo* info);
        void exec_asBC_Cast(CompileInfo* info);
        void exec_asBC_i64TOi(CompileInfo* info);
        void exec_asBC_uTOi64(CompileInfo* info);
        void exec_asBC_iTOi64(CompileInfo* info);
        void exec_asBC_fTOi64(CompileInfo* info);
        void exec_asBC_dTOi64(CompileInfo* info);
        void exec_asBC_fTOu64(CompileInfo* info);
        void exec_asBC_dTOu64(CompileInfo* info);
        void exec_asBC_i64TOf(CompileInfo* info);
        void exec_asBC_u64TOf(CompileInfo* info);
        void exec_asBC_i64TOd(CompileInfo* info);
        void exec_asBC_u64TOd(CompileInfo* info);
        void exec_asBC_NEGi64(CompileInfo* info);
        void exec_asBC_INCi64(CompileInfo* info);
        void exec_asBC_DECi64(CompileInfo* info);
        void exec_asBC_BNOT64(CompileInfo* info);
        void exec_asBC_ADDi64(CompileInfo* info);
        void exec_asBC_SUBi64(CompileInfo* info);
        void exec_asBC_MULi64(CompileInfo* info);
        void exec_asBC_DIVi64(CompileInfo* info);
        void exec_asBC_MODi64(CompileInfo* info);
        void exec_asBC_BAND64(CompileInfo* info);
        void exec_asBC_BOR64(CompileInfo* info);
        void exec_asBC_BXOR64(CompileInfo* info);
        void exec_asBC_BSLL64(CompileInfo* info);
        void exec_asBC_BSRL64(CompileInfo* info);
        void exec_asBC_BSRA64(CompileInfo* info);
        void exec_asBC_CMPi64(CompileInfo* info);
        void exec_asBC_CMPu64(CompileInfo* info);
        void exec_asBC_ChkNullS(CompileInfo* info);
        void exec_asBC_ClrHi(CompileInfo* info);
        void exec_asBC_JitEntry(CompileInfo* info);
        void exec_asBC_CallPtr(CompileInfo* info);
        void exec_asBC_FuncPtr(CompileInfo* info);
        void exec_asBC_LoadThisR(CompileInfo* info);
        void exec_asBC_PshV8(CompileInfo* info);
        void exec_asBC_DIVu(CompileInfo* info);
        void exec_asBC_MODu(CompileInfo* info);
        void exec_asBC_DIVu64(CompileInfo* info);
        void exec_asBC_MODu64(CompileInfo* info);
        void exec_asBC_LoadRObjR(CompileInfo* info);
        void exec_asBC_LoadVObjR(CompileInfo* info);
        void exec_asBC_RefCpyV(CompileInfo* info);
        void exec_asBC_JLowZ(CompileInfo* info);
        void exec_asBC_JLowNZ(CompileInfo* info);
        void exec_asBC_AllocMem(CompileInfo* info);
        void exec_asBC_SetListSize(CompileInfo* info);
        void exec_asBC_PshListElmnt(CompileInfo* info);
        void exec_asBC_SetListType(CompileInfo* info);
        void exec_asBC_POWi(CompileInfo* info);
        void exec_asBC_POWu(CompileInfo* info);
        void exec_asBC_POWf(CompileInfo* info);
        void exec_asBC_POWd(CompileInfo* info);
        void exec_asBC_POWdi(CompileInfo* info);
        void exec_asBC_POWi64(CompileInfo* info);
        void exec_asBC_POWu64(CompileInfo* info);
        void exec_asBC_Thiscall1(CompileInfo* info);
    };

}// namespace JIT
