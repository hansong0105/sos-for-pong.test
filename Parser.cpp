#include <cctype>
#include "./Parser.h"
#include "./C_Commands.h"

using namespace parth::nand2tetris::myassembler;
using namespace std;

Parser::~Parser() {
    _in.close();
}

// advance _pos till a non-ws char
void Parser::skipWhitespace() {
    while(isspace(_currLine[_pos]))
        _pos++;
}

/*
	A user-defined symbol can be any sequence
	of letters, digits, underscore ( _ ),
	dot ( . ), dollar sign ( $ ), and colon ( : )
	that does not begin with a digit.
	Side effect:
 at the end, leaves _pos to point the character
 just after the symbol (not at the last
 character of the symbol)
 */
string Parser::extractSymbol() {
    string sym;
    char c;
    
    while(_pos < _currLine.size()) {
        c = _currLine[_pos];
        if(!isalnum(c) && c != '_' && c != '.'
           && c != '$' && c != ':')
            break;
        sym += c;
        _pos++;
    }
    return sym;
}

// in handleComment, do NOT assume _pos == 0
// even at the start of the call. It could be
// that the comment could start after an A_COMMAND
// or so (in the same line) - and _pos != 0
// but it is SAFE to assume that _currLine[_pos] == '/'
// because the function will be called only in that case
void Parser::handleComment() {
    // whitespace can't be skipped here
    // already read one '/'
    _pos++;
    if(_currLine[_pos] == '/')
    {
        // the line must be marked as "read"
        _pos = _currLine.size();
        return;
    }
    string err_message = "Unexpected '";
    err_message += _currLine[_pos];
    err_message += "'";
    
    throw SyntaxErrorException(_lineNum, _pos,
                               _currLine, err_message);
}
void Parser::handleACommand() {
    _pos++;			// '@' has been read
    skipWhitespace();
    string sym;
    if(!isdigit(_currLine[_pos]))
        sym = extractSymbol();
    else {
        // it's a number.
        while(isdigit(_currLine[_pos]))
            sym += _currLine[_pos++];
    }
    
    // only whitespace allowed after this point.
    skipWhitespace();
    
    string err_message = "Unexpected '";
    err_message += _currLine[_pos];
    err_message += "'";
    
    if(_pos != _currLine.size())
        throw SyntaxErrorException(_lineNum, _pos,
                                   _currLine, err_message);
    
    _pos = _currLine.size();
    _symbol = sym;
    _cmdType = A_COMMAND;
}
// [dest=]comp[;jump]
void Parser::handleCCommand() {
    /*
     proceed till the end. If we encounter a
     '=' sign, we have a dest. Set beg = pos('=')+1
     If we encounter a ';' or '\n' we found the comp
     After the ';', get the jump
     */
    /*
     states:
     0 : no clue
     If we find a '=', we found a dest
     If we find a ';', we found a comp, dest = null
     If we reach end, we found a comp, dest = null
     If we find a '/', we found a comp, dest = null
     1 : found dest, looking for comp
     If we find a ';', we found a comp
     If we reach end, we found a comp
     If we find a '/', we found a comp
     2 : found dest and comp, looking for jump
     If we reach end & no chars found, jump = null
     If we read '/' & no chars found, jump = null
     If we reach end, & found chars, we found jump
     If we reach '/' & found chars, we found jump
     then check dest, comp, jump. If all are valid,
     aal ijh well. Else, error.
     */
    skipWhitespace();
    int state = 0;
    char c;
    string s;
    
    int dest_pos = _pos, comp_pos = _pos, jump_pos = _pos;
    
    // state is guaranteed to be 3 by the time we're done
    while(state < 3) {
        if(_pos >= _currLine.size())
            c = 0;		// end - denoted by c=0
        else
            c = _currLine[_pos];
        
        if(isspace(c)) {
            _pos++;
            continue;
        }
        
        if(state == 0) {
            switch(c) {
                case '=':
                    _dest = s;
                    s = "";
                    comp_pos = jump_pos = _pos;
                    state = 1;
                    _pos++;
                    continue;
                case ';':
                case '/':
                case 0:		// end
                    _dest = "null";
                    _comp = s;
                    s = "";
                    jump_pos = _pos;
                    state = 2;
                    _pos++;
                    continue;
            }
        }
        else if(state == 1) {
            switch(c) {
                case ';':
                case '/':
                case 0:		// end
                    _comp = s;
                    s = "";
                    jump_pos = _pos;
                    state = 2;
                    _pos++;
                    continue;
            }
        }
        else if(state == 2) {
            switch(c) {
                case '/':
                case 0:		// end
                    if(s == "")
                        _jump = "null";
                    else
                        _jump = s;
                    state = 3;
            }
        }
        
        s += c;
        _pos++;
    }
    
    /*
     cout << "CurrLine: '" << _currLine << "' : "
     << "dest: " << _dest << "; comp: " << _comp
     << "; jump: " << _jump << "\n";
     */
    if( !dest_mnemonics.isMnemonicValid(_dest) )
        throw SyntaxErrorException(_lineNum, dest_pos,
                                   _currLine, string("Bad destination. '")
                                   + _dest + "' given.");
    
    if( !comp_mnemonics.isMnemonicValid(_comp) )
        throw SyntaxErrorException(_lineNum, comp_pos,
                                   _currLine, string("Bad comp. '")
                                   + _comp + "' given.");
    
    if( !jump_mnemonics.isMnemonicValid(_jump) )
        throw SyntaxErrorException(_lineNum, jump_pos,
                                   _currLine, string("Bad jump. '")
                                   + _jump + "' given.");
    
    _cmdType = C_COMMAND;
    _pos = _currLine.size();
}
void Parser::handleLCommand() {
    _pos++;			// '(' has been read
    skipWhitespace();
    
    string label = extractSymbol();
    // _pos is after the end of the label
    if(_currLine[_pos] != ')')
        throw SyntaxErrorException(_lineNum, _pos, _currLine,
                                   string("')' Expected"));
    
    _pos++;
    skipWhitespace();
    
    string err_message = "Unexpected '";
    err_message += _currLine[_pos];
    err_message += "'\n";
    
    if(_pos != _currLine.size())
        throw SyntaxErrorException(_lineNum, _pos,
                                   _currLine, err_message);
    
    _symbol = label;
    _cmdType = L_COMMAND;
    _pos = _currLine.size();
}


bool Parser::hasMoreCommands() {
    if(!_in) return false;
    
    skipWhitespace();
    if(_pos ==_currLine.size())
        getline(_in, _currLine);
    else {
        return true;
    }
    
    _pos = 0;
    
    skipWhitespace();
    // check for empty lines
    if(_pos == _currLine.size())
        return hasMoreCommands();
    
    if(_currLine[_pos] == '/')
    {
        handleComment();
        return hasMoreCommands();
    }
    
    return true;
}

void Parser::advance() {
    if(!hasMoreCommands())
        throw IllegalCallException("Illegal Call");
    
    // don't advance the postion.
    // Let the 'handle' methods check
    switch(_currLine[_pos]) {
            // comments handled by hasMoreCommands()
        case '(':		// L_COMMAND
            handleLCommand();
            break;
        case '@':		// A_COMMAND
            handleACommand();
            break;
            // no _instructionNum++ here.
        default:		// *could* be a C_COMMAND.
            // There COULD be a syntax error
            handleCCommand();
            break;
    }
    _lineNum++;
}

CommandType Parser::commandType() {	return _cmdType; }
string Parser::symbol() { return _symbol; }
string Parser::dest() { return _dest; }
string Parser::comp() { return _comp; }
string Parser::jump() { return _jump; }