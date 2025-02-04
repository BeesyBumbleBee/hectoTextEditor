#ifndef _HECTO_SYNTAX_H
#define _HECTO_SYNTAX_H

struct editorSyntax {
	char *filetype;
	char **filematch;
	char **keywords;
	char *singleline_comment_start;
	char *multiline_comment_start;
	char *multiline_comment_end;
	char *custom_line_start;
	int flags;
};

enum editorHighlight {
	HL_NORMAL = 0,
	HL_COMMENT,
	HL_MLCOMMENT,
	HL_KEYWORD1,
	HL_KEYWORD2,
	HL_STRING,
	HL_NUMBER,
	HL_MATCH,
	HL_SELECT,
	HL_CUSTOM
};

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)


char *C_HL_extensions[] = { ".c", ".h", NULL };
char *C_HL_keywords[] = {
	"alignas", "alignof", "auto", "break", "case", "const", "constexpr",
	"continue", "default", "do", "else", "enum", "extern", "false", "for",
	"goto", "if", "inline", "nullptr", "register", "restrict",
	"return", "sizeof", "static", "static_assert", "struct", "switch",
	"thread_local", "true", "typedef", "typeof", "typeof_unqual", "union",
	"void", "volatile", "while",
	
	"int|", "long|", "double|", "float|", "char|", "unsinged|", "signed|",
	"void|", NULL
};

char *Cpp_HL_extensions[] = { ".cpp", ".hpp", NULL };
char *Cpp_HL_keywords[] = {
        "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel",
	"atomic_commit", "atomic_noexcept",  "auto", "bitand", "bitor", 
	"break", "case", "catch", "class", "compl", "concept", "const", "constexpr",
        "consteval", "constinit", "const_cast", "continue", "co_await", "co_return",
	"co_yield", "decltype", "default", "do", "dynamic_cast", "else", "enum", 
	"explicit", "extern", "false", "for", "friend", "goto", "if", "inline", "mutable",
	"namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or",
	"or_eq", "private", "protected", "public", "reflexpr", "register", "reinterpret_cast",
	"restrict", "requires", "return", "sizeof", "static", "static_assert", "static_cast",
	 "struct", "switch", "synchronized", "template", "this", "thread_local", "true",
	"throw", "try", "typeid", "typedef", "typename", "union", "using", "virtual",
        "void", "volatile", "while", "xor", "xor_eq",
        
 	"char|", "char8_t|", "char16_t|", "char32_t|", "wchar_t|",        
	"int|", "long|", "double|", "float|", "char|", "unsinged|", "signed|",
        "void|", NULL
};

char *Go_HL_extensions[] = { ".go", NULL };
char *Go_HL_keywords[] = {
	"break", "default", "func", "interface", "select", "case",
	"defer", "go", "map", "struct", "chan", "else", "goto", "package",
	"switch", "const", "falltrough", "if", "range", "type", "continue",
	"for", "import", "return", "var",

	"bool|", "uint|", "int|", "uintptr|"
	"uint8|", "uint16|", "uint32|", "uint64|",
	"int8|", "int16|", "int32|", "int64|",
	"float32|", "float64|",
	"complex64|", "complex128|",
	"byte|", "rune|", "string|", NULL
};

char *Java_HL_extensions[] = { ".java", NULL };
char *Java_HL_keywords[] = {
	"abstract", "assert", "break", "case", "catch",
	"class", "const", "continue", "default", "do", "else",
	"enum", "extends", "final", "finally", "for", "goto", 
	"if", "implements", "import", "instanceof", "interface",
	"native", "new", "package", "private", "protected", "public",
	"return", "static", "strictfp", "super", "switch", "synchronized",
	"this", "throw", "throws", "transient", "try", "volatile", "while"

	"boolean|", "byte|", "char|", "double|", "float|", "int|",
	"long|", "short|", "void|", NULL
};

char *Python_HL_extensions[] = { ".py", NULL };
char *Python_HL_keywords[] = {
	"False", "None", "True", "and", "as", "assert",
	"async", "await", "break", "class", "continue",
	"def", "del", "elif", "else", "except", "finally",
	"for", "from", "global", "if", "import", "in", "is",
	"lambda", "nonlocal", "not", "or", "pass", "raise",
	"return", "try", "while", "with", "yield", NULL
};

// A collection of filetypes with builtin syntax highlighting
struct editorSyntax HLDB[] = {
	{
		"c",
		C_HL_extensions,
		C_HL_keywords,
		"//", "/*", "*/",
		"#",
		HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
	},
	{
		"c++",
		Cpp_HL_extensions,
		Cpp_HL_keywords,
		"//", "/*", "*/",
		"#",
		HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
	},
	{
		"go",
		Go_HL_extensions,
		Go_HL_keywords,
		"//", "/*", "*/",
		NULL,
		HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
	},
	{
		"java",
		Java_HL_extensions,
		Java_HL_keywords,
		"//", "/*", "*/",
		"@",
		HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
	},
	{
		"python",
		Python_HL_extensions,
		Python_HL_keywords,
		"#", "\'\'\'", "\'\'\'",
		"@",
		HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
	}
};

//}

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

#endif
