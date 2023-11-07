#include <angelscript.h>
#include <fstream>
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
{
    if (argc == 1)
    {
        printf("Usage: ./programm filename\n");
        return 0;
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

    JIT::X86_64_Compiler compiler;
    engine->SetJITCompiler(&compiler);

    asIScriptModule* module = engine->GetModule("Module", asGM_ALWAYS_CREATE);
    module->AddScriptSection("script", code.c_str());
    module->Build();

    asIScriptContext* context = engine->CreateContext();
    context->Prepare(module->GetFunctionByName("main"));
    context->Execute();
    context->Release();
    return 0;
}
