/*
 * File: statement.cpp
 * -------------------
 * This file implements the constructor and destructor for
 * the Statement class itself.  Your implementation must do
 * the same for the subclasses you define for each of the
 * BASIC statements.
 */

#include <string>
#include <iostream>
#include <memory>
#include "parser.h"
#include "statement.h"
#include "../StanfordCPPLib/tokenscanner.h"
#include "../StanfordCPPLib/error.h"
using namespace std;

/* Implementation of the Statement class */

Statement::Statement() {
   /* Empty */
}

Statement::~Statement() {
   /* Empty */
}

void Statement::executeGo(EvalState & state, map<int, shared_ptr<Statement>> & statements, map<int, shared_ptr<Statement>>::iterator & itr){

}

/* Implementation of the RemStmt class*/
RemStmt::RemStmt(string mes) {
    message = mes;
}

RemStmt::~RemStmt() {

}

void RemStmt::execute(EvalState & state) {

}

/* Implementation of the LetStmt class*/
LetStmt::LetStmt(string mes, string variable, string con) {
    var = variable;
    content = con;
    message = mes;
}

LetStmt::~LetStmt() {

}

void LetStmt::execute(EvalState & state) {
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(content);   
    Expression *exp = parseExp(scanner);
    int value = exp->eval(state);
    state.setValue(var,value);
    delete exp;
}   

/* Implementation of the PrintStmt class*/
PrintStmt::PrintStmt(string mes, string cont) {
    content = cont;
    message = mes;
}

PrintStmt::~PrintStmt() {

}

void PrintStmt::execute(EvalState & state) {
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(content);   
    Expression *exp = parseExp(scanner);
    int value = exp->eval(state);
    cout << value << endl;
    delete exp;
}

/* Implementation of the InputStmt class*/
InputStmt::InputStmt(string mes, string v) {
    var = v;
    message = mes;
}

InputStmt::~InputStmt() {

}

void InputStmt::execute(EvalState & state) {
    cout << " ? ";
    int value;
    cin >> value;
    string str;
    if (!cin.fail()) {
        getline(cin, str);
    }
    while (cin.fail() || !(str.length() == 0)) {
        if (cin.fail()) {
            cin.clear();
            getline(cin, str);            
        }
        cout <<"INVALID NUMBER" << endl;
        cout << " ? ";
        cin >> value;
        if (!cin.fail())
            getline(cin, str);
    } 
    state.setValue(var, value);
}

/* Implementation of the GotoStmt class*/
GotoStmt::GotoStmt(string mes, int number) {
    lineNumber = number;
//    cout << "stored number: " << lineNumber << endl;
    message = mes;
}

GotoStmt::~GotoStmt() {

}

void GotoStmt::execute(EvalState & state) {

}

void GotoStmt::executeGo(EvalState & state, map<int, shared_ptr<Statement>> & statements, map<int, shared_ptr<Statement>>::iterator & itr) {
    map<int, shared_ptr<Statement>>::iterator itr2;
    for (itr2 = statements.begin(); itr2!=statements.end(); itr2++) {
//        cout << lineNumber << endl;
//        cout << itr2->first << endl;
        if (itr2->first == lineNumber) {
            itr = itr2;
//            cout << "successfully" << endl;
            return;
        }
    }  
    error("LINE NUMBER ERROR");  
}



/* Implementation of the IfStmt class*/
IfStmt::IfStmt(string mes, string le, string re, string o, int number) {
    message = mes;
    lExpression = le;
    rExpression = re;
    op = o;
    lineNumber = number;
}

IfStmt::~IfStmt() {

}

void IfStmt::execute(EvalState & state) {
    

}

void IfStmt::executeGo(EvalState & state, map<int, shared_ptr<Statement>> & statements, map<int, shared_ptr<Statement>>::iterator & itr) {
   
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(lExpression);   
    Expression *exp = parseExp(scanner);
    int lValue = exp->eval(state);
    scanner.setInput(rExpression);
    exp = parseExp(scanner);
    int rValue = exp->eval(state);
    delete exp;

    bool flag;
    if (op == ">") {
        if (lValue > rValue) 
            flag = true;
        else flag = false;
    }
    else if (op == "<") {
        if (lValue < rValue) flag = true;
        else flag = false;
    }
    else if (op == "=") {
        if (lValue == rValue) flag = true;
        else flag = false;
    }  
    
    if (flag) {
        map<int, shared_ptr<Statement>>::iterator itr2;
        for (itr2 = statements.begin(); itr2!=statements.end(); itr2++) {
//        cout << lineNumber << endl;
//        cout << itr2->first << endl;
            if (itr2->first == lineNumber) {
                itr = itr2;
//            cout << "successfully" << endl;
                return;
            }
        } 
    }
    else itr++;   
}

/* Implementation of the EndStmt class*/
EndStmt::EndStmt(string mes) {
    message = mes;
}

EndStmt::~EndStmt() {

}

void EndStmt::execute(EvalState & state) {

}
