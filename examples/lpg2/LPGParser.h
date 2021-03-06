
    //#line 163 "btParserTemplateF.gi

#pragma once

#include <iostream>
#include "lpg2/AstPoolHolder.h"
#include "lpg2/BacktrackingParser.h"
#include "lpg2/DeterministicParser.h"
#include "lpg2/diagnose.h"
#include "lpg2/ErrorToken.h"
#include "lpg2/Exception.h"
#include "lpg2/IAbstractArrayList.h"
#include "lpg2/IAst.h"
#include "lpg2/IAstVisitor.h"
#include "lpg2/ILexStream.h"
#include "LPGParsersym.h"
#include "LPGParserprs.h"
#include "lpg2/Object.h"
#include "lpg2/ParseTable.h"
#include "lpg2/PrsStream.h"
#include "lpg2/RuleAction.h"
#include "lpg2/IcuUtil.h"
#include "lpg2/stringex.h"
#include "lpg2/Any.h"

    //#line 7 "LPGParser.g

    #include <unordered_map>
namespace LPGParser_top_level_ast {
    struct ASTNodeToken;
    struct defineSpec;
    struct terminal_symbol0;
    struct recover_symbol;
    struct terminal;
    struct nonTerm;
}

 
    //#line 190 "btParserTemplateF.gi

 struct LPGParser :public Object ,public RuleAction
{
   
    PrsStream* prsStream = nullptr;
    ~LPGParser (){
        delete prsStream;
        delete btParser;
    }
     bool unimplementedSymbolsWarning = false;

     inline static ParseTable* prsTable = new LPGParserprs();
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
                    std::wcout <<L"    " << LPGParsersym::orderedTerminalSymbols[id] << std::endl;               
                }
               std::cout <<std::endl;  
            }
        }
        catch (UndefinedEofSymbolException& e)
        {
            std::stringex str= "The Lexer does not implement the Eof symbol ";
            str += IcuUtil::ws2s(LPGParsersym::orderedTerminalSymbols[prsTable->getEoftSymbol()]);
            throw  UndefinedEofSymbolException(str);
        } 
    }
    
     LPGParser(ILexStream* lexStream = nullptr)
    {
        try
        {
            btParser = new BacktrackingParser(prsStream, prsTable,  this);
        }
        catch (NotBacktrackParseTableException& e)
        {
            throw ( NotBacktrackParseTableException
                                ("Regenerate LPGParserprs.java with -BACKTRACK option"));
        }
        catch (BadParseSymFileException& e)
        {
            throw ( BadParseSymFileException("Bad Parser Symbol File -- LPGParsersym::java"));
        }

        if(lexStream)
        {
            reset(lexStream);
        }
    }
    

    
     int numTokenKinds() { return LPGParsersym::numTokenKinds; }
     std::vector<std::wstring> orderedTerminalSymbols() { 
         return LPGParsersym::orderedTerminalSymbols; 
    }
     std::wstring getTokenKindName(int kind) { return LPGParsersym::orderedTerminalSymbols[kind]; }
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

     Object* parser()
    {
        return parser(nullptr, 0);
    }
    
     Object* parser(Monitor* monitor)
    {
        return parser(monitor, 0);
    }
    
     Object * parser(int error_repair_count)
    {
        return parser(nullptr, error_repair_count);
    }

     Object * parser(Monitor* monitor, int error_repair_count)
    {
        btParser->setMonitor(monitor);
        
        try
        {
            return (Object *) btParser->fuzzyParse(error_repair_count);
        }
        catch (BadParseException& e)
        {
            prsStream->reset(e.error_token); // point to error token

            std::shared_ptr< DiagnoseParser> diagnoseParser = std::make_shared<DiagnoseParser>(prsStream, prsTable);
            diagnoseParser->diagnose(e.error_token);
        }

        return nullptr;
    }
     void ruleAction(int ruleNumber);
    //
    // Additional entry points, if any
    //
    

    //#line 38 "LPGParser.g


std::unordered_multimap<std::wstring, LPGParser_top_level_ast::terminal_symbol0*>  terminal_symbol_produce_SYMBOL;
std::unordered_multimap<std::wstring, LPGParser_top_level_ast::recover_symbol*>  _recover_symbols;
std::unordered_multimap<std::wstring, LPGParser_top_level_ast::defineSpec*>  _define_specs;

std::unordered_multimap<std::wstring, LPGParser_top_level_ast::nonTerm*>  _non_terms;
std::unordered_multimap<std::wstring, LPGParser_top_level_ast::terminal*>  _terms;
std::vector<LPGParser_top_level_ast::ASTNodeToken*>  _macro_name_symbo;
 
    //#line 364 "btParserTemplateF.gi

};

