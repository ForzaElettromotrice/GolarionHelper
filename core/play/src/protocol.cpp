#include "protocol.hpp"

#include <cmath>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace
{
    using Bytes = std::vector<std::uint8_t>;
    using Json = nlohmann::json;

    void validateMessageFields(const golarion::play::detail::ProtocolMessage &message)
    {
        constexpr std::string_view whitespace = " \t\r\n\f\v";
        if (message.type.find_first_not_of(whitespace) == std::string::npos)
        {
            throw std::invalid_argument("protocol message type must not be empty");
        }
        if (message.requestId && message.requestId->find_first_not_of(whitespace) == std::string::npos)
        {
            throw std::invalid_argument("protocol request ID must not be empty");
        }
        if (!message.payload.is_object())
        {
            throw std::invalid_argument("protocol payload must be a JSON object");
        }
    }

    void validatePayload(const Json &payload)
    {
        // The payload starts at depth one inside the envelope; walk without recursion.
        std::vector<std::pair<const Json *, std::size_t>> pending{{&payload, 1}};
        while (!pending.empty())
        {
            const auto [value, depth] = pending.back();
            pending.pop_back();
            if (value->is_binary() || value->is_discarded() || (value->is_number_float() && !std::isfinite(value->get<double>())))
            {
                throw std::invalid_argument("protocol payload contains a value that cannot be represented as JSON");
            }
            if (!value->is_structured()) continue;
            if (depth >= golarion::play::detail::MaximumJsonNesting)
            {
                throw std::invalid_argument("protocol JSON nesting limit exceeded");
            }
            for (const auto &child : *value) pending.emplace_back(&child, depth + 1);
        }
    }

    Json parseDocument(Bytes::const_iterator begin, Bytes::const_iterator end)
    {
        if (end - begin >= 3 && begin[0] == 0xef && begin[1] == 0xbb && begin[2] == 0xbf)
        {
            throw std::invalid_argument("protocol JSON must not contain a byte-order mark");
        }
        std::vector<std::unordered_set<std::string>> objectKeys;
        const auto callback = [&objectKeys](int depth, Json::parse_event_t event, Json &parsed)
        {
            if ((event == Json::parse_event_t::object_start || event == Json::parse_event_t::array_start) && depth >= static_cast<int>(golarion::play::detail::MaximumJsonNesting))
            {
                throw std::invalid_argument("protocol JSON nesting limit exceeded");
            }
            if (event == Json::parse_event_t::object_start) objectKeys.emplace_back();
            if (event == Json::parse_event_t::key && !objectKeys.back().insert(parsed.get<std::string>()).second)
            {
                throw std::invalid_argument("protocol JSON contains a duplicate key");
            }
            if (event == Json::parse_event_t::object_end) objectKeys.pop_back();
            return true;
        };
        try { return Json::parse(begin, end, callback); }
        catch (const Json::exception &error) { throw std::invalid_argument("invalid protocol JSON: " + std::string(error.what())); }
    }

    golarion::play::detail::ProtocolMessage readMessage(Json document)
    {
        if (!document.is_object() || !document.contains("version") || !document.contains("type") || !document.contains("payload"))
        {
            throw std::invalid_argument("protocol JSON envelope is missing required fields");
        }
        if (document.size() != (document.contains("requestId") ? 4u : 3u))
        {
            throw std::invalid_argument("protocol JSON envelope contains unknown fields");
        }
        if (!document["version"].is_number_integer() || document["version"] != golarion::play::detail::JsonProtocolVersion)
        {
            throw std::invalid_argument("unsupported JSON protocol version");
        }
        if (!document["type"].is_string() || (document.contains("requestId") && !document["requestId"].is_string()))
        {
            throw std::invalid_argument("protocol type and request ID must be strings");
        }
        golarion::play::detail::ProtocolMessage message;
        message.type = document["type"].get<std::string>();
        if (document.contains("requestId")) message.requestId = document["requestId"].get<std::string>();
        message.payload = std::move(document["payload"]);
        validateMessageFields(message);
        return message;
    }

    void appendInteger(Bytes &bytes, std::uint32_t value)
    {
        for (int shift : {24, 16, 8, 0})
        {
            bytes.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    }

    Bytes frame(Bytes payload)
    {
        if (payload.size() > golarion::play::detail::MaximumFrameBytes)
        {
            throw std::invalid_argument("protocol frame is too large");
        }
        Bytes result;
        appendInteger(result, static_cast<std::uint32_t>(payload.size()));
        result.insert(result.end(), payload.begin(), payload.end());
        return result;
    }

}

namespace golarion::play::detail
{
    std::vector<std::uint8_t> encodeMessage(const ProtocolMessage &message)
    {
        validateMessageFields(message);
        validatePayload(message.payload);
        std::string text;
        try
        {
            Json document{{"version", JsonProtocolVersion}, {"type", message.type}, {"payload", message.payload}};
            if (message.requestId) document["requestId"] = *message.requestId;
            text = document.dump();
        }
        catch (const Json::exception &error)
        {
            throw std::invalid_argument("invalid protocol JSON: " + std::string(error.what()));
        }
        if (text.size() > MaximumFrameBytes) throw std::invalid_argument("protocol frame is too large");
        return frame(Bytes(text.begin(), text.end()));
    }

    std::optional<ProtocolMessage> decodeMessage(std::vector<std::uint8_t> &buffer)
    {
        if (buffer.size() < 4) return std::nullopt;
        std::uint32_t size = 0;
        for (std::size_t index = 0; index < 4; ++index) size = (size << 8) | buffer[index];
        if (size == 0 || size > MaximumFrameBytes) throw std::invalid_argument("invalid protocol frame size");
        if (buffer.size() - 4 < size) return std::nullopt;
        ProtocolMessage message = readMessage(parseDocument(buffer.cbegin() + 4, buffer.cbegin() + 4 + size));
        buffer.erase(buffer.begin(), buffer.begin() + 4 + size);
        return message;
    }

}
