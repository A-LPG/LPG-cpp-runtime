
#pragma once

#include <iostream>

#include "AstPoolHolder.h"
#include "BacktrackingParser.h"
#include "DeterministicParser.h"
#include "diagnose.h"
#include "ErrorToken.h"
#include "Exception.h"
#include "IAbstractArrayList.h"
#include "IAst.h"
#include "IAstVisitor.h"
#include "ILexStream.h"
#include "JavaParsersym.h"
#include "JavaParserprs.h"
#include "Object.h"
#include "ParseTable.h"
#include "PrsStream.h"
#include "RuleAction.h"
#include "IcuUtil.h"
#include "stringex.h"

    //#line 17 "GJavaParser.g



    //#line 187 "btParserTemplateF.gi


namespace ExprParser_top_level_ast {
	struct Ast;
}

 struct JavaParser :public Object ,public RuleAction
{


    PrsStream* prsStream = nullptr;
    ~JavaParser (){
        delete prsStream;
        delete btParser;
    }
     bool unimplementedSymbolsWarning = false;

     inline static ParseTable* prsTable = new JavaParserprs();
     ParseTable* getParseTable() { return prsTable; }

     BacktrackingParser* btParser = nullptr;
     BacktrackingParser* getParser() { return btParser; }

     void setResult(Object* object) { btParser->setSym1(object); }
     Object* getRhsSym(int i) { return btParser->getSym(i); }

     int getRhsTokenIndex(int i) { return btParser->getToken(i); }
     IToken* getRhsIToken(int i) { return prsStream->getIToken(getRhsTokenIndex(i)); }
    
     int getRhsFirstTokenIndex(int i) { return btParser->getFirstToken(i); }
     IToken* getRhsFirstIToken(int i) { return prsStream->getIToken(getRhsFirstTokenIndex(i)); }

     int getRhsLastTokenIndex(int i) { return btParser->getLastToken(i); }
     IToken* getRhsLastIToken(int i) { return prsStream->getIToken(getRhsLastTokenIndex(i)); }

     int getLeftSpan() { return btParser->getFirstToken(); }
     IToken* getLeftIToken()  { return prsStream->getIToken(getLeftSpan()); }

     int getRightSpan() { return btParser->getLastToken(); }
     IToken* getRightIToken() { return prsStream->getIToken(getRightSpan()); }

     int getRhsErrorTokenIndex(int i)
    {
        int index = btParser->getToken(i);
        IToken* err = prsStream->getIToken(index);
        return ( dynamic_cast<ErrorToken*>(err) ? index : 0);
    }
     ErrorToken * getRhsErrorIToken(int i)
    {
        int index = btParser->getToken(i);
        IToken* err = prsStream->getIToken(index);
        return (ErrorToken*) ( dynamic_cast<ErrorToken*>(err) ? err : nullptr);
    }

     void reset(ILexStream* lexStream)
    {
        delete prsStream;
        prsStream = new PrsStream(lexStream);
        btParser->reset(prsStream);

        try
        {
            prsStream->remapTerminalSymbols(orderedTerminalSymbols(), prsTable->getEoftSymbol());
        }
        catch (NullExportedSymbolsException& e) {
        }
        catch (NullTerminalSymbolsException& e) {
        }
        catch (UnimplementedTerminalsException& e)
        {
            if (unimplementedSymbolsWarning) {
               auto unimplemented_symbols = e.getSymbols();
                std::cout << "The Lexer will not scan the following token(s):" << std::endl;
                for (int i = 0; i < unimplemented_symbols.size(); i++)
                {
                    auto id = unimplemented_symbols.at(i);
                    std::wcout <<L"    " << JavaParsersym::orderedTerminalSymbols[id] << std::endl;               
                }
               std::cout <<std::endl;  
            }
        }
        catch (UndefinedEofSymbolException& e)
        {
            std::stringex str= "The Lexer does not implement the Eof symbol ";
            str += IcuUtil::ws2s(JavaParsersym::orderedTerminalSymbols[prsTable->getEoftSymbol()]);
            throw  UndefinedEofSymbolException(str);
        } 
    }
    
