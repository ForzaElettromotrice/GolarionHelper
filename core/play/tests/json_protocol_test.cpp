#include "protocol.hpp"

#include <array>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    using namespace golarion::play::detail;
    using Bytes = std::vector<std::uint8_t>;
    using Json = nlohmann::json;

    template<typename Function>
    bool rejects(Function function)
    {
        try { function(); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }

    Bytes rawFrame(std::string_view text)
    {
        const auto size = static_cast<std::uint32_t>(text.size());
        Bytes bytes;
        for (const int shift : {24, 16, 8, 0}) bytes.push_back(static_cast<std::uint8_t>(size >> shift));
        bytes.insert(bytes.end(), text.begin(), text.end());
        return bytes;
    }

    void expectRejected(Bytes bytes)
    {
        const Bytes original = bytes;
        assert(rejects([&] { decodeMessage(bytes); }));
        assert(bytes == original);
    }

    void expectMessage(const ProtocolMessage &actual, const ProtocolMessage &expected)
    {
        assert(actual.type == expected.type);
        assert(actual.requestId == expected.requestId);
        assert(actual.payload == expected.payload);
    }

    Json envelope()
    {
        return Json{{"version", 2}, {"type", "custom.command"}, {"requestId", "request-42"}, {"payload", Json::object()}};
    }

    void roundTripAndUtf8()
    {
        const std::string unicode = "caf\xc3\xa8 \xf0\x9f\x8e\xb2";
        const ProtocolMessage request{"custom.command", "request-42", Json{{"name", unicode}, {"nested", {{"values", Json::array({nullptr, true, -3, 4.5})}}}}};
        auto bytes = encodeMessage(request);
        const std::uint32_t size = (std::uint32_t(bytes[0]) << 24) | (std::uint32_t(bytes[1]) << 16) | (std::uint32_t(bytes[2]) << 8) | bytes[3];
        assert(size == bytes.size() - 4);
        const std::string text(bytes.begin() + 4, bytes.end());
        assert(text.find(unicode) != std::string::npos);
        const Json wire = Json::parse(text);
        assert(wire.at("version") == 2);
        assert(wire.at("type") == request.type);
        assert(wire.at("requestId") == *request.requestId);
        assert(wire.at("payload") == request.payload);
        const auto restored = decodeMessage(bytes);
        assert(restored);
        expectMessage(*restored, request);
        assert(bytes.empty());

        const ProtocolMessage event{"users.snapshot", std::nullopt, Json{{"users", Json::array()}}};
        bytes = encodeMessage(event);
        assert(!Json::parse(bytes.begin() + 4, bytes.end()).contains("requestId"));
        expectMessage(*decodeMessage(bytes), event);
        assert(bytes.empty());

        bytes = rawFrame(R"({"payload":{"targetUserId":"user-7","role":"spectator"},"requestId":"r1","type":"users.setRole","version":2})");
        const auto independent = decodeMessage(bytes);
        assert(independent && independent->type == "users.setRole");
        assert(independent->payload.at("targetUserId") == "user-7");
        assert(bytes.empty());
    }

    void fragmentedAndCoalescedFrames()
    {
        const ProtocolMessage first{"first", "1", Json{{"value", 1}}};
        const ProtocolMessage second{"second", std::nullopt, Json::object()};
        const Bytes encoded = encodeMessage(first);
        Bytes buffer;
        assert(!decodeMessage(buffer));
        for (std::size_t index = 0; index < encoded.size(); ++index)
        {
            buffer.push_back(encoded[index]);
            const Bytes before = buffer;
            const auto message = decodeMessage(buffer);
            if (index + 1 == encoded.size())
            {
                assert(message);
                expectMessage(*message, first);
                assert(buffer.empty());
            }
            else
            {
                assert(!message);
                assert(buffer == before);
            }
        }

        const Bytes encodedSecond = encodeMessage(second);
        buffer = encoded;
        buffer.insert(buffer.end(), encodedSecond.begin(), encodedSecond.end());
        buffer.insert(buffer.end(), encoded.begin(), encoded.end() - 1);
        expectMessage(*decodeMessage(buffer), first);
        expectMessage(*decodeMessage(buffer), second);
        const Bytes remaining = buffer;
        assert(!decodeMessage(buffer));
        assert(buffer == remaining);
        buffer.push_back(encoded.back());
        expectMessage(*decodeMessage(buffer), first);
        assert(buffer.empty());
    }

    void envelopeValidation()
    {
        const Json valid = envelope();
        for (const std::string key : {"version", "type", "payload"})
        {
            Json missing = valid;
            missing.erase(key);
            expectRejected(rawFrame(missing.dump()));
        }
        for (const Json &version : std::array<Json, 10>{0, 1, 3, -1, 2.0, "2", true, nullptr, std::uint64_t{4294967298ULL}, std::numeric_limits<std::uint64_t>::max()})
        {
            Json invalid = valid;
            invalid["version"] = version;
            expectRejected(rawFrame(invalid.dump()));
        }
        for (const Json &invalidText : std::array<Json, 7>{"", " \t\n", 7, false, nullptr, Json::array(), Json::object()})
        {
            for (const std::string key : {"type", "requestId"})
            {
                Json invalid = valid;
                invalid[key] = invalidText;
                expectRejected(rawFrame(invalid.dump()));
            }
        }
        for (const Json &payload : std::array<Json, 6>{nullptr, false, 1, 2.5, "text", Json::array()})
        {
            Json invalid = valid;
            invalid["payload"] = payload;
            expectRejected(rawFrame(invalid.dump()));
            assert(rejects([&] { encodeMessage(ProtocolMessage{"test", std::nullopt, payload}); }));
        }
        Json unknown = valid;
        unknown["userId"] = "host";
        expectRejected(rawFrame(unknown.dump()));
        for (const std::string invalid : {"", " \t\n"})
        {
            assert(rejects([&] { encodeMessage(ProtocolMessage{invalid}); }));
            assert(rejects([&] { encodeMessage(ProtocolMessage{"test", invalid}); }));
        }
        for (const auto text : {"null", "[]", "true", "2", "\"text\"", "{", "not JSON"}) expectRejected(rawFrame(text));
        expectRejected(rawFrame(valid.dump() + " {}"));
        auto whitespace = rawFrame(" \n" + valid.dump() + "\t ");
        assert(decodeMessage(whitespace));
        assert(whitespace.empty());
    }

    void invalidUtf8AndAmbiguousJson()
    {
        const std::string invalidUtf8(1, static_cast<char>(0xff));
        expectRejected(rawFrame(std::string("{\"version\":2,\"type\":\"") + invalidUtf8 + "\",\"payload\":{}}"));
        expectRejected(rawFrame(std::string("\xef\xbb\xbf") + envelope().dump()));
        expectRejected(rawFrame(R"({"version":2,"type":"test","payload":{"text":"\uD800"}})"));
        expectRejected(rawFrame(R"({"version":2,"type":"test","payload":{"number":1e400}})"));
        expectRejected(rawFrame(R"({"version":1,"version":2,"type":"test","payload":{}})"));
        expectRejected(rawFrame(R"({"version":2,"type":"test","payload":{"role":"player","role":"host"}})"));
        auto siblings = rawFrame(R"({"version":2,"type":"test","payload":{"objects":[{"same":1},{"same":2}],"same":3}})");
        assert(decodeMessage(siblings)->payload.at("same") == 3);
        assert(siblings.empty());

        assert(rejects([&] { encodeMessage(ProtocolMessage{invalidUtf8}); }));
        assert(rejects([&] { encodeMessage(ProtocolMessage{"test", invalidUtf8}); }));
        assert(rejects([&] { encodeMessage(ProtocolMessage{"test", std::nullopt, Json{{"text", invalidUtf8}}}); }));
        assert(rejects([&] { encodeMessage(ProtocolMessage{"test", std::nullopt, Json{{invalidUtf8, true}}}); }));
        for (const Json &value : std::array<Json, 5>{Json(std::numeric_limits<double>::infinity()), Json(-std::numeric_limits<double>::infinity()), Json(std::numeric_limits<double>::quiet_NaN()), Json::binary({1, 2}), Json::parse("invalid", nullptr, false)})
        {
            assert(rejects([&] { encodeMessage(ProtocolMessage{"test", std::nullopt, Json{{"nested", Json::array({value})}}}); }));
        }
    }

    void nestingLimit()
    {
        Json payload = Json::object();
        for (std::size_t level = 1; level < MaximumJsonNesting - 1; ++level)
        {
            payload = Json{{"next", std::move(payload)}};
        }
        const ProtocolMessage atLimit{"deep", std::nullopt, payload};
        auto bytes = encodeMessage(atLimit);
        expectMessage(*decodeMessage(bytes), atLimit);
        assert(bytes.empty());
        const ProtocolMessage tooDeep{"deep", std::nullopt, Json{{"next", payload}}};
        assert(rejects([&] { encodeMessage(tooDeep); }));
        Json document{{"version", 2}, {"type", "deep"}, {"payload", tooDeep.payload}};
        expectRejected(rawFrame(document.dump()));
        const std::string arrays = std::string(MaximumJsonNesting, '[') + "0" + std::string(MaximumJsonNesting, ']');
        expectRejected(rawFrame("{\"version\":2,\"type\":\"deep\",\"payload\":{\"array\":" + arrays + "}}"));
    }

    void frameLimitsAndLegacyRejection()
    {
        ProtocolMessage message{"large", std::nullopt, Json{{"text", ""}}};
        const std::size_t overhead = encodeMessage(message).size() - 4;
        message.payload["text"] = std::string(MaximumFrameBytes - overhead, 'x');
        auto bytes = encodeMessage(message);
        assert(bytes.size() == MaximumFrameBytes + 4);
        assert(bytes[0] == 0 && bytes[1] == 1 && bytes[2] == 0 && bytes[3] == 0);
        expectMessage(*decodeMessage(bytes), message);
        assert(bytes.empty());
        message.payload["text"].get_ref<std::string &>().push_back('x');
        assert(rejects([&] { encodeMessage(message); }));
        expectRejected(Bytes{0, 1, 0, 1});
        expectRejected(Bytes{0, 0, 0, 0});
        expectRejected(Bytes{0xff, 0xff, 0xff, 0xff});

        // A complete version-1 binary join is rejected without a compatibility decoder.
        expectRejected(Bytes{0, 0, 0, 11, 1, 1, 0, 0, 0, 5, 'A', 'l', 'i', 'c', 'e'});
    }
}

int main()
{
    roundTripAndUtf8();
    fragmentedAndCoalescedFrames();
    envelopeValidation();
    invalidUtf8AndAmbiguousJson();
    nestingLimit();
    frameLimitsAndLegacyRejection();
}
