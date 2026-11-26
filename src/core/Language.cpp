#include "Language.h"

#include <QFileInfo>
#include <QHash>

namespace paw {
namespace Language {

namespace {
struct Entry { const char *id; const char *name; };

const QHash<QString, Entry> &byExtension()
{
    static const QHash<QString, Entry> map = {
        {"lua", {"lua", "Lua"}}, {"py", {"python", "Python"}}, {"pyw", {"python", "Python"}},
        {"js", {"javascript", "JavaScript"}}, {"mjs", {"javascript", "JavaScript"}}, {"cjs", {"javascript", "JavaScript"}},
        {"jsx", {"javascript", "JavaScript React"}}, {"ts", {"typescript", "TypeScript"}}, {"tsx", {"typescript", "TypeScript React"}},
        {"c", {"c", "C"}}, {"h", {"c", "C"}},
        {"cpp", {"cpp", "C++"}}, {"cc", {"cpp", "C++"}}, {"cxx", {"cpp", "C++"}}, {"hpp", {"cpp", "C++"}}, {"hh", {"cpp", "C++"}}, {"hxx", {"cpp", "C++"}},
        {"cs", {"csharp", "C#"}}, {"java", {"java", "Java"}}, {"kt", {"kotlin", "Kotlin"}}, {"kts", {"kotlin", "Kotlin"}},
        {"swift", {"swift", "Swift"}}, {"rs", {"rust", "Rust"}}, {"go", {"go", "Go"}},
        {"php", {"php", "PHP"}}, {"rb", {"ruby", "Ruby"}}, {"pl", {"perl", "Perl"}}, {"pm", {"perl", "Perl"}},
        {"dart", {"dart", "Dart"}}, {"r", {"r", "R"}}, {"scala", {"scala", "Scala"}}, {"hs", {"haskell", "Haskell"}},
        {"ex", {"elixir", "Elixir"}}, {"exs", {"elixir", "Elixir"}}, {"erl", {"erlang", "Erlang"}}, {"hrl", {"erlang", "Erlang"}},
        {"zig", {"zig", "Zig"}}, {"asm", {"asm", "Assembly"}}, {"s", {"asm", "Assembly"}}, {"nasm", {"asm", "Assembly"}},
        {"sql", {"sql", "SQL"}}, {"html", {"html", "HTML"}}, {"htm", {"html", "HTML"}}, {"css", {"css", "CSS"}}, {"scss", {"scss", "SCSS"}},
        {"sass", {"scss", "Sass"}}, {"less", {"less", "Less"}}, {"json", {"json", "JSON"}}, {"jsonc", {"json", "JSON with Comments"}},
        {"xml", {"xml", "XML"}}, {"svg", {"xml", "SVG"}}, {"yaml", {"yaml", "YAML"}}, {"yml", {"yaml", "YAML"}},
        {"toml", {"toml", "TOML"}}, {"md", {"markdown", "Markdown"}}, {"markdown", {"markdown", "Markdown"}},
        {"sh", {"bash", "Shell Script"}}, {"bash", {"bash", "Shell Script"}}, {"zsh", {"bash", "Shell Script"}},
        {"ps1", {"powershell", "PowerShell"}}, {"psm1", {"powershell", "PowerShell"}},
        {"bat", {"batch", "Batch"}}, {"cmd", {"batch", "Batch"}}, {"cmake", {"cmake", "CMake"}},
        {"ini", {"ini", "INI"}}, {"cfg", {"ini", "INI"}}, {"conf", {"ini", "Config"}}, {"txt", {"plaintext", "Plain Text"}},
        {"vue", {"vue", "Vue"}}, {"gradle", {"groovy", "Gradle"}}, {"groovy", {"groovy", "Groovy"}},
        {"tf", {"terraform", "Terraform"}}, {"proto", {"proto", "Protocol Buffers"}}, {"graphql", {"graphql", "GraphQL"}},
        {"nim", {"nim", "Nim"}}, {"jl", {"julia", "Julia"}}, {"clj", {"clojure", "Clojure"}}, {"ml", {"ocaml", "OCaml"}},
        {"fs", {"fsharp", "F#"}}, {"vb", {"vb", "Visual Basic"}}, {"m", {"objc", "Objective-C"}}, {"mm", {"objc", "Objective-C++"}},
        {"cu", {"cuda", "CUDA"}}, {"glsl", {"glsl", "GLSL"}}, {"hlsl", {"hlsl", "HLSL"}}, {"v", {"verilog", "Verilog"}},
        {"tex", {"latex", "LaTeX"}}, {"diff", {"diff", "Diff"}}, {"patch", {"diff", "Diff"}}, {"log", {"log", "Log"}},
    };
    return map;
}

const QHash<QString, Entry> &byFileName()
{
    static const QHash<QString, Entry> map = {
        {"dockerfile", {"dockerfile", "Dockerfile"}}, {"makefile", {"makefile", "Makefile"}}, {"gnumakefile", {"makefile", "Makefile"}},
        {"cmakelists.txt", {"cmake", "CMake"}}, {"cargo.toml", {"toml", "TOML"}}, {"cargo.lock", {"toml", "TOML"}},
        {"pyproject.toml", {"toml", "TOML"}}, {".gitignore", {"ignore", "Ignore File"}}, {".gitattributes", {"ignore", "Git Attributes"}},
        {".dockerignore", {"ignore", "Ignore File"}}, {".editorconfig", {"ini", "EditorConfig"}}, {".npmrc", {"ini", "INI"}},
        {".prettierrc", {"json", "JSON"}}, {".babelrc", {"json", "JSON"}}, {".eslintrc", {"json", "JSON"}},
        {".htaccess", {"apache", "Apache Config"}}, {".env", {"dotenv", "Environment"}}, {".bashrc", {"bash", "Shell Script"}},
        {".zshrc", {"bash", "Shell Script"}}, {"requirements.txt", {"plaintext", "Plain Text"}},
    };
    return map;
}
} // namespace

LanguageInfo forFile(const QString &path)
{
    const QFileInfo fi(path);
    const QString lowerName = fi.fileName().toLower();
    if (auto it = byFileName().constFind(lowerName); it != byFileName().constEnd())
        return {QString::fromLatin1(it->id), QString::fromLatin1(it->name)};
    if (lowerName.startsWith(QStringLiteral(".env")))
        return {QStringLiteral("dotenv"), QStringLiteral("Environment")};
    const QString ext = fi.suffix().toLower();
    if (auto it = byExtension().constFind(ext); it != byExtension().constEnd())
        return {QString::fromLatin1(it->id), QString::fromLatin1(it->name)};
    return {QStringLiteral("plaintext"), QStringLiteral("Plain Text")};
}

} // namespace Language
} // namespace paw
