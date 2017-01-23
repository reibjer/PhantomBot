/**
    jsonLexer.cpp
    PhantomBot Project
    By: Robert F. (Phantom139)
**/

#include "jsonLexer.h"

namespace JSON {

	/**
		StringStream Class
	**/

	char StringStream::peek() {
		char retChar = '\0';
		if(strCPos < strEnd) {
			retChar = *strCPos;
		}
		return retChar;
	}
	
	char StringStream::get() {
		char retChar = '\0'
		if(strCPos < strEnd) {
			retChar = *strCPos;
			updatePosition(retChar);
		}
		return retChar;
	}
	
	void StringStream::moveBack() {
		if(strCPos > strStart) {
			strCPos--;
		}
	}
	
	bool StringStream::nextCharsMatch(const char *test) {
		bool result = true;
		const char *tmp = strCPos;
		while(result == true && (tmp < strEnd && test && *test)) {
			if(tolower(*test) != tolower(*tmp)) {
				result = false;
				break;
			}
			tmp++;
			test++;
		}
		//Push back if we matched
		if(result == true) {
			strCPos = tmp;
		}
		return result;
	}
	
	int StringStream::lNum() const {
		return cLine;
	}
	
	int StringStream::curPos() const {
		return cPos;
	}
	
	void StringStream::updatePosition(char tChar) {
		if(tChar == '\n') {
			cLine++;
			cPos = 1;
		}
	}
	
	/**
		Lexer Class
	**/	
	Token Lexer::readNext() {
		Token t;
		char c = '\0';
		while(t.getType() == UNKNOWN) {
			c = srcStream->get();
			//Skip spaces, end of lines, etc
			if(c == ' ' || c == '\t' || c == '\n' || c == '\r') {
				continue;
			}
			//convert to lowercase
			c = static_cast<char>(tolower(c));
			//Scan different charachters, set token
			if(c == '\0') {
				t.set(EOF);
				t.set("EOF");
			} 
			else if(c == '{') {
				t.set(OBJECT_BEGIN);
				t.set("{");
			}
			else if(c == '}') {
				t.set(OBJECT_END);
				t.set("}");
			}
			else if(c == '[') {
				t.set(ARRAY_BEGIN);
				t.set("[");
			}
			else if(c == ']') {
				t.set(ARRAY_END);
				t.set("]");
			}
			else if(c == ",") {
				t.set(COMMA);
				t.set(",");
			}
			else if(c == ':') {
				t.set(COLON);
				t.set(":");
			}
			else if(c == '"') {
				t = readFromString(c);
			}
			//Booleans & NULL require scanning additional characters, we use our nextCharsMatch method
			// to scan the following to see if we have any matches here.
			else if(c == 't' && srcStream->nextCharsMatch("rue")) {
				t.set(BOOLEAN);
				t.set("true");
			}
			else if(c == 'f' && srcStream->nextCharsMatch("alse")) {
				t.set(BOOLEAN);
				t.set("false");
			}
			else if(c == 'n' && srcStream->nextCharsMatch("ull")) {
				t.set(TNULL);
				t.set("null");
			}
			else if(isdigit(c)) {
				t = readFromNumber(c);
			}
			else if(charIsSign(c)) {
				//Accoring to the JSON documentation, signs (+ & -) are treated as numerics
				// But for the time being, we'll keep this in a separate conditional in the event
				// things change.
				t = readFromNumber(c);
			}
			else {
				conditionalException(srcStream, true, "Invalid Charachter: '" + c + "'.");
			}
		}
		return t;
	}
	
	int Lexer::lNum() const {
		return srcStream->lNum();
	}
	
	int Lexer::curPos() const {
		return srcStream->curPos();
	}
	
	uint8_t Lexer::readFromHex(char c) {
		if(c >= '0' && c <= '9') {
			return c - '0';
		}
		else if(c >= 'a' && c <= 'f') {
			return c - 'a' + 10;
		}
		else if(c >= 'A' && c <= 'F') {
			return c - 'A' + 10;
		}
	}
	
	Token Lexer::readFromString(char c) {
		Token t;
		while(t.getType() == UNKNOWN) {
			c = srcStream->get();
			if(c == '"') {
				t.set(STRING);
				t.set("\"");
			}
			else if(c == '\\') {
				//Escape Sequences
				c = srcStream->get();
				if(c == '"' || c == '\\' || c == '/') {
					t += c;
				}
				else if(c == 'b') {
					t += '\b';
				}
				else if(c == 'f') {
					t += '\f';
				}
				else if(c == 'n') {
					t += '\n';
				}
				else if(c == 't') {
					t += '\t';
				}
				else if(c == 'r') {
					t += '\r';
				}
				else if(c == 'u') {
					unsigned long pt = 0;
					for(size_t i = 0; i < 4; i++) {
						c = srcStream->get();
						uint8_t digit = 0;
						conditionalException(srcStream, c == '\0', "Unterminated string instance");
						conditionalException(srcStream, c == '"', "Invalid Unicode escape sequence");
						conditionalException(srcStream, !isxdigit(c), "Invalid unicode escape sequuence formatting");
						digit = readFromHex(c);
						pt *= 16;
						pt += digit;
					}
					//Format codepoint accordingly
					if(pt < 0x80) {
						t += (char)pt;
					}
					else if(pt < 0x800) {
						t += (char)(0xC0 | (pt >> 6));
						t += (0x80 | (pt & 0x3F));
					}
					else {
						t += (char)(0xE0 | (pt >> 12));
						t += (0x80 | ((pt >> 6) & 0x3F));
						t += (0x80 | (pt & 0x3F));
					}
				}
			}
			else if(c == '\0') {
				conditionalException(srcStream, true, "Unterminated string instance");
			}
			else {
				t += c;
			}
		}
		return t;
	}
	
	Token Lexer::readFromNumber(char c) {
		Token t;
		t.set(NUMERIC);
		t += c;
		c = srcStream->get();
		//Scan to ensure text remains numerical
		while(isdigit(c)) {
			t += c;
			c = srcStream->get();
		}
		//Decimal?
		if(c == '.') {
			t += c;
			if(isdigit(srcStream->peek())) {
				c = srcStream->get();
				while(isdigit(c)) {
					t += c;
					c = srcStream->get();
				}
				//Once we reach a non-numeric, move back one so the lexer can process this character
				srcStream->moveBack();
			}
			else {
				conditionalException(srcStream, true, "Invalid numeric");
			}
		}
		else {
			//Not a decimal, move back one character and let the lexer continue
			srcStream->moveBack();
		}
		return t;
	}
	
	bool Lexer::charIsSign(char c) {
		return (c == '+' || c == '-');
	}
	
	/*
	Stream Utility Functions
	*/
	inline void conditionalException(Stream *src, bool throwCondition, string msg) {
		if(throwCondition) {
			string err = "Error: ";
			err += " L: " + (src ? src->lNum() : 0);
			err += " P: " + (src ? src->curPos() : 0);
			err += " Alert: " + msg;
			throw Exception(err.c_str());
		}
	}
	
	inline void conditionalException(bool throwCondition, string msg) {
		if(throwCondition) {
			throw Exception(msg.c_str());
		}
	}

};