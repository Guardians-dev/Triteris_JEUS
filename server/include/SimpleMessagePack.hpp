#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <map>

// JSON 대신 간단한 데이터 구조 정의
class MessageData {
public:
    enum Type { Null, Boolean, Integer, Float, String, Array, Object };
    
    Type type;
    bool boolValue;
    int64_t intValue;
    double floatValue;
    std::string stringValue;
    std::vector<MessageData> arrayValue;
    std::map<std::string, MessageData> objectValue;
    
    MessageData() : type(Null) {}
    
    // 생성자들
    MessageData(bool value) : type(Boolean), boolValue(value) {}
    MessageData(int value) : type(Integer), intValue(value) {}
    MessageData(int64_t value) : type(Integer), intValue(value) {}
    MessageData(double value) : type(Float), floatValue(value) {}
    MessageData(const std::string& value) : type(String), stringValue(value) {}
    
    // 맵 생성자
    MessageData(const std::map<std::string, MessageData>& value) : type(Object), objectValue(value) {}
    
    // 배열 생성자
    MessageData(const std::vector<MessageData>& value) : type(Array), arrayValue(value) {}
    
    // 벡터 변환 생성자
    MessageData(const std::vector<std::vector<int>>& board) : type(Array) {
        for (const auto& row : board) {
            MessageData rowData;
            for (int cell : row) {
                rowData.push_back(cell);
            }
            arrayValue.push_back(rowData);
        }
    }
    
    // 인덱스 접근자
    MessageData& operator[](const std::string& key) {
        type = Object;
        return objectValue[key];
    }
    
    // 값 접근
    const MessageData& operator[](const std::string& key) const {
        return objectValue.at(key);
    }
    
    // 값 존재 확인
    bool contains(const std::string& key) const {
        return type == Object && objectValue.find(key) != objectValue.end();
    }
    
    // 직렬화/역직렬화 메서드
    std::string serialize() const;
    static MessageData deserialize(const std::string& data);

    // MessageData 클래스에 추가
    MessageData& operator=(std::initializer_list<std::pair<const std::string, MessageData>> init) {
        type = Object;
        objectValue.clear();
        for (const auto& pair : init) {
            objectValue[pair.first] = pair.second;
        }
        return *this;
    }

    // MessageData 클래스에 추가
    operator std::string() const {
        if (type == String) {
            return stringValue;
        }
        return ""; // 기본값
    }

    // MessageData 클래스에 추가
    std::string dump() const {
        std::string result;
        
        switch (type) {
            case Null:
                result = "null";
                break;
            case Boolean:
                result = boolValue ? "true" : "false";
                break;
            case Integer:
                result = std::to_string(intValue);
                break;
            case Float:
                result = std::to_string(floatValue);
                break;
            case String:
                result = "\"" + stringValue + "\"";
                break;
            case Array:
                result = "[";
                for (size_t i = 0; i < arrayValue.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += arrayValue[i].dump();
                }
                result += "]";
                break;
            case Object:
                result = "{";
                size_t i = 0;
                for (const auto& [key, value] : objectValue) {
                    if (i++ > 0) result += ", ";
                    result += "\"" + key + "\": " + value.dump();
                }
                result += "}";
                break;
        }
        
        return result;
    }

    // MessageData 클래스에 추가
    static MessageData array() {
        MessageData data;
        data.type = Array;
        return data;
    }

    void push_back(const MessageData& value) {
        if (type != Array) {
            type = Array;
            arrayValue.clear();
        }
        arrayValue.push_back(value);
    }

    // 대입 연산자 추가
    MessageData& operator=(const std::vector<std::vector<int>>& board) {
        type = Array;
        arrayValue.clear();
        for (const auto& row : board) {
            MessageData rowData;
            for (int cell : row) {
                rowData.push_back(cell);
            }
            arrayValue.push_back(rowData);
        }
        return *this;
    }

    // 배열 인덱스 접근자 추가
    MessageData& operator[](size_t index) {
        if (type != Array) {
            type = Array;
            arrayValue.resize(index + 1);
        } else if (index >= arrayValue.size()) {
            arrayValue.resize(index + 1);
        }
        return arrayValue[index];
    }

    const MessageData& operator[](size_t index) const {
        return arrayValue.at(index);
    }

    // size() 메서드 추가
    size_t size() const {
        if (type == Array) return arrayValue.size();
        if (type == Object) return objectValue.size();
        if (type == String) return stringValue.size();
        return 0;
    }

    // 초기화 리스트 생성자 추가
    MessageData(std::initializer_list<std::pair<const std::string, MessageData>> init) : type(Object) {
        for (const auto& pair : init) {
            objectValue[pair.first] = pair.second;
        }
    }

private:
    // deserializeValue 함수를 MessageData 클래스의 private 멤버로 선언
    static MessageData deserializeValue(const std::string& data, size_t& pos);
};

class SimpleMessagePack {
public:
    // MessageData를 바이너리 형식으로 변환
    static std::string pack(const MessageData& data) {
        std::string serialized = data.serialize();
        uint32_t size = serialized.size();
        
        // 메시지 길이 정보 추가 (4바이트)
        std::string result(4 + size, 0);
        result[0] = (size >> 24) & 0xFF;
        result[1] = (size >> 16) & 0xFF;
        result[2] = (size >> 8) & 0xFF;
        result[3] = size & 0xFF;
        
        // 직렬화된 데이터 복사
        std::copy(serialized.begin(), serialized.end(), result.begin() + 4);
        
        return result;
    }
    
    // 바이너리 형식을 MessageData로 변환
    static MessageData unpack(const std::string& data) {
        return MessageData::deserialize(data);
    }
}; 