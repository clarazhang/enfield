%skeleton "lalr1.cc"

%code requires {
    #include "enfield/Analysis/Nodes.h"
    #include "enfield/Analysis/Driver.h"
    #include "enfield/Support/WrapperVal.h"
    #include "enfield/Support/RTTI.h"

    namespace efd {
        class EfdScanner;
    };

    typedef efd::Node* NodeRef;
}

%code provides {
    #if ! defined(yyFlexLexerOnce)
    #include <FlexLexer.h>
    #endif

    #define YYDEBUG 1

    #include <stdio.h>

    /* Size of default input buffer. */
    #ifndef YY_BUF_SIZE
    #ifdef __ia64__
    /* On IA-64, the buffer size is 16k, not 8k.
     * Moreover, YY_BUF_SIZE is 2*YY_READ_BUF_SIZE in the general case.
     * Ditto for the __ia64__ case accordingly.
     */
    #define YY_BUF_SIZE 32768
    #else
    #define YY_BUF_SIZE 16384
    #endif /* __ia64__ */
    #endif

    #include <iostream>
    #include <fstream>

    namespace efd {
        class EfdScanner : public yyFlexLexer {
            public:
                EfdScanner(std::istream* iStream, std::ostream* oStream);
                yy::EfdParser::symbol_type lex();
        };
    }

    #undef YY_DECL
    #define YY_DECL \
        efd::yy::EfdParser::symbol_type efd::EfdScanner::lex()
}

%code {
    efd::EfdScanner::EfdScanner(std::istream* iStream, std::ostream* oStream)
       : yyFlexLexer(iStream, oStream) {
    }

    void efd::yy::EfdParser::error(efd::yy::location const& loc, std::string const& err) {
        std::string filename = "unknown";
        if (loc.begin.filename) filename = *loc.begin.filename;
        std::cerr << filename << ":" << loc.begin.line << ":" 
            << loc.begin.column << ": " << err << std::endl;
    }
}

%defines
%define parser_class_name {EfdParser}
%define api.token.constructor
%define api.namespace {efd::yy}
%define api.value.type variant
%define parse.assert

%parse-param { efd::ASTWrapper &ast }
%parse-param { efd::EfdScanner &scanner }

%code {
    #define _YYLEX_ scanner.lex
    #define yylex _YYLEX_
}

%locations
%initial-action {
    @$.begin.filename = &ast.mFile;
    @$.end.filename = &ast.mFile;
}

%define parse.trace true
%define parse.error verbose

%define api.token.prefix {TOK_}

%token IBMQASM INCLUDE;
%token QREG CREG;
%token OPAQUE GATE;
%token MEASURE BARRIER RESET IF;

%token U CX;
%token SIN COS TAN EXP LN SQRT;

%token EQUAL    "=="
%token ADD      "+"
%token SUB      "-"
%token MUL      "*"
%token DIV      "/"
%token POW      "^"

%token LPAR     "("
%token RPAR     ")"
%token LSBRAC   "["
%token RSBRAC   "]"
%token LCBRAC   "{"
%token RCBRAC   "}"

%token MARROW   "->"
%token COMMA    ","
%token SEMICOL  ";"

%token <efd::IntVal> INT;
%token <efd::RealVal> REAL;
%token <std::string> ID;
%token <std::string> STRING;

%token EOF 0 "end of file";

%type <NodeRef> program stmtlist stmtlist_ statement
%type <NodeRef> decl gatedecl opaquedecl qop uop ifstmt include
%type <NodeRef> barrier reset measure
%type <NodeRef> goplist
%type <NodeRef> explist explist_ exp
%type <NodeRef> anylist anylist_
%type <NodeRef> idlist idlist_
%type <NodeRef> params args arg
%type <NodeRef> id idref integer real string

%type <efd::NDUnaryOp::UOpType> unary

%left "+" "-"
%left "*" "/"
%left "^"
%right NEG
%%

%start program;

program: stmtlist                   { $$ = $1; }
       | IBMQASM real ";" stmtlist  { $$ = efd::NDQasmVersion::create($2, $4); }

stmtlist: stmtlist_ statement EOF {
                                      efd::dynCast<efd::NDStmtList>($1)->addChild($2);
                                      $$ = $1;
                                      ast.mAST = $1;
                                  }
       ;
