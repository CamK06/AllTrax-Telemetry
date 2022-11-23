#pragma once

#include <string>
#include <cmath>

enum VarType
{
    BOOL, BYTE, SBYTE, UINT16, INT16, UINT32, INT32, STRING
};

class Var
{
protected:
    Var(const char* name, VarType type, int arrayLen, uint32_t addr, long minVal, long maxVal, double convertVal, int machineOffset, const char* unitLabel) {

        // Set basic info
        this->_name = (char*)name;
        this->_type = type;
        this->_arrayLen = arrayLen;
        this->_addr = addr;
        this->_minVal = minVal;
        this->_maxVal = maxVal;
        this->_machineOffset = machineOffset;
        this->_convertVal = convertVal;
        this->_unitLabel = (char*)unitLabel;

        // Set the numBytes based on the var type
        switch(type)
        {
        case VarType::BOOL:
        case VarType::BYTE:
        case VarType::SBYTE:
            this->_numBytes = 1;
            return;
        case VarType::INT16:
        case VarType::UINT16:
            this->_numBytes = 2;
            return;
        case VarType::INT32:
        case VarType::UINT32:
            this->_numBytes = 4;
            return;
        case VarType::STRING:
            this->_numBytes = 1;
            return;
        default:
            return;
        }
    }

    // Setters
    void setMinVal(long minVal) { this->_minVal = minVal; }
    void setMaxVal(long maxVal) { this->_maxVal = maxVal; }
    void setAddress(uint32_t addr) { this->_addr = addr; }

    // Functions
    double convertToReal(long value) {
        return (double)(value - (long)this->_machineOffset) * this->_convertVal;
    }
    long convertToMachine(double value) {
        if(this->_convertVal == 0.0)
            return 0;

        long num = std::round(value / this->_convertVal + this->_machineOffset);
        if(num > this->_maxVal)
            num = this->_maxVal;
        if(num < this->_minVal)
            num = this->_minVal;
        return num;
    }

public:
    // Getters
    const char* getName() { return (const char*)_name; }
    const char* getUnitLabel() { return (const char*)_unitLabel; }
    VarType getType() { return this->_type; }
    long getMinVal() { return this->_minVal; }
    long getMaxVal() { return this->_maxVal; }
    int getMachineOffset() { return this->_machineOffset; }
    double getConvertVal() { return this->_convertVal; }
    long* getValue() { return this->_val; }
    int getNumBytes() { return this->_numBytes; }
    int getArrayLen() { return this->_arrayLen; }
    uint32_t getAddr() { return this->_addr; }

    // Setters
    void setValue(long* val, int length) { 

        // If there's no valid data to copy, set this->_val to null then return
        if(val == nullptr || length > this->_arrayLen) {
            this->_val = nullptr;
            return;
        }

        // Replace this->_val with the contents of val
        if(this->_val != nullptr)
            delete this->_val;
        this->_val = new long[length];
        for(int i = 0; i < length; i++)
            this->_val[i] = val[i];
    }

private:
    char* _name;
    char* _unitLabel;
    VarType _type;
    int _numBytes;
    int _arrayLen;
    int _machineOffset;
    long _minVal;
    long _maxVal;
    double _convertVal;
    uint32_t _addr;
    long* _val;
};