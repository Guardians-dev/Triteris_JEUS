#pragma once
#include <string>
#include "SimpleMessagePack.hpp"
#include <functional>
#include <map>
#include <vector>
#include <memory>

// Event 클래스 정의
struct Event {
    std::string name;
    MessageData data;
    
    Event(const std::string& eventName, const MessageData& eventData = MessageData())
        : name(eventName), data(eventData) {}
};

// EventBus 클래스 정의
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
        if (handlers.find(event.name) != handlers.end()) {
            for (const auto& handler : handlers[event.name]) {
                handler(event);
            }
        }
    }
    
    // 편의를 위한 오버로드
    void publish(const std::string& eventType, const MessageData& data = MessageData()) {
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