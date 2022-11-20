#pragma once
#include "var.h"

class VarInt32 : protected Var
{
public:
    VarInt32(const char* name, uint32_t addr, int minVal, int maxVal, double convertVal, int machineOffset, const char* unitLabel)
        : Var(name, VarType::INT32, 1, addr, (long)minVal, (long)maxVal, convertVal, machineOffset, unitLabel){}

    // Getters
    const char* getUnitLabel() { return Var::getUnitLabel(); }
    int getMinVal() { return Var::getMinVal(); }
    int getMaxVal() { return Var::getMaxVal(); }
    int getVal() {
        if(this->_array != nullptr)
            return this->_array[0];
        return 0;
    }
    int* getArray() {

        // Get the inherited value array, return if null
        long* value = Var::getValue();
        if(value == nullptr)
            return nullptr;
        
        // Map the values of the value array to a new int array
        int* array = new int[Var::getArrayLen()];
        for(int i = 0; i < Var::getArrayLen(); i++)
            array[i] = (int)value[i];
        return array;
    }

    // Setters
    void setMinVal(int val) { Var::setMinVal(val); }
    void setMaxVal(int val) { Var::setMaxVal(val); }
    void setVal(int val) {
        if(_array != nullptr)
            delete _array;
        _array = new int[1];
        _array[0] = val;
    }
    void setArray(int* array, int length) {
        long* newArray = new long[length];
        for(int i = 0; i < length; i++)
            newArray[i] = (long)array[i];
        Var::setValue(newArray, length);
    }

    // Functions
    double convertToReal() { return Var::convertToReal((long)getVal()); }
    double convertToReal(int value) { return Var::convertToReal((long)value); }
    int convertToMachine(double value) { return (int)Var::convertToMachine(value); }

private:
    int* _array;
};