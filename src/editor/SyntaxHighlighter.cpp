#include "SyntaxHighlighter.h"

#include <QFont>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextDocument>

namespace paw {

namespace {

QTextCharFormat fmt(const QColor &c, bool bold = false, bool italic = false)
{
    QTextCharFormat f;
    f.setForeground(c);
    if (bold)
        f.setFontWeight(QFont::Bold);
    if (italic)
        f.setFontItalic(true);
    return f;
}

// \b-anchored keyword alternation. Space-separated words (in one or several
// list entries) become | alternatives.
QString kw(const QStringList &words)
{
    QString joined = words.join(QLatin1Char('|'));
    joined.replace(QLatin1Char(' '), QLatin1Char('|'));
    return QStringLiteral("\\b(?:%1)\\b").arg(joined);
}

QTextCharFormat boldFmt() { return fmt(QColor(0xe4, 0xe4, 0xe7), true); }

} // namespace

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *doc) : QSyntaxHighlighter(doc) { setLanguage(QStringLiteral("plaintext")); }

void SyntaxHighlighter::setLanguage(const QString &languageId)
{
    if (m_language == languageId)
        return;
    m_language = languageId;
    rebuild();
    rehighlight();
}

void SyntaxHighlighter::addRule(const QString &pattern, const QTextCharFormat &f)
{
    Rule r;
    r.pattern = QRegularExpression(pattern);
    r.format = f;
    m_rules.push_back(r);
}