     JavaParser(ILexStream* lexStream = nullptr)
    {
        try
        {
            btParser = new BacktrackingParser(prsStream, prsTable,  this);
        }
        catch (NotBacktrackParseTableException& e)
        {
            throw ( NotBacktrackParseTableException
                                ("Regenerate JavaParserprs.java with -BACKTRACK option"));
        }
        catch (BadParseSymFileException& e)
        {
            throw ( BadParseSymFileException("Bad Parser Symbol File -- JavaParsersym::java"));
        }

        if(lexStream)
        {
            reset(lexStream);
        }
    }
    

    
     int numTokenKinds() { return JavaParsersym::numTokenKinds; }
     std::vector<std::wstring> orderedTerminalSymbols() { 
         return JavaParsersym::orderedTerminalSymbols; 
    }
     std::wstring getTokenKindName(int kind) { return JavaParsersym::orderedTerminalSymbols[kind]; }
     int getEOFTokenKind() { return prsTable->getEoftSymbol(); }
     IPrsStream* getIPrsStream() { return prsStream; }

    /**
     * @deprecated replaced by {@link #getIPrsStream()}
     *
     */
     PrsStream* getPrsStream() { return prsStream; }

    /**
     * @deprecated replaced by {@link #getIPrsStream()}
     *
     */
     PrsStream* getParseStream() { return prsStream; }

    ExprParser_top_level_ast::Ast* parser()
    {
        return parser(nullptr, 0);
    }

    ExprParser_top_level_ast::Ast* parser(Monitor* monitor)
    {
        return parser(monitor, 0);
    }
    
    ExprParser_top_level_ast::Ast * parser(int error_repair_count)
    {
        return parser(nullptr, error_repair_count);
    }

    ExprParser_top_level_ast::Ast * parser(Monitor* monitor, int error_repair_count)
    {
        btParser->setMonitor(monitor);
        
        try
        {
            return (ExprParser_top_level_ast::Ast *) btParser->fuzzyParse(error_repair_count);
        }
        catch (BadParseException& e)
        {
            prsStream->reset(e.error_token); // point to error token

            std::shared_ptr< DiagnoseParser> diagnoseParser = std::make_shared<DiagnoseParser>(prsStream, prsTable);
            diagnoseParser->diagnose(e.error_token);
        }

        return nullptr;
    }

    //
    // Additional entry points, if any
    //
    
    ExprParser_top_level_ast::Ast * parseClassBodyDeclarationsopt()
    {
        return parseClassBodyDeclarationsopt(nullptr, 0);
    }
        
    ExprParser_top_level_ast::Ast * parseClassBodyDeclarationsopt(Monitor* monitor)
    {
        return parseClassBodyDeclarationsopt(monitor, 0);
    }
        
    ExprParser_top_level_ast::Ast * parseClassBodyDeclarationsopt(int error_repair_count)
    {
        return parseClassBodyDeclarationsopt(nullptr, error_repair_count);
    }
        
    ExprParser_top_level_ast::Ast * parseClassBodyDeclarationsopt(Monitor *monitor, int error_repair_count)
    {
        btParser->setMonitor(monitor);
        
        try
        {
            return (ExprParser_top_level_ast::Ast *) btParser->fuzzyParseEntry(JavaParsersym::TK_ClassBodyDeclarationsoptMarker, error_repair_count);
        }
        catch (BadParseException& e)
        {
            prsStream->reset(e.error_token); // point to error token

             std::shared_ptr< DiagnoseParser> diagnoseParser = std::make_shared<DiagnoseParser>(prsStream, prsTable);
            diagnoseParser->diagnoseEntry(JavaParsersym::TK_ClassBodyDeclarationsoptMarker, e.error_token);
        }

        return nullptr;
    }

    ExprParser_top_level_ast::Ast * parseLPGUserAction()
    {
        return parseLPGUserAction(nullptr, 0);
    }
        
    ExprParser_top_level_ast::Ast * parseLPGUserAction(Monitor* monitor)
    {
        return parseLPGUserAction(monitor, 0);
    }
        
    ExprParser_top_level_ast::Ast * parseLPGUserAction(int error_repair_count)
    {
        return parseLPGUserAction(nullptr, error_repair_count);
    }
        
    ExprParser_top_level_ast::Ast * parseLPGUserAction(Monitor *monitor, int error_repair_count)
    {
        btParser->setMonitor(monitor);
        
        try
        {
            return (ExprParser_top_level_ast::Ast *) btParser->fuzzyParseEntry(JavaParsersym::TK_LPGUserActionMarker, error_repair_count);
        }
        catch (BadParseException& e)
        {
            prsStream->reset(e.error_token); // point to error token

             std::shared_ptr< DiagnoseParser> diagnoseParser = std::make_shared<DiagnoseParser>(prsStream, prsTable);
            diagnoseParser->diagnoseEntry(JavaParsersym::TK_LPGUserActionMarker, e.error_token);
        }

        return nullptr;
    }

	
   pool_holder ast_pool;
   void ruleAction(int ruleNumber);
    //#line 357 "btParserTemplateF.gi

 };
