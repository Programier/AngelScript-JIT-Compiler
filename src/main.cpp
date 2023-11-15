#include <angelscript.h>
#include <chrono>
#include <fstream>
#include <arm64/compiler.hpp>
#include <x86-64/compiler.hpp>


void print(const std::string& str)
{
    printf("%s\n", str.c_str());
}


void message_callback(const asSMessageInfo* msg, void* param)
{
    printf("Script message: %s\n", msg->message);
}


int main(int argc, char** argv)
try
{
    if (argc == 1)
    {
        printf("Usage: ./program <file>\n");
        return -1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open())
    {
        printf("Cannot open file '%s'\n", argv[1]);
        return -1;
    }


    std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    asIScriptEngine* engine = asCreateScriptEngine();
    engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
    engine->SetMessageCallback(asFUNCTION(message_callback), 0, asCALL_CDECL);
    asInitializeAddons(engine);

    engine->RegisterGlobalFunction("void print(const string& in)", asFUNCTION(print), asCALL_CDECL);
#if defined(__aarch64__)
    JIT::ARM64_Compiler compiler;
#else
    JIT::X86_64_Compiler compiler;
#endif
    engine->SetJITCompiler(&compiler);

    std::string current_name = "";
    for (int i = 2; i < argc; i++)
    {
        try
        {
            unsigned int index = static_cast<unsigned int>(std::stoi(argv[i]));
            compiler.push_instruction_index_for_skip(current_name, index);
        }
        catch (...)
        {
            current_name = argv[i];
        }
    }

    asIScriptModule* module = engine->GetModule("Module", asGM_ALWAYS_CREATE);
    module->AddScriptSection("script", code.c_str());
    module->Build();

    asIScriptContext* context = engine->CreateContext();
    context->Prepare(module->GetFunctionByName("main"));

    auto begin = std::chrono::steady_clock::now();
    context->Execute();
    auto end = std::chrono::steady_clock::now();

    printf("Exec time: %zu milliseconds\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
    context->Release();

    return 0;
}
catch (const std::exception& e)
{
    printf("Exception: %s\n", e.what());
    return -1;
}