void SyntaxHighlighter::rebuild()
{
    m_rules.clear();
    m_lineComment = QRegularExpression();
    m_blockStart.clear();
    m_blockEnd.clear();

    const QTextCharFormat kwFmt = fmt(keywordColor());
    const QTextCharFormat typeFmt = fmt(typeColor());
    const QTextCharFormat strFmt = fmt(stringColor());
    const QTextCharFormat numFmt = fmt(numberColor());
    const QTextCharFormat comFmt = fmt(commentColor(), false, true);
    const QTextCharFormat fnFmt = fmt(functionColor());
    const QTextCharFormat ppFmt = fmt(preprocessorColor());

    const QString id = m_language;

    // ---- C-like family ------------------------------------------------------
    static const QStringList cFamily = {QStringLiteral("cpp"), QStringLiteral("c"),   QStringLiteral("csharp"),
                                        QStringLiteral("java"),      QStringLiteral("kotlin"), QStringLiteral("swift"),
                                        QStringLiteral("rust"),      QStringLiteral("go"),     QStringLiteral("dart"),
                                        QStringLiteral("scala"),     QStringLiteral("zig"),    QStringLiteral("cuda"),
                                        QStringLiteral("glsl"),      QStringLiteral("hlsl"),   QStringLiteral("objc"),
                                        QStringLiteral("v"),  QStringLiteral("groovy"), QStringLiteral("javascript"),
                                        QStringLiteral("typescript"), QStringLiteral("javascript react"), QStringLiteral("typescript react"),
                                        QStringLiteral("php"), QStringLiteral("fsharp"), QStringLiteral("vb")};
    static const QStringList cWords = {
        "alignas alignof and asm auto base break case catch class co_await co_return co_yield concept const consteval "
        "constexpr constinit const_cast continue decltype default delete do dynamic_cast else enum explicit export "
        "extends extern false final fn for friend goto if implements import in inline interface is let lock loop "
        "match module mut namespace new nil noexcept not null nullptr operator override private protected public pub "
        "ref register reinterpret_cast requires return sizeof static static_assert static_cast struct super switch "
        "template this throw throws trait true try typedef typeid typename union unsafe use using virtual void "
        "volatile when where while with yield async await defer chan func map package range select var const val "
        "string int long short double float bool char byte boolean object any symbol bigint number"};
    if (cFamily.contains(id) || id == QLatin1String("cuda")) {
        addRule(kw(cWords), kwFmt);
        addRule(QStringLiteral("^[\\s]*#[A-Za-z_]+"), ppFmt); // preprocessor
        addRule(QStringLiteral("\\b[A-Z][A-Za-z0-9_]*\\b"), typeFmt);
        addRule(QStringLiteral("[A-Za-z_][A-Za-z0-9_]*\\s*(?=\\()"), fnFmt);
        addRule(QStringLiteral("\\b(?:0[xXbB][0-9a-fA-F_]+|\\d[\\d_]*(?:\\.\\d+)?(?:[eE][+-]?\\d+)?[fFuUlL]*)\\b"), numFmt);
        addRule(QStringLiteral("\"(\\\\.|[^\"\\\\])*\""), strFmt);
        addRule(QStringLiteral("'(\\\\.|[^'\\\\])*'"), strFmt);
        m_blockStart = QStringLiteral("/*");
        m_blockEnd = QStringLiteral("*/");
        m_lineComment = QRegularExpression(QStringLiteral("//[^\n]*"));
    }

    // ---- Python / Ruby / Lua / Perl / Nim / Julia -----------------------------
    static const QStringList pyWords = {
        "and as assert async await break class continue def del elif else except False finally for from global if "
        "import in is lambda None nonlocal not or pass raise return True try while with yield do end elsif unless "
        "until then begin module require nil self function local repeat then then puts nil end function local"};
    if (id == QLatin1String("python") || id == QLatin1String("ruby") || id == QLatin1String("lua") || id == QLatin1String("perl")
        || id == QLatin1String("nim") || id == QLatin1String("julia")) {
        addRule(kw(pyWords), kwFmt);
        addRule(QStringLiteral("@[A-Za-z_][A-Za-z0-9_.]*"), fnFmt);
        addRule(QStringLiteral("[A-Za-z_][A-Za-z0-9_]*\\s*(?=\\()"), fnFmt);
        addRule(QStringLiteral("\\b[A-Z][A-Za-z0-9_]*\\b"), typeFmt);
        addRule(QStringLiteral("\\b(?:0[xXbBoO][0-9a-fA-F_]+|\\d[\\d_]*(?:\\.\\d+)?(?:[eE][+-]?\\d+)?)\\b"), numFmt);
        addRule(QStringLiteral("\"\"\".*?\"\"\""), strFmt);
        addRule(QStringLiteral("'''[^']*'''"), strFmt);
        addRule(QStringLiteral("\"(\\\\.|[^\"\\\\])*\""), strFmt);
        addRule(QStringLiteral("'(\\\\.|[^'\\\\])*'"), strFmt);
        if (id == QLatin1String("lua")) {
            m_blockStart = QStringLiteral("--[[");
            m_blockEnd = QStringLiteral("]]");
            m_lineComment = QRegularExpression(QStringLiteral("--[^\n]*"));
        } else {
            m_blockStart = QStringLiteral("\"\"\"");
            m_blockEnd = QStringLiteral("\"\"\"");
            m_lineComment = QRegularExpression(id == QLatin1String("lua") ? QStringLiteral("--[^\n]*")
                                : id == QLatin1String("perl")            ? QStringLiteral("#[^\n]*")
                                                                         : QStringLiteral("#[^\n]*"));
        }
    }

    // ---- JSON -----------------------------------------------------------------
    if (id == QLatin1String("json")) {
        addRule(QStringLiteral("\"(\\\\.|[^\"\\\\])*\"(?=\\s*:)"), typeFmt); // keys
        addRule(QStringLiteral("\"(\\\\.|[^\"\\\\])*\""), strFmt);
        addRule(QStringLiteral("\\b(?:true|false|null)\\b"), kwFmt);
        addRule(QStringLiteral("-?\\b\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?\\b"), numFmt);
    }

    // ---- XML / HTML -------------------------------------------------------------
    if (id == QLatin1String("xml")) {
        addRule(QStringLiteral("</?[A-Za-z][A-Za-z0-9:_.-]*|/?>"), fmt(tagColor(), true));
        addRule(QStringLiteral("[A-Za-z_][A-Za-z0-9_.:-]*(?==)"), fnFmt);
        addRule(QStringLiteral("\"[^\"]*\"|'[^']*'"), strFmt);
        m_blockStart = QStringLiteral("<!--");
        m_blockEnd = QStringLiteral("-->");
    }

    // ---- CSS / SCSS / Less -------------------------------------------------------
    if (id == QLatin1String("css") || id == QLatin1String("scss") || id == QLatin1String("less")) {
        addRule(QStringLiteral("[.#][A-Za-z_][A-Za-z0-9_-]*"), typeFmt);
        addRule(QStringLiteral("[A-Za-z-]+(?=\\s*:)"), fnFmt);
        addRule(QStringLiteral("-?-?[a-z-]+(?=\\s*:)"), fnFmt);
        addRule(QStringLiteral("#[0-9a-fA-F]{3,8}\\b"), numFmt);
        addRule(QStringLiteral("\\b\\d+(?:\\.\\d+)?(?:px|em|rem|vh|vw|s|ms|%|fr|ch|deg)?\\b"), numFmt);
        addRule(QStringLiteral("\"[^\"]*\"|'[^']*'"), strFmt);
        m_blockStart = QStringLiteral("/*");
        m_blockEnd = QStringLiteral("*/");
    }

    // ---- SQL ----------------------------------------------------------------------
    if (id == QLatin1String("sql")) {
        addRule(kw({"SELECT FROM WHERE INSERT INTO VALUES UPDATE SET DELETE CREATE TABLE ALTER DROP JOIN LEFT RIGHT "
                    "INNER OUTER ON GROUP BY ORDER HAVING LIMIT OFFSET AS AND OR NOT NULL PRIMARY KEY FOREIGN "
                    "REFERENCES INDEX VIEW IF EXISTS DISTINCT UNION ALL CASE WHEN THEN ELSE END LIKE IN IS BETWEEN "
                    "COUNT SUM AVG MIN MAX BEGIN COMMIT TRANSACTION"}),
                kwFmt);
        addRule(QStringLiteral("'(''|[^'])*'"), strFmt);
        addRule(QStringLiteral("\\b\\d+(?:\\.\\d+)?\\b"), numFmt);
        m_lineComment = QRegularExpression(QStringLiteral("--[^\n]*"));
        m_blockStart = QStringLiteral("/*");
        m_blockEnd = QStringLiteral("*/");
    }

    // ---- YAML / TOML / INI ---------------------------------------------------------
    if (id == QLatin1String("yaml") || id == QLatin1String("toml") || id == QLatin1String("ini") || id == QLatin1String("dotenv")) {
        addRule(QStringLiteral("^[\\s]*[-]?[\\s]*[A-Za-z0-9_.-]+(?=\\s*:)"), fnFmt);
        addRule(QStringLiteral("(?<=^|\\s)[A-Za-z0-9_.-]+(?=\\s*=)"), fnFmt);
        addRule(QStringLiteral("(?<=:)[\\s]*\\[[^\\]]*\\]"), strFmt);
        addRule(QStringLiteral("\"[^\"]*\"|'[^']*'"), strFmt);
        addRule(QStringLiteral("\\b(?:true|false|null|on|off|yes|no)\\b"), kwFmt);
        addRule(QStringLiteral("\\b\\d+(?:\\.\\d+)?\\b"), numFmt);
        m_lineComment = QRegularExpression(QStringLiteral("#[^\n]*"));
        if (id == QLatin1String("toml"))
            addRule(QStringLiteral("\\[+[^\\]]*\\]+"), typeFmt);
    }

    // ---- Markdown -------------------------------------------------------------------
    if (id == QLatin1String("markdown")) {
        addRule(QStringLiteral("^#{1,6}[^\n]*"), fmt(typeColor(), true));
        addRule(QStringLiteral("\\*\\*[^*\n]+\\*\\*|__[^_\n]+__"), boldFmt());
        addRule(QStringLiteral("`[^`\n]+`"), fmt(stringColor()));
        addRule(QStringLiteral("\\[[^\\]\n]*\\]\\([^)\n]*\\)"), fmt(functionColor()));
        addRule(QStringLiteral("^\\s*[-*+]\\s|^\\s*\\d+\\.\\s"), fmt(keywordColor(), true));
    }

    // ---- Shell / PowerShell / Batch ---------------------------------------------------
    if (id == QLatin1String("bash") || id == QLatin1String("powershell")) {
        addRule(kw({"if then else elif fi for while until do done case esac function return in select time coproc "
                    "begin end switch default break continue try catch finally throw param exit foreach where"}),
                kwFmt);
        addRule(QStringLiteral("\\$\\{?[A-Za-z_][A-Za-z0-9_]*\\}?|\\$\\$|\\$[0-9@?#*!]"), fmt(numberColor()));
        addRule(QStringLiteral("--?[A-Za-z][A-Za-z0-9_-]*"), fmt(typeColor()));
        addRule(QStringLiteral("\"(\\\\.|[^\"\\\\])*\""), strFmt);
        addRule(QStringLiteral("'[^']*'"), strFmt);
        m_lineComment = QRegularExpression(QStringLiteral("#[^\n]*"));
    }
    if (id == QLatin1String("batch")) {
        addRule(kw({"if else for goto call echo set exit exist not errorlevel defined equ neq lss leq gtr geq "
                    "shift start rem pause title off on do in"}),
                kwFmt);
        addRule(QStringLiteral("%[^% \t\n]+%|%%"), fmt(numberColor()));
        addRule(QStringLiteral("\"[^\"]*\""), strFmt);
        m_lineComment = QRegularExpression(QStringLiteral("(?i)^\\s*(?:rem|::)[^\n]*"));
    }

    // ---- CMake --------------------------------------------------------------------------
    if (id == QLatin1String("cmake")) {
        addRule(QStringLiteral("\\b(?:if|else|elseif|endif|foreach|endforeach|function|endfunction|macro|endmacro|"
                              "set|unset|include|project|add_executable|add_library|target_link_libraries|"
                              "target_include_directories|target_compile_definitions|target_compile_options|"
                              "find_package|message|option|enable_testing|add_test|add_subdirectory|install|"
                              "list|file|string|return)\\b"),
                kwFmt);
        addRule(QStringLiteral("\\$\\{[^}]*\\}|\\$<[^>]*>|@[^@\n]+@"), fmt(numberColor()));
        addRule(QStringLiteral("\"[^\"]*\""), strFmt);
        m_lineComment = QRegularExpression(QStringLiteral("#[^\n]*"));
    }

    // ---- Dockerfile / Makefile ------------------------------------------------------------
    if (id == QLatin1String("dockerfile")) {
        addRule(kw({"FROM RUN CMD LABEL EXPOSE ENV ADD COPY ENTRYPOINT VOLUME USER WORKDIR ARG ONBUILD STOPSIGNAL "
                    "HEALTHCHECK SHELL AS"}),
                fmt(keywordColor(), true));
        m_lineComment = QRegularExpression(QStringLiteral("#[^\n]*"));
    }
    if (id == QLatin1String("makefile")) {
        addRule(kw({"if else endif for endfor include define endifeq ifeq ifneq wildcard patsubst shell foreach "
                    "call eval export unexport override"}),
                kwFmt);
        addRule(QStringLiteral("^[A-Za-z0-9_.-]+(?=\\s*[:?+]?=)"), fnFmt);
        addRule(QStringLiteral("\\$\\([^)]*\\)|\\$\\$|\\$[@<^*?]"), fmt(numberColor()));
        m_lineComment = QRegularExpression(QStringLiteral("#[^\n]*"));
    }

    // ---- Diff / Log ------------------------------------------------------------------------
    if (id == QLatin1String("diff")) {
        addRule(QStringLiteral("^\\+[^\\n]*"), fmt(stringColor()));
        addRule(QStringLiteral("^-[^\\n]*"), fmt(tagColor()));
        addRule(QStringLiteral("^@@[^\\n]*"), fmt(functionColor()));
        addRule(QStringLiteral("^(?:diff|index|---|\\+\\+\\+)[^\n]*"), comFmt);
    }
    if (id == QLatin1String("log")) {
        addRule(QStringLiteral("\\[(?:TRACE|DEBUG)\\]|\\b(?:trace|debug)\\b"), fmt(commentColor()));
        addRule(QStringLiteral("\\[(?:INFO)\\]|\\b(?:info)\\b"), fmt(functionColor()));
        addRule(QStringLiteral("\\[(?:WARN|WARNING)\\]|\\b(?:warn(?:ing)?)\\b"), fmt(numberColor()));
        addRule(QStringLiteral("\\[(?:ERROR|FATAL)\\]|\\b(?:error|fatal)\\b"), fmt(tagColor(), true));
    }
    if (id == QLatin1String("ignore") || id == QLatin1String("plaintext")) {
        if (id == QLatin1String("ignore"))
            m_lineComment = QRegularExpression(QStringLiteral("#[^\n]*"));
    }

    // ---- Haskell / Elixir / Erlang / Clojure / OCaml -----------------------------------------
    if (id == QLatin1String("haskell") || id == QLatin1String("elixir") || id == QLatin1String("erlang") || id == QLatin1String("clojure")
        || id == QLatin1String("ocaml")) {
        addRule(kw({"module where import export case of if then else do let in data type newtype instance class "
                    "deriving defn def fn end when require use cond lambda match function receive after begin "
                    "try catch true false nil fun and or not"}),
                kwFmt);
        addRule(QStringLiteral(":[a-z][A-Za-z0-9_]*"), fmt(typeColor())); // atoms
        addRule(QStringLiteral("@[a-z][A-Za-z0-9_]*"), fnFmt);
        addRule(QStringLiteral("\"(\\\\.|[^\"\\\\])*\""), strFmt);
        addRule(QStringLiteral("'(\\\\.|[^'\\\\])*'"), strFmt);
        addRule(QStringLiteral("\\b\\d+(?:\\.\\d+)?\\b"), numFmt);
        m_lineComment = QRegularExpression(id == QLatin1String("erlang") ? QStringLiteral("%[^\n]*")
                          : id == QLatin1String("ocaml")                ? QStringLiteral("")
                                                                        : QStringLiteral("--[^\n]*"));
        if (id == QLatin1String("ocaml")) {
            m_blockStart = QStringLiteral("(*");
            m_blockEnd = QStringLiteral("*)");
        }
    }

    // ---- Assembly -----------------------------------------------------------------------------
    if (id == QLatin1String("asm")) {
        addRule(QStringLiteral("^[\\s]*[a-zA-Z_.][a-zA-Z0-9_.]*:"), fnFmt);
        addRule(kw({"mov push pop add sub mul div inc dec jmp je jne jz jnz cmp call ret leave int syscall nop xor "
                    "and or not shl shr lea lds les test enter section global extern db dw dd resb resw resd "
                    "byte word dword qword ptr"}),
                kwFmt);
        m_lineComment = QRegularExpression(QStringLiteral(";[^\n]*"));
    }
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    // Continue a block comment / triple-quote started in a previous block.
    int state = previousBlockState();
    int offset = 0;
    if (state == 1 && (!m_blockEnd.isEmpty() || m_blockStart == QLatin1String("\"\"\""))) {
        QTextCharFormat spanFmt = m_blockStart == QLatin1String("\"\"\"") ? fmt(stringColor()) : fmt(commentColor(), false, true);
        const int end = m_blockEnd.isEmpty() ? -1 : text.indexOf(m_blockEnd);
        if (end < 0) {
            setFormat(0, text.length(), spanFmt);
            setCurrentBlockState(1);
            return;
        }
        setFormat(0, end + m_blockEnd.length(), spanFmt);
        offset = end + static_cast<int>(m_blockEnd.length());
        setCurrentBlockState(0);
    } else {
        setCurrentBlockState(0);
    }

    for (const Rule &r : std::as_const(m_rules)) {
        if (!r.pattern.isValid() || r.pattern.pattern().isEmpty())
            continue;
        auto it = r.pattern.globalMatch(text, offset);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), r.format);
        }
    }

    if (!m_lineComment.pattern().isEmpty() && m_lineComment.isValid()) {
        auto it = m_lineComment.globalMatch(text, offset);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(m.capturedStart(), text.length() - m.capturedStart(), fmt(commentColor(), false, true));
            break; // everything from the first comment marker to EOL is a comment
        }
    }

    // Start of a new multi-line comment / doc string.
    if (!m_blockStart.isEmpty() && state != 1) {
        const int start = text.indexOf(m_blockStart, offset);
        if (start >= 0) {
            const bool docString = m_blockStart == QLatin1String("\"\"\"");
            const int skip = docString && text.indexOf(m_blockStart, start + 3) == start + 3 ? 6 : 3;
            const int end = text.indexOf(m_blockEnd, start + skip);
            QTextCharFormat spanFmt = docString ? fmt(stringColor()) : fmt(commentColor(), false, true);
            if (end >= 0) {
                setFormat(start, end + static_cast<int>(m_blockEnd.length()) - start, spanFmt);
            } else {
                setFormat(start, text.length() - start, spanFmt);
                setCurrentBlockState(1);
            }
        }
    }
}

} // namespace paw
