#pragma once
#include "var.h"

class VarU32 : public Var
{
public:
    VarU32(const char* name, uint32_t addr, uint minVal, uint maxVal, double convertVal, int machineOffset, const char* unitLabel)
        : Var(name, VarType::UINT32, 1, addr, (long)minVal, (long)maxVal, convertVal, machineOffset, unitLabel){}

    // Getters
    const char* getUnitLabel() { return Var::getUnitLabel(); }
    uint getMinVal() { return Var::getMinVal(); }
    uint getMaxVal() { return Var::getMaxVal(); }
    uint getVal() {
        getArray();
        if(this->_array != nullptr)
            return this->_array[0];
        return 0;
    }
    uint* getArray() {

        // Get the inherited value array, return if null
        long* value = Var::getValue();
        if(value == nullptr)
            return nullptr;
        
        // Map the values of the value array to a new int array
        delete _array;
        _array = new uint[Var::getArrayLen()];
        for(int i = 0; i < Var::getArrayLen(); i++)
            _array[i] = (uint)value[i];
        return _array;
    }

    // Setters
    void setMinVal(uint val) { Var::setMinVal(val); }
    void setMaxVal(uint val) { Var::setMaxVal(val); }
    void setVal(uint val) {
        if(_array != nullptr)
            delete _array;
        _array = new uint[1];
        _array[0] = val;
    }
    void setArray(uint* array, uint length) {
        long* newArray = new long[length];
        for(int i = 0; i < length; i++)
            newArray[i] = (long)array[i];
        Var::setValue(newArray, length);
    }

    // Functions
    double convertToReal() { return Var::convertToReal((long)getVal()); }
    double convertToReal(uint value) { return Var::convertToReal((long)value); }
    uint convertToMachine(double value) { return (uint)Var::convertToMachine(value); }

private:
    uint* _array;
};