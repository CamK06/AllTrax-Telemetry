#pragma once
#include "var.h"

class VarBool : public Var
{
public:
    VarBool(const char* name, uint32_t addr)
        : Var(name, VarType::BOOL, 1, addr, 0, 255, 0.0, 0, nullptr){}

    // Getters
    bool getValue() { return _array != nullptr && _array[0]; }
    bool* getArray() { 

        // Get the inherited value array, return if null
        long* value = Var::getValue();
        if(value == nullptr)
            return nullptr;
        
        // Map the values of the value array to a new bool array
        bool* array = new bool[Var::getArrayLen()];
        for(int i = 0; i < Var::getArrayLen(); i++) {
            if(value[i] == 0)
                array[i] = false;
            else
                array[i] = true;
        }

        // Return the new bool array
        return array;
    }

    // Setters
    void setValue(bool val) {

        if(_array != nullptr)
            delete _array;
        _array = new bool[1];
        _array[0] = val;
    }
    void setArray(bool* array, int length) {

        // Map the values of the bool array to a new long array
        long* newArray = new long[length];
        for(int i = 0; i < length; i++) {
            if(!array[i])
                newArray[i] = 0;
            else
                newArray[i] = 1;
        }
        Var::setValue(newArray, length);
    }

private:
    bool* _array;
};