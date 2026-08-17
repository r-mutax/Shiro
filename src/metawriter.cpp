#include "metawriter.hpp"
#include <fstream>
#include <cstdint>
#include <iomanip>

void MetaWriter::writeHeader(std::string_view src){
    out << "// shiro-interface\n";
    out << "// source_file: " << srcname << "\n";
    out << "// source_hash: " << calculateHash(src) << "\n\n";
}

void MetaWriter::writePub(){
    out << "// public definition\n";
    for(auto [name, sym] : global_scope->symbols){
        if(sym.pub){
            out << "pub ";
            switch(sym.kind){
                case Symbol::VARIABLE:
                    // now shiro is not supported global variables
                    break;
                case Symbol::FUNCTION:
                    writeFunctionDeclare(sym);
                    break;
                case Symbol::TYPE:
                    if(sym.type_info->isStruct()){
                        writeStructDeclare(sym);
                    }
                    break;
            }
        }
    }
}
void MetaWriter::writePrivate(){
    out << "// private definition\n";
    for(auto [name, sym] : global_scope->symbols){
        if(!sym.pub){
            switch(sym.kind){
                case Symbol::VARIABLE:
                    // now shiro is not supported global variables
                    break;
                case Symbol::FUNCTION:
                    writeFunctionDeclare(sym);
                    break;
                case Symbol::TYPE:
                    if(sym.type_info->isStruct()){
                        writeStructDeclare(sym);
                    }
                    break;
            }
        }
    }    
}

void MetaWriter::writeFunctionDeclare(Symbol& sym){
    out << "fn " << sym.name << "(";
    for(size_t i = 0; i < sym.params.size(); i++){
        auto param = sym.params[i];
        out << param->name << ": " << param->type_info->name;
        if(i < sym.params.size() - 1){
            out << ", ";
        }
    }
    out << ") -> " << sym.type_info->name << ";\n";
}

void MetaWriter::writeMethodDeclare(Symbol& method, std::string strct_name){
    // remove "<struct>__" from method name
    std::string method_name = method.name.substr(strct_name.size() + 2);

    out << "fn " << method_name << "(";
    for(size_t i = 0; i < method.params.size(); i++){
        auto param = method.params[i];
        out << param->name << ": " << param->type_info->name;
        if(i < method.params.size() - 1){
            out << ", ";
        }
    }
    out << ") -> " << method.type_info->name << ";\n";
}

void MetaWriter::writeStructDeclare(Symbol& sym){
    out << "struct " << sym.name << " {\n";
    for(auto [name, member] : sym.type_info->scope->symbols){
        out << "  ";
        if(member.pub){
            out << "pub ";
        }

        if(member.kind == Symbol::VARIABLE){
            out << name << ": " << member.type_info->name << ";\n";
        } else if(member.kind == Symbol::FUNCTION){
            writeMethodDeclare(member, sym.name);
        }
    }
    out << "};\n";
}

void MetaWriter::write(const Scope* global_scope, std::string_view src) {
    
    out.open(meta_filename);

    if (!out.is_open()) {
        reporter.reportError({0, 0}, "Failed to open meta file");
        return;
    }

    this->global_scope = global_scope;

    writeHeader(src);
    writePub();
    out << "\n";
    writePrivate();
}

std::string MetaWriter::calculateHash(std::string_view content){
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (char c : content) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 0x100000001b3ULL;
    }
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}