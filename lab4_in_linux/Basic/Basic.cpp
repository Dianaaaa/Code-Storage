/*
 * File: Basic.cpp
 * ---------------
 * Name: [TODO: enter name here]
 * Section: [TODO: enter section leader here]
 * This file is the starter project for the BASIC interpreter from
 * Assignment #6.
 * [TODO: extend and correct the documentation]
 */

#include <cctype>
#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include "exp.h"
#include "parser.h"
#include "program.h"
#include "statement.h"
#include "../StanfordCPPLib/error.h"
#include "../StanfordCPPLib/tokenscanner.h"

#include "../StanfordCPPLib/simpio.h"
#include "../StanfordCPPLib/strlib.h"
using namespace std;

/* Function prototypes */
void processLine(string line, Program & program, EvalState & state);

/* Main program */

int main() {
   EvalState state;
   Program program;
//   cout << "Stub implementation of BASIC" << endl;
   while (true) {
      try {
          string line;
          getline(cin, line);
          if (line.length() > 0) {
            processLine(line, program, state);
          }
      } catch (ErrorException & ex) {
         cout << ex.getMessage() << endl;
      }
   }
   return 0;
}

/*
 * Function: processLine
 * Usage: processLine(line, program, state);
 * -----------------------------------------
 * Processes a single line entered by the user.  In this version,
 * the implementation does exactly what the interpreter program
 * does in Chapter 19: read a line, parse it as an expression,
 * and then print the result.  In your implementation, you will
 * need to replace this method with one that can respond correctly
 * when the user enters a program line (which begins with a number)
 * or one of the BASIC commands, such as LIST or RUN.
 */

void processLine(string line, Program & program, EvalState & state) {
/*   TokenScanner scanner;
   scanner.ignoreWhitespace();
   scanner.scanNumbers();
   scanner.setInput(line);   
   Expression *exp = parseExp(scanner);
   int value = exp->eval(state);
   cout << value << endl;
   delete exp;*/

    stringstream ss(line);
    int number;
    ss >> number;
    
    if (ss.fail()) {
        ss.clear();
        stringstream sst(line);
        string str;
        sst >> str;
        if (str == "LET") {
            string var;
            sst >> var;
            if (!program.checkIdentifier(var)) error("SYNTAX ERROR");

            string str;
            sst >> str;
            if (str != "=") error("SYNTAX ERROR");

            string content;
            getline(sst, content);
            LetStmt let(line, var, content);
            let.execute(state);
        }
        else if (str == "PRINT") {
            string var;
            getline(sst, var);
            PrintStmt print(line, var);
            print.execute(state);
        }
        else if (str == "INPUT") {
            string var;
            sst >> var;
            InputStmt input(line, var);
            input.execute(state);
        }
        else if (str == "QUIT") {
            exit(0);
        }
        else if (str == "LIST") {
            program.listAll();
        }
        else if (str == "CLEAR") {
            program.clear();
            state.clear();
        }
        else if (str == "RUN") {
            program.run(state);
        }
        else if (str == "HELP") {
            program.help();
        }
        else error("SYNTAX ERROR");
    }
    else {
        string str;
        getline(ss, str);
        program.addSourceLine(number, str, state);
    }
}
