// JavaExample.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "JavaLexer.h"
#include "JavaParser.h"
#include "JavaParser_top_level_ast.h"

int main()
{
    JavaLexer lexer; // Create the lexer
    lexer.reset(LR"(test2.java)", 8);
    JavaParser parser(lexer.getILexStream()); // Create the parser
    lexer.lexer(nullptr, parser.getIPrsStream());
    JavaParser_top_level_ast::Ast* ast = (JavaParser_top_level_ast::Ast*)parser.parser(nullptr);
    std::cout << "Hello World!\n";
}
