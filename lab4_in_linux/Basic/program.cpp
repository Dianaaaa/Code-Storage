/*
 * File: program.cpp
 * -----------------
 * This file is a stub implementation of the program.h interface
 * in which none of the methods do anything beyond returning a
 * value of the correct type.  Your job is to fill in the bodies
 * of each of these methods with an implementation that satisfies
 * the performance guarantees specified in the assignment.
 */

#include <string>
#include <map>
#include <sstream>
#include <memory>
#include "program.h"
#include "statement.h"
#include "../StanfordCPPLib/error.h"
using namespace std;

Program::Program() {
   // Replace this stub with your own code
}

Program::~Program() {
   // Replace this stub with your own code
}

void Program::clear() {
   // Replace this stub with your own code
   statements.clear();
}

bool Program::checkIdentifier(string str) {
    if ((str=="REM") || (str=="LET") || (str=="PRINT") || (str=="INPUT") || (str=="END") ||(str=="GOTO")
    || (str=="IF") || (str=="THEN") || (str == "RUN") || (str=="LIST") || (str=="CLEAR") || (str=="QUIT")
    || (str=="HELP")) return false;
    else return true;
}

void Program::addSourceLine(int lineNumber, string line, EvalState & state) {
   // Replace this stub with your own code
   stringstream ss(line);
   string word;
   ss >> word;
   if (ss.fail()) {
       map<int, shared_ptr<Statement>>::iterator itre;
       for (itre = statements.begin(); itre != statements.end(); itre++) {
           if (itre->first == lineNumber) statements.erase(itre);
       }
   }
   if (word == "REM") {
       shared_ptr<RemStmt> rem(new RemStmt(line));
       if (statements.count(lineNumber)) {
           statements[lineNumber] = rem;
       }
       statements.insert(pair<int, shared_ptr<Statement>>(lineNumber, rem));      
   }
   else if (word == "LET") {
       string var;
       ss >> var;
       if (!checkIdentifier(var)) error("SYNTAX ERROR");

       string str;
       ss >> str;
       if (str != "=") error("SYNTAX ERROR");

       string content;
       getline(ss, content);
       
       
       shared_ptr<LetStmt> let(new LetStmt(line, var, content));
       if (statements.count(lineNumber)) {
           statements[lineNumber] = let;
       }
       statements.insert(pair<int, shared_ptr<Statement>>(lineNumber, let));
   }
   else if (word == "PRINT") {
       string content;
       getline(ss, content);
       shared_ptr<PrintStmt> print(new PrintStmt(line, content));
       if (statements.count(lineNumber)) {
           statements[lineNumber] = print;
       }
       statements.insert(pair<int, shared_ptr<Statement>>(lineNumber, print));
   }
   else if (word == "INPUT") {
       string var;
       ss >> var;
       shared_ptr<InputStmt> input(new InputStmt(line, var));
       if (statements.count(lineNumber)) {
           statements[lineNumber] = input;
       }
       statements.insert(pair<int, shared_ptr<Statement>>(lineNumber, input));
   }
   else if (word == "GOTO") {
       int number;
       ss >> number;
       if (cin.fail()) {
           error("LINE NUMBER ERROR");
       }
//       cout << "input goto number: " << number << endl;
       shared_ptr<GotoStmt> got(new GotoStmt(line, number));
       if (statements.count(lineNumber)) {
           statements[lineNumber] = got;
       }
       statements.insert(pair<int, shared_ptr<Statement>>(lineNumber, got));
   }
   else if (word == "IF") {
       string str;
       ss >> str;
       string le;
       while(str != ">" && str != "<" && str != "=") {           
           le += str;  
           ss >> str;
           if (ss.fail()) {
               error("SYNTAX ERROR");
           }         
       }
       string o = str;
       ss >> str;
       string re;
       while (str != "THEN") {
           re += str;
           ss >> str;
           if (ss.fail()){
               error("SYNTAX ERROR");
           }
       }
       int number;
       ss >> number;
       shared_ptr<IfStmt> ifs(new IfStmt(line, le, re, o, number));
       if (statements.count(lineNumber)) {
           statements[lineNumber] = ifs;
       }
       statements.insert(pair<int, shared_ptr<Statement>>(lineNumber, ifs));

   }
   else if (word == "END") {
       shared_ptr<EndStmt> end(new EndStmt(line));
       if (statements.count(lineNumber)) {
           statements[lineNumber] = end;
       }
       statements.insert(pair<int, shared_ptr<Statement>>(lineNumber, end));
   }
}

void Program::removeSourceLine(int lineNumber) {
   // Replace this stub with your own code
   map<int, shared_ptr<Statement>>::iterator itr;
   for(itr = statements.begin(); itr != statements.end(); itr++) {
       if (itr->first == lineNumber) {
        statements.erase(itr);
        break;
       }
   }
}

string Program::getSourceLine(int lineNumber) {
       // Replace this stub with your own code
    map<int, shared_ptr<Statement>>::iterator itr;
   for(itr = statements.begin(); itr != statements.end(); itr++) {
       if (itr->first == lineNumber) {
        return itr->second->message;
       }
   }
}

void Program::setParsedStatement(int lineNumber, Statement *stmt) {
   // Replace this stub with your own code
}

Statement *Program::getParsedStatement(int lineNumber) {
   return NULL;  // Replace this stub with your own code
}

int Program::getFirstLineNumber() {
   return 0;     // Replace this stub with your own code
}

int Program::getNextLineNumber(int lineNumber) {
   return 0;     // Replace this stub with your own code
}



void Program::listAll() {
    map<int, shared_ptr<Statement>>::iterator itr;
    for (itr = statements.begin(); itr != statements.end(); itr++) {
        cout << itr->first << itr->second->message << endl;
    }
}

void Program::run(EvalState & state) {
    map<int, shared_ptr<Statement>>::iterator itr;
    itr = statements.begin();
    while ( itr != statements.end()) {
        stringstream ss(itr->second->message);
        string str;
        ss >> str;
        if (str == "GOTO") {
            
            itr->second->executeGo(state, statements, itr);
                
        }
        else if (str == "IF") {
            itr->second->executeGo(state, statements, itr);
        }
        else if (str == "END") {
            return;
        }
        else { 
            itr->second->execute(state);
            itr ++;
        }
    }
}

void Program::help() {
    cout << "Directly-executing: " << endl;
    cout << "   LET identifier = exp, assign the value of exp to the identifier." << endl;
    cout << "   PRINT exp, print the value of exp." << endl;
    cout << "   INPUT identifier, input the value of the identifier." << endl;
    cout << "Basic Program:" << endl;
    cout << "   You should add a number before each line to identify the executing order of \n this line" << endl;
    cout << "   If the number already exist, it will be rewrite." << endl;
    cout << "   n REM comments: used for comments." << endl;
    cout << "   n LET identifier = exp, assign the value of exp to the identifier." << endl;
    cout << "   n PRINT exp, print the value of exp." << endl;
    cout << "   n INPUT identifier, input the value of the identifier." << endl;
    cout << "   n END: mark the end of the program." << endl;
    cout << "   n GOTO n, go to the line n." << endl;
    cout << "   n IF exp cmp exp THEN n, if (exp cmp exp) is true then go to the line n." << endl;
    cout << "   RUN: run this program."  << endl;
    cout << "   LIST: lists the steps in the program in numerical sequence. " << endl;
    cout << "   CLEAR: deletes all program and variables." << endl;
    cout << "   QUIT: exits from the BASIC interpreter by calling exit(0). " << endl;
    cout << "   HELP: provides a simple help message describing your interpreter. " << endl;
}