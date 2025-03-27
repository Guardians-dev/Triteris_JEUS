#include "SimpleMessagePack.hpp"
#include <sstream>
#include <iomanip>

std::string MessageData::serialize() const {
    std::string result;
    
    // 타입 정보 추가 (1바이트)
    result.push_back(static_cast<char>(type));
    
    switch (type) {
        case Null:
            // 널은 추가 데이터 없음
            break;
            
        case Boolean:
            result.push_back(boolValue ? 1 : 0);
            break;
            
        case Integer: {
            // 8바이트 정수
            for (int i = 0; i < 8; i++) {
                result.push_back((intValue >> (i * 8)) & 0xFF);
            }
            break;
        }
            
        case Float: {
            // 8바이트 부동소수점
            uint64_t bits;
            memcpy(&bits, &floatValue, sizeof(floatValue));
            for (int i = 0; i < 8; i++) {
                result.push_back((bits >> (i * 8)) & 0xFF);
            }
            break;
        }
            
        case String: {
            // 문자열 길이 (4바이트) + 문자열 데이터
            uint32_t size = stringValue.size();
            for (int i = 0; i < 4; i++) {
                result.push_back((size >> (i * 8)) & 0xFF);
            }
            result.append(stringValue);
            break;
        }
            
        case Array: {
            // 배열 크기 (4바이트) + 각 요소 직렬화
            uint32_t size = arrayValue.size();
            for (int i = 0; i < 4; i++) {
                result.push_back((size >> (i * 8)) & 0xFF);
            }
            for (const auto& item : arrayValue) {
                std::string itemData = item.serialize();
                result.append(itemData);
            }
            break;
        }
            
        case Object: {
            // 객체 크기 (4바이트) + 각 키-값 쌍 직렬화
            uint32_t size = objectValue.size();
            for (int i = 0; i < 4; i++) {
                result.push_back((size >> (i * 8)) & 0xFF);
            }
            for (const auto& [key, value] : objectValue) {
                // 키 길이 (4바이트) + 키 문자열 + 값 직렬화
                uint32_t keySize = key.size();
                for (int i = 0; i < 4; i++) {
                    result.push_back((keySize >> (i * 8)) & 0xFF);
                }
                result.append(key);
                
                std::string valueData = value.serialize();
                result.append(valueData);
            }
            break;
        }
    }
    
    return result;
}

MessageData MessageData::deserialize(const std::string& data) {
    if (data.empty()) {
        return MessageData();
    }
    
    size_t pos = 0;
    return deserializeValue(data, pos);
}

MessageData MessageData::deserializeValue(const std::string& data, size_t& pos) {
    if (pos >= data.size()) {
        return MessageData();
    }
    
    Type type = static_cast<Type>(data[pos++]);
    MessageData result;
    result.type = type;
    
    switch (type) {
        case Null:
            // 널은 추가 데이터 없음
            break;
            
        case Boolean:
            result.boolValue = (data[pos++] != 0);
            break;
            
        case Integer: {
            // 8바이트 정수
            int64_t value = 0;
            for (int i = 0; i < 8; i++) {
                value |= static_cast<int64_t>(static_cast<uint8_t>(data[pos++])) << (i * 8);
            }
            result.intValue = value;
            break;
        }
            
        case Float: {
            // 8바이트 부동소수점
            uint64_t bits = 0;
            for (int i = 0; i < 8; i++) {
                bits |= static_cast<uint64_t>(static_cast<uint8_t>(data[pos++])) << (i * 8);
            }
            memcpy(&result.floatValue, &bits, sizeof(bits));
            break;
        }
            
        case String: {
            // 문자열 길이 (4바이트) + 문자열 데이터
            uint32_t size = 0;
            for (int i = 0; i < 4; i++) {
                size |= static_cast<uint32_t>(static_cast<uint8_t>(data[pos++])) << (i * 8);
            }
            result.stringValue = data.substr(pos, size);
            pos += size;
            break;
        }
            
        case Array: {
            // 배열 크기 (4바이트) + 각 요소 역직렬화
            uint32_t size = 0;
            for (int i = 0; i < 4; i++) {
                size |= static_cast<uint32_t>(static_cast<uint8_t>(data[pos++])) << (i * 8);
            }
            for (uint32_t i = 0; i < size; i++) {
                result.arrayValue.push_back(deserializeValue(data, pos));
            }
            break;
        }
            
        case Object: {
            // 객체 크기 (4바이트) + 각 키-값 쌍 역직렬화
            uint32_t size = 0;
            for (int i = 0; i < 4; i++) {
                size |= static_cast<uint32_t>(static_cast<uint8_t>(data[pos++])) << (i * 8);
            }
            for (uint32_t i = 0; i < size; i++) {
                // 키 길이 (4바이트) + 키 문자열
                uint32_t keySize = 0;
                for (int j = 0; j < 4; j++) {
                    keySize |= static_cast<uint32_t>(static_cast<uint8_t>(data[pos++])) << (j * 8);
                }
                std::string key = data.substr(pos, keySize);
                pos += keySize;
                
                // 값 역직렬화
                result.objectValue[key] = deserializeValue(data, pos);
            }
            break;
        }
    }
    
    return result;
} 