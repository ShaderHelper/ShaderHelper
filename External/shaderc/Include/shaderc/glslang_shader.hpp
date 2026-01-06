#ifndef SHADERC_GLSLANG_SHADER_HPP_
#define SHADERC_GLSLANG_SHADER_HPP_

#include <string>

#include "shaderc/shaderc.h"

// Forward declaration only. Callers that want to inspect the AST must include
// glslang headers themselves.
namespace glslang {
class TShader;
}

namespace shaderc {

// RAII wrapper around shaderc_parsed_shader_t.
//
// You can obtain a handle from shaderc_parse_into_glslang_shader() or from the
// C++ wrapper shaderc::Compiler::ParseGlslToGlslangShader(...), then wrap it:
//
//   shaderc::ParsedGlslangShader parsed(
//       compiler.ParseGlslToGlslangShader(...));
//
// The handle will be released automatically.
class ParsedGlslangShader {
 public:
  explicit ParsedGlslangShader(shaderc_parsed_shader_t parsed = nullptr)
      : parsed_(parsed) {}
  ~ParsedGlslangShader() { shaderc_parsed_shader_release(parsed_); }

  ParsedGlslangShader(ParsedGlslangShader&& other) noexcept : parsed_(nullptr) {
    *this = std::move(other);
  }
  ParsedGlslangShader& operator=(ParsedGlslangShader&& other) noexcept {
    if (this != &other) {
      shaderc_parsed_shader_release(parsed_);
      parsed_ = other.parsed_;
      other.parsed_ = nullptr;
    }
    return *this;
  }

  ParsedGlslangShader(const ParsedGlslangShader&) = delete;
  ParsedGlslangShader& operator=(const ParsedGlslangShader&) = delete;

  bool IsValid() const { return parsed_ != nullptr; }

  shaderc_compilation_status GetCompilationStatus() const {
    if (!parsed_) return shaderc_compilation_status_null_result_object;
    return shaderc_parsed_shader_get_compilation_status(parsed_);
  }

  std::string GetErrorMessage() const {
    if (!parsed_) return "";
    return shaderc_parsed_shader_get_error_message(parsed_);
  }

  size_t GetNumWarnings() const {
    if (!parsed_) return 0;
    return shaderc_parsed_shader_get_num_warnings(parsed_);
  }

  size_t GetNumErrors() const {
    if (!parsed_) return 0;
    return shaderc_parsed_shader_get_num_errors(parsed_);
  }

  glslang::TShader* GetTShader() const {
    if (!parsed_) return nullptr;
    return static_cast<glslang::TShader*>(
        shaderc_parsed_shader_get_tshader(parsed_));
  }

  shaderc_parsed_shader_t get() const { return parsed_; }

 private:
  shaderc_parsed_shader_t parsed_;
};

}  // namespace shaderc

#endif  // SHADERC_GLSLANG_SHADER_HPP_


