/*
	Class: SymbolTable
	Namespace: parth::nand2tetris::myassembler
	@author Han Song
 */

#ifndef SYMBOLTABLE
#define SYMBOLTABLE 1

#include <map>
#include <string>

using namespace std;

namespace parth {
    namespace nand2tetris {
        namespace myassembler {
            
            class SymbolTable {
            private:
                map<string, int> _tab;
            public:
                SymbolTable() {}
                void addEntry(string s, int addr);
                bool contains(string s);
                string getAddress(string s);
                string formatAsBin(int addr);
            };
            
        }
    }
}

#endif