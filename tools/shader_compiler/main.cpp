#include <d3dcompiler.h>
#include <cstdio>
#include <vector>
#include <string_view>
#include <fstream>

int main(int argc, char* argv[])
{
    if (argc <= 1) {
        printf(
            "Usage: shader_compiler [options...] hlsl_file\n\n"
            "Available options:\n"
            "-o output_file output file name\n"
            "-e entrypoint  sets shader main function\n"
            "-t target      sets shader target (e.g. vs_2_0)\n"
            "-Dname=value   adds macro-definition\n"
        );
        return 1;
    }

    std::string hlsl_filename;
    std::string output_filename;
    std::string entrypoint{"main"};
    std::string target;
    DWORD flags1 = 0;
    DWORD flags2 = 0;
    std::vector<D3D_SHADER_MACRO> macros;

    for (int i = 1; i < argc; ++i) {
        char* arg = argv[i];
        std::string_view arg_sv{arg};
        if (arg_sv.substr(0, 2) == "-D") {
            auto eq_pos = arg_sv.find("=", 2);
            if (eq_pos == std::string_view::npos) {
                D3D_SHADER_MACRO macro = {arg + 2, nullptr};
                macros.push_back(macro);
            }
            else {
                arg[eq_pos] = '\0';
                D3D_SHADER_MACRO macro = {arg + 2, arg + eq_pos + 1};
                macros.push_back(macro);
            }
        }
        else if (arg_sv == "-e" && i + 1 < argc) {
            entrypoint = argv[++i];
        }
        else if (arg_sv == "-t" && i + 1 < argc) {
            target = argv[++i];
        }
        else if (arg_sv == "-o" && i + 1 < argc) {
            output_filename = argv[++i];
        }
        else if (arg_sv == "-O1") {
            flags1 |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
        }
        else if (arg_sv == "-O2") {
            flags1 |= D3DCOMPILE_OPTIMIZATION_LEVEL2;
        }
        else if (arg_sv == "-O3") {
            flags1 |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
        }
        else if (arg_sv[0] == '-') {
            printf("Unrecognized option: %s\n", arg);
        }
        else if (hlsl_filename.empty()) {
            hlsl_filename = arg_sv;
        }
        else if (output_filename.empty()) {
            output_filename = arg_sv;
        }
        else {
            printf("Unexpected argument: %s\n", arg);
        }
    }

    if (output_filename.empty()) {
        output_filename = hlsl_filename + ".bin";
    }

    D3D_SHADER_MACRO null_macro = {nullptr, nullptr};
    macros.push_back(null_macro);

    std::wstring hlsl_filename_w;
    hlsl_filename_w.resize(hlsl_filename.size() + 1);
    int num = mbstowcs(hlsl_filename_w.data(), hlsl_filename.c_str(), hlsl_filename.size() + 1);
    hlsl_filename_w.resize(num);
    ID3DBlob* shader_bytecode = nullptr;
    ID3DBlob* errors = nullptr;
    // HRESULT hr = D3DCompileFromFile(
    //     hlsl_filename_w.c_str(),
    //     macros.data(),
    //     nullptr,
    //     entrypoint.c_str(),
    //     target.c_str(),
    //     flags1,
    //     flags2,
    //     &shader_bytecode,
    //     &errors
    // );
    std::fstream input_file{hlsl_filename.c_str(), std::ios_base::in | std::ios_base::binary};
    std::string hlsl_code((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());

    HRESULT hr = D3DCompile(
        hlsl_code.data(),
        hlsl_code.size(),
        hlsl_filename.c_str(),
        macros.data(),
        nullptr,
        entrypoint.c_str(),
        target.c_str(),
        flags1,
        flags2,
        &shader_bytecode,
        &errors
    );
    if (errors) {
        if (errors->GetBufferSize() > 0) {
            auto errors_size = errors->GetBufferSize();
            auto errors_ptr = errors->GetBufferPointer();
            printf("Error messages:\n%*s\n", static_cast<int>(errors_size), static_cast<char*>(errors_ptr));
        }
        errors->Release();
    }

    if (FAILED(hr)) {
        printf("D3DCompileFromFile failed: %lx\n", hr);
        return 1;
    }

    auto shader_size = shader_bytecode->GetBufferSize();
    auto shader_data = shader_bytecode->GetBufferPointer();
    std::fstream output_file{output_filename.c_str(), std::ios_base::out | std::ios_base::binary};
    output_file.write(static_cast<char*>(shader_data), shader_size);
    shader_bytecode->Release();
    printf("Shader byte code size: %d\n", shader_size);
    return 0;
}
