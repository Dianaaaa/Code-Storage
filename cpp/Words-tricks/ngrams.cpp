#include <iostream>
#include <random>
#include <map>
#include <vector>
#include <fstream>
#include <ctime>
#include <list>
#include <cmath>
using namespace std;

int TEXT_WIDTH = 10;
int C_TEXT_WIDTH = 30;
int SEED = int(time(NULL));

string promptUserForFile(ifstream & infile) {
    while (true) {
        cout << "Input file? ";
        string filename;
        getline(cin, filename);
        infile.open(filename.c_str());
        if (!infile.fail())
            return filename;
        infile.clear();
        cout << "Unable to open the file. Try again." << endl;
    }
}

int getN(int threshold, string prompt) {
    int n;
    while(true) {
        cout << prompt;
        cin >> n;
        if (cin.fail()) {
            cin.clear();
            string str;
            getline(cin, str);
            cout << "Invalid input. Try again." << endl;
        }
        else if (n >= threshold || n == 0)
            return n;
        else {
            cout << "N should be bigger than" << threshold <<". Try again." << endl;
        }         
    }
}

int randomInteger(int low, int high) {
    srand(SEED);
    double d = rand() / (double(RAND_MAX) + 1);
    double s = d * (double(high) - low);
    SEED = (SEED + 1) % 1008611;
    return int(floor(low + s));
}

void getCharacter(ifstream& infile, string& word) {
    word = "";
    char ch;
    infile >> ch;
    word += ch;
    if (isprint(ch))
        return;
    infile >> ch;
    word += ch;
}

void getTrainedSet(ifstream& infile, map<vector<string>, vector<string>>& trainedSet, vector<vector<string>>& forBeginning, int n, bool ChineseFlag) {
    list<string> buffer;
    list<string>::iterator itr;
    string word;
    for (int i = 0; i < n; i++) {
        if (ChineseFlag == 0)
            infile >> word;
        else {
            getCharacter(infile, word);
        }
        buffer.push_back(word);
    }
    while (!infile.eof()) {
        vector<string> key;
        for (itr = buffer.begin(); itr != buffer.end(); itr++) {
            key.push_back(*itr);
        }

        if (('A' < key[0][0]) && (key[0][0] < 'Z'))
            forBeginning.push_back(key);

        if (ChineseFlag == 0)
            infile >> word;
        else
            getCharacter(infile, word);

        if (trainedSet.count(key)) {
            trainedSet[key].push_back(word);
        }
        else {
            vector<string> value;
            value.push_back(word);
            trainedSet.insert(pair<vector<string>, vector<string>>(key, value));
        }
        buffer.pop_front();
        buffer.push_back(word);
    }
}

void chooseBeginning(vector<vector<string>>& forBeginning, vector<string>& currentKey, int& count) {
    int randomChoice = randomInteger(0, forBeginning.size());
    currentKey = forBeginning[randomChoice];
    for (int i = 0; i < currentKey.size(); i++) {
        if (count % TEXT_WIDTH == 0)
            cout << endl;
        cout << currentKey[i] << " ";
        count++;
    }
}

void getSingleWord(map<vector<string>, vector<string>>& trainedSet, vector<string>& currentKey, string& currentWord, int& count, bool ChineseFlag) {
    int width;
    if (ChineseFlag == 0)
        width = TEXT_WIDTH;
    else
        width = C_TEXT_WIDTH;
    int randomChoice = randomInteger(0, trainedSet[currentKey].size());
    currentWord = trainedSet[currentKey][randomChoice];
    if (count % width == 0)
        cout << endl;
    if (ChineseFlag == 0)
        cout << currentWord << " ";
    else
        cout << currentWord;
    count++;
    vector<string> temp;
    for (int i = 1; i < currentKey.size(); i++) {
        temp.push_back(currentKey[i]);
    }
    temp.push_back(currentWord);
    currentKey = temp;
}

void chooseEndding(map<vector<string>, vector<string>>& trainedSet, vector<string>& currentKey, string& currentWord, int& count) {
    char mark = currentWord[currentWord.length() - 1];
    while (true) {
        vector<string> currentValue = trainedSet[currentKey];
        for (int i = 0; i < currentValue.size(); i++) {
            currentWord = currentValue[i];
            mark = currentWord[currentWord.length() - 1]; 
            if (mark == '.' || mark == ';' || mark == '?' || mark == '!') {
                if (count % TEXT_WIDTH == 0)
                    cout << endl;
                cout << currentWord << " ";
                return;
            }           
        }
        getSingleWord(trainedSet, currentKey, currentWord, count, 0);
    }
}

void randomWrite(map<vector<string>, vector<string>>& trainedSet, vector<vector<string>>& forBeginning, int n, bool ChineseFlag) {
    int textLength = getN(n, "# of random words to generate (0 to quit)? ");
    if (textLength == 0)
        return;

    int count = 0;
    string currentWord;
    vector<string> currentKey;
    //Choose the beginning of text randomly.
    if (ChineseFlag == 0) {
        chooseBeginning(forBeginning, currentKey, count);
        while (count < textLength) {
            getSingleWord(trainedSet, currentKey, currentWord, count, 0);
        }
        chooseEndding(trainedSet, currentKey, currentWord, count);
    }
    else {
        int randomChoice = randomInteger(0, trainedSet.size());
        map<vector<string>, vector<string>>::iterator itr;
        itr = trainedSet.begin();
        for (int i = 0; i < randomChoice; i++)
            itr++;
        currentKey = itr->first;
        for (int i = 0; i < currentKey.size(); i++) {
            if (count % C_TEXT_WIDTH == 0)
                cout << endl;
            cout << currentKey[i];
            count++;
        }
        while (count < textLength) {
            getSingleWord(trainedSet, currentKey, currentWord, count, 1);
        }
        while(true) {
            vector<string> currentValue = trainedSet[currentKey];
            for (int i = 0; i < currentValue.size(); i++) {
                currentWord = currentValue[i];
                if(currentWord == "¡£" || currentWord == "£¡" || currentWord == "£¿" || currentWord == "£»") {
                    if (count % C_TEXT_WIDTH == 0)
                        cout << endl;
                    cout << currentWord;
                    return;
                }           
            }
            getSingleWord(trainedSet, currentKey, currentWord, count, bool(1));
        }
    }

    cout << endl;
}
void nGrams(ifstream& infile, int n, bool ChineseFlag) {
    map<vector<string>, vector<string>> trainedSet;
    vector<vector<string>> forBeginning;
    getTrainedSet(infile, trainedSet, forBeginning, n, ChineseFlag);
    randomWrite(trainedSet, forBeginning, n, ChineseFlag);
}

int main() {
    ifstream infile;
    promptUserForFile(infile);

    bool ChineseFlag = 0;
    char ch;
    ch = infile.peek();
    if (int(ch) < 0)
        ChineseFlag = 1;
    

    int n = getN(2, "Value of N (0 to quit)? ");
    if (n == 0)
        return 0;


    nGrams(infile, n, ChineseFlag);

    infile.close();
    return 0;
}