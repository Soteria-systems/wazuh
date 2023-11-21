#ifndef _BK_MOCK_CONTROLLER_HPP
#define _BK_MOCK_CONTROLLER_HPP

#include <gmock/gmock.h>

#include <bk/icontroller.hpp>
#include <bk/icontroller.hpp>

namespace bk::mocks
{
class MockController : public IController
{
public:
    MOCK_METHOD(void, build, (base::Expression, std::unordered_set<std::string>, std::function<void()>), (override));
    MOCK_METHOD(void, build, (base::Expression, std::unordered_set<std::string>), (override));
    MOCK_METHOD(void, ingest, (base::Event&&), (override));
    MOCK_METHOD(base::Event, ingestGet, (base::Event&&), (override));
    MOCK_METHOD(bool, isAviable, (), (const, override));
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(std::string, printGraph, (), (const, override));
    MOCK_METHOD(const std::unordered_set<std::string>&, getTraceables, (), (const, override));
    MOCK_METHOD(base::RespOrError<Subscription>, subscribe, (const std::string&, const Subscriber&), (override));
    MOCK_METHOD(void, unsubscribe, (const std::string&, Subscription), (override));
};

class MockMakerController : public IControllerMaker
{
public:
    MOCK_METHOD(std::shared_ptr<IController>, create, (), (override));
};
} // namespace bk::mocks

#endif // _BK_MOCK_CONTROLLER_HPP