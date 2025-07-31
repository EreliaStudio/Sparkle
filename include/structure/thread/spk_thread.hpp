#pragma once

#include "structure/spk_iostream.hpp"

#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

namespace spk
{
class Thread
{
private:
std::wstring _name;
std::function<void()> _innerCallback;
std::unique_ptr<std::thread> _handle = nullptr;

public:
Thread(const std::wstring &p_name, const std::function<void()> &p_callback);
virtual ~Thread();

virtual void start();
virtual void join();

bool isJoinable() const;
};
}