stmtlist_: %empty                { $$ = efd::NDStmtList::create(); }
         | stmtlist_ statement   {
                                     efd::dynCast<efd::NDStmtList>($1)->addChild($2);
                                     $$ = $1;
                                 }

statement: decl         { $$ = $1; }
         | gatedecl     { $$ = $1; }
         | opaquedecl   { $$ = $1; }
         | qop          { $$ = $1; }
         | barrier      { $$ = $1; }
         | ifstmt       { $$ = $1; }
         | include      { $$ = $1; }
         ;

include: INCLUDE string ";"     {
                                    efd::ASTWrapper _ast { 
                                        efd::dynCast<efd::NDString>($2)->getVal(), 
                                        ast.mPath,
                                        nullptr 
                                    };

                                    std::ifstream ifs((_ast.mPath + _ast.mFile).c_str());
                                    if (ifs.fail()) {
                                        error(@$, "Could not open file: " + _ast.mPath + _ast.mFile);
                                        return 1;
                                    }

                                    std::cout << "Beginning include." << std::endl;
                                    efd::yy::EfdParser parser(ast, scanner);
                                    scanner.yypush_buffer_state(scanner.yy_create_buffer(&ifs, YY_BUF_SIZE));
                                    if (parser.parse()) return 1;
                                    scanner.yypop_buffer_state();
                                    std::cout << "Finished include." << std::endl;

                                    std::cout << "AST: " << ast.mAST << std::endl;
                                    $$ = efd::NDInclude::create($2, ast.mAST); 
                                }
        ;

decl: QREG id "[" integer "]" ";"   { $$ = efd::NDDecl::create(efd::NDDecl::QUANTUM, $2, $4); }
    | CREG id "[" integer "]" ";"   { $$ = efd::NDDecl::create(efd::NDDecl::CONCRETE, $2, $4); }
    ;

gatedecl: GATE id params idlist "{" goplist "}" { 
                                                    $$ = efd::NDGateDecl::create
                                                    ($2, $3, 
                                                    $4, $6);
                                                }
        ;

opaquedecl: OPAQUE id params idlist ";" { $$ = efd::NDOpaque::create($2, $3, $4); }
          ;

barrier: BARRIER idlist ";" { $$ = efd::NDQOpBarrier::create($2); }
       ;

reset: RESET arg ";"    { $$ = efd::NDQOpReset::create($2); }
     ;

measure: MEASURE arg "->" arg ";"   { $$ = efd::NDQOpMeasure::create($2, $4); }
       ;

ifstmt: IF "(" id "==" integer ")" qop  { $$ = efd::NDIfStmt::create($3, $5, $7); }

params: %empty          { $$ = efd::NDList::create(); }
      | "(" idlist ")"  { $$ = $2; }
      ;

goplist: %empty             { $$ = efd::NDGOpList::create(); }
       | goplist uop        {
                                efd::dynCast<efd::NDGOpList>($1)->addChild($2);
                                $$ = $1;
                            }
       | goplist barrier    {
                                efd::dynCast<efd::NDGOpList>($1)->addChild($2);
                                $$ = $1;
                            }
       ;

qop: uop        { $$ = $1; }
   | measure    { $$ = $1; }
   | reset      { $$ = $1; }
   ;

uop: id args anylist ";"    { $$ = efd::NDQOpGeneric::create($1, $2, $3); }
   | U args arg ";"         { $$ = efd::NDQOpU::create($2, $3); }
   | CX arg "," arg ";"     { $$ = efd::NDQOpCX::create($2, $4); }
   ;

args: %empty            { $$ = efd::NDList::create(); }
    | "(" explist ")"   { $$ = $2; }
    ;

idlist: idlist_ id      {
                            efd::dynCast<efd::NDList>($1)->addChild($2);
                            $$ = $1;
                        }
      ;
idlist_: %empty         { $$ = efd::NDList::create(); }
       | idlist_ id "," {
                            efd::dynCast<efd::NDList>($1)->addChild($2);
                            $$ = $1;
                        }
       ;

anylist: anylist_ id    {
                            efd::dynCast<efd::NDList>($1)->addChild($2);
                            $$ = $1;
                        }
       | anylist_ idref {
                            efd::dynCast<efd::NDList>($1)->addChild($2);
                            $$ = $1;
                        }
       ;
anylist_: %empty                { $$ = efd::NDList::create(); }
        | anylist_ id ","       {
                                    efd::dynCast<efd::NDList>($1)->addChild($2);
                                    $$ = $1;
                                }
        | anylist_ idref ","    {
                                    efd::dynCast<efd::NDList>($1)->addChild($2);
                                    $$ = $1;
                                }
        ;

explist: %empty         { $$ = efd::NDList::create(); }
       | explist_ exp   {
                            efd::dynCast<efd::NDList>($1)->addChild($2);
                            $$ = $1;
                        }
       ;
explist_: %empty            { $$ = efd::NDList::create(); }
        | explist_ exp ","  {
                                efd::dynCast<efd::NDList>($1)->addChild($2);
                                $$ = $1;
                            }

exp: real               { $$ = $1; }
   | integer            { $$ = $1; }
   | id                 { $$ = $1; }
   | unary "(" exp ")"  { $$ = efd::NDUnaryOp::create($1, $3); }
   | "(" exp ")"        { $$ = $2; }
   | exp "+" exp        { $$ = efd::NDBinOp::create(efd::NDBinOp::OP_ADD, $1, $3); }
   | exp "-" exp        { $$ = efd::NDBinOp::create(efd::NDBinOp::OP_SUB, $1, $3); }
   | exp "*" exp        { $$ = efd::NDBinOp::create(efd::NDBinOp::OP_MUL, $1, $3); }
   | exp "/" exp        { $$ = efd::NDBinOp::create(efd::NDBinOp::OP_DIV, $1, $3); }
   | exp "^" exp        { $$ = efd::NDBinOp::create(efd::NDBinOp::OP_POW, $1, $3); }
   | "-" exp            { $$ = efd::NDUnaryOp::create(efd::NDUnaryOp::UOP_NEG, $2); }
   ;

unary: SIN  { $$ = efd::NDUnaryOp::UOP_SIN; }
     | COS  { $$ = efd::NDUnaryOp::UOP_COS; }
     | TAN  { $$ = efd::NDUnaryOp::UOP_TAN; }
     | EXP  { $$ = efd::NDUnaryOp::UOP_EXP; }
     | LN   { $$ = efd::NDUnaryOp::UOP_LN; }
     | SQRT { $$ = efd::NDUnaryOp::UOP_SQRT; }
     ;

arg: id     { $$ = $1; }
   | idref  { $$ = $1; }
   ;

idref: id "[" integer "]" { $$ = efd::NDIdRef::create($1, $3); }
     ;

id: ID { $$ = efd::NDId::create($1); }
  ;

integer: INT { $$ = efd::NDInt::create($1); }
       ;

string: STRING { $$ = efd::NDString::create($1.substr(1, $1.length() - 2)); }

real: REAL { $$ = efd::NDReal::create($1); }
    ;

%%

static int Parse(efd::ASTWrapper& ast) {
    std::ifstream ifs((ast.mPath + ast.mFile).c_str());
    if (ifs.fail()) {
        std::cerr << "Could not open file: " << ast.mPath + ast.mFile << std::endl;
        return 1;
    }

    efd::EfdScanner scanner(&ifs, nullptr);
    efd::yy::EfdParser parser(ast, scanner);

    int ret = parser.parse();
    ifs.close();

    return ret;
}

efd::NodeRef efd::ParseFile(std::string filename, std::string path) {
    ASTWrapper ast { filename, path, nullptr };
    if (Parse(ast)) return nullptr;
    return ast.mAST;
}

efd::NodeRef efd::ParseString(std::string program) {
    std::string filename = "qasm-" + std::to_string((unsigned long long) &program) + ".qasm";
    std::string path =  "./";

    std::ofstream ofs((path + filename).c_str());
    ofs << program;
    ofs.flush();
    ofs.close();

    ASTWrapper ast { filename, path, nullptr };
    int ret = Parse(ast);
    remove((path + filename).c_str());

    if (ret) return nullptr;
    return ast.mAST;
}