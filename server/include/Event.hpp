#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <functional>
#include <map>
#include <vector>
#include <memory>

using json = nlohmann::json;

// 이벤트 기본 클래스
class Event {
public:
    std::string type;
    json data;
    
    Event(const std::string& eventType, const json& eventData = {})
        : type(eventType), data(eventData) {}
    virtual ~Event() = default;
};

// 이벤트 버스 - 이벤트 발행/구독 시스템
class EventBus {
private:
    // 이벤트 타입별 핸들러 맵
    std::map<std::string, std::vector<std::function<void(const Event&)>>> handlers;
    
public:
    // 이벤트 핸들러 등록
    void subscribe(const std::string& eventType, std::function<void(const Event&)> handler) {
        handlers[eventType].push_back(handler);
    }
    
    // 이벤트 발행
    void publish(const Event& event) {
        if (handlers.find(event.type) != handlers.end()) {
            for (const auto& handler : handlers[event.type]) {
                handler(event);
            }
        }
    }
    
    // 편의를 위한 오버로드
    void publish(const std::string& eventType, const json& data = {}) {
        publish(Event(eventType, data));
    }
};

// 전역 이벤트 버스 (싱글톤) - 메이어스 싱글톤 패턴 적용
class GlobalEventBus {
private:
    // 생성자와 복사 생성자를 private으로 설정하여 외부에서 인스턴스 생성 방지
    GlobalEventBus() = default;
    GlobalEventBus(const GlobalEventBus&) = delete;
    GlobalEventBus& operator=(const GlobalEventBus&) = delete;
    
public:
    // 메이어스 싱글톤 패턴: 정적 지역 변수를 사용하여 스레드 안전하게 인스턴스 생성
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }
}; 