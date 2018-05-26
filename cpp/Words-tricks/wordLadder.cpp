#include <iostream>
#include <queue>
#include <stack>
#include <set>
#include <fstream>
#include <cctype>
#include <string>
using namespace std;

string promptUserForFile(ifstream & infile) {
    while (true) {
        cout << "Dictionary file name? ";
        string filename;
        getline(cin, filename);
        infile.open(filename.c_str());
        if (!infile.fail())
            return filename;
        infile.clear();
        cout << "Unable to open the file. Try again." << endl;
    }
}

void getDictionary(ifstream & infile, set<string>& dict) {
    string str;
    while(getline(infile,str)) {
        dict.insert(str);
    }
}

string toLowerCase(string str) {
    string result = "";
    for (int i = 0; i < str.length(); i++) {
        if (!isalpha(str[i]))
            return "";
        result += tolower(str[i]);
    }
    return result;
}

int getWords(string &word1, string &word2, set<string>& dict) {
    cout << "Word #1 (or Enter to quit): ";
    getline(cin, word1);
    if (word1 == "") {
        cout << "Have a nice day." << endl;
        return -1;
    }
    cout << "Word #2 (or Enter to quit): ";
    getline(cin, word2);
    if (word2 == "") {
        cout << "Have a nice day." << endl;
        return -1;
    }
    word1 = toLowerCase(word1);
    word2 = toLowerCase(word2);
    if (word1 != "" && word2 != "") {
        if (word1 != word2) {
                return 1;
            }
        else {
            cout << "The two words must be different." << endl;
            return 0;
        }
    }
    else {
        cout << "Invalid input." << endl;
        return 0;
    }   
}

void wordLadder(set<string> &dict, string word1, string word2) {
    queue<stack<string>> ladders;
    stack<string> firstLadder;
    firstLadder.push(word1);
    ladders.push(firstLadder);
    stack<string> currentLadder;
    set<string> wordUsed;
    wordUsed.insert(word1);
    bool breakFlag = 0;
    while(true) {
		int count = 0;
        int number = ladders.size();
        for (int i = 0; i < number; i++) {
            currentLadder = ladders.front();
            ladders.pop();
            string word = currentLadder.top();

            for (int j = 0; j <= word.length(); j++){
                for (int n = 0; n < 26; n++) {
                    string wordAdded = word;
                    string str = "";
                    wordAdded.insert(j, str + (char)('a' + n));
                    if (dict.count(wordAdded) && !(wordUsed.count(wordAdded))) {
					    count++;
                        wordUsed.insert(wordAdded);
                        stack<string> newLadder = currentLadder;
                        newLadder.push(wordAdded);
                        ladders.push(newLadder);
                        if (wordAdded == word2) {
                            breakFlag = 1;
                            currentLadder = newLadder;
                            break;
                        }
                    }
                }
                if (breakFlag)
                break;
            }
            if (breakFlag)
                break;

            for (int j = 0; j < word.length(); j++) {
                if (word.length() > 1) {
                    string wordRemoved = word;
                    wordRemoved.erase(j, 1);
                    if (dict.count(wordRemoved) && !(wordUsed.count(wordRemoved))){
                        count++;
                        wordUsed.insert(wordRemoved);
                        stack<string> newLadder = currentLadder;
                        newLadder.push(wordRemoved);
                        ladders.push(newLadder);
                        if (wordRemoved == word2) {
                            breakFlag = 1;
                            currentLadder = newLadder;
                            break;
                        }
                    }
                }          
                for (int n = 0; n < 26; n++) {
                    string currentWord = word;
                    currentWord[j] = (char)('a'+n);
                    if (dict.count(currentWord) && !(wordUsed.count(currentWord))) {
						count++;
                        wordUsed.insert(currentWord);
                        stack<string> newLadder = currentLadder;
                        newLadder.push(currentWord);
                        ladders.push(newLadder);
                        if (currentWord == word2) {
                            breakFlag = 1;
                            currentLadder = newLadder;
                            break;
                        }
                    }
                }
                if (breakFlag)
                    break;
            }
            if (breakFlag)
                break;
        }
        if (breakFlag)
            break;
		if (count == 0) {
			cout << "There is no ladder from " << word2 << " back to " << word1 << "." << endl;
			return;
		}
    }
    cout << "A laddaer from " << word2 << " back to " << word1 << ":" << endl;
    string str = currentLadder.top();
    while(true) {
        currentLadder.pop();
        if (currentLadder.empty()) {
            cout << str << endl;
            return;
        }
        cout << str << " -> ";
        str = currentLadder.top();
    }
}

int main() {
    set<string> dict;
    ifstream infile;

    string filename =  promptUserForFile(infile);
    getDictionary(infile, dict);

    string word1;
    string word2;
    while(true) {
        cout << endl;
        int sign = getWords(word1, word2, dict);
        if(sign == -1)
            return 0;
        else if (sign == 1)
            wordLadder(dict, word1, word2);
    }


    infile.close();

    system("pause");
    return 0;
}