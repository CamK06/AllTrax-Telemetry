#pragma once
#include "var.h"

class VarString : public Var
{
public:
    VarString(const char* name, int length, uint32_t addr)
        : Var(name, VarType::STRING, length, addr, 0, 255, 0.0, 0, nullptr){}

    // Getters
    const char* getValue() {
        long* value = Var::getValue();
        if(value == nullptr)
            return nullptr;
        
        // Map the values in value to characters
        char* array = new char[Var::getArrayLen()];
        for(int i = 0; i < Var::getArrayLen(); i++) {
            array[i] = (char)value[i];
            // If there's no character, put a space
            if(array[i] == 0)
                array[i] = 32;
        }

        return (const char*)array;
    }

    // Setters
    void setValue(const char* string, int length) {
        long* array = new long[Var::getArrayLen()];
        for(int i = 0; i < Var::getArrayLen(); i++) {
            if(i < length)
                array[i] = (long)string[i];
            else
                array[i] = 32;
        }
        Var::setValue(array, length);
    }

private:
};