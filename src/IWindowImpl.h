#pragma once

#include <AEvent>
#include <memory>
#include <string>
#include <vector>

class IWindowImpl {
public:
    virtual ~IWindowImpl() = default;

    virtual bool create(const std::string& title, int width, int height) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    virtual std::vector<std::shared_ptr<AEvent>> pollEvents() = 0;

    virtual void* getNativeHandle() const = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual void setTitle(const std::string& title) = 0;
};
