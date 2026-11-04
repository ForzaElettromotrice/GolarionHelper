#include "protocol.hpp"

#include <stdexcept>
#include <utility>

namespace
{
    using Bytes = std::vector<std::uint8_t>;

    void appendInteger(Bytes &bytes, std::uint32_t value)
    {
        for (int shift : {24, 16, 8, 0})
        {
            bytes.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    }

    void appendText(Bytes &bytes, std::string_view text)
    {
        if (text.size() > 128)
        {
            throw std::invalid_argument("protocol text exceeds 128 bytes");
        }
        appendInteger(bytes, static_cast<std::uint32_t>(text.size()));
        bytes.insert(bytes.end(), text.begin(), text.end());
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

    class Reader final
    {
    public:
        explicit Reader(const Bytes &bytes) : bytes_(bytes) {}

        std::uint8_t byte()
        {
            if (position_ == bytes_.size()) throw std::invalid_argument("truncated protocol message");
            return bytes_[position_++];
        }

        std::uint32_t integer()
        {
            std::uint32_t value = 0;
            for (int index = 0; index < 4; ++index) value = (value << 8) | byte();
            return value;
        }

        std::string text()
        {
            const std::uint32_t size = integer();
            if (size > 128 || size > bytes_.size() - position_) throw std::invalid_argument("invalid protocol string size");
            std::string result(bytes_.begin() + position_, bytes_.begin() + position_ + size);
            position_ += size;
            return result;
        }

        void requireEnd() const
        {
            if (position_ != bytes_.size()) throw std::invalid_argument("unexpected protocol data");
        }

    private:
        const Bytes &bytes_;
        std::size_t position_ = 0;
    };
}

namespace golarion::play::detail
{
    std::vector<std::uint8_t> encodeJoin(std::string_view name)
    {
        Bytes payload{1, 1};
        appendText(payload, name);
        return frame(std::move(payload));
    }

    std::vector<std::uint8_t> encodeSnapshot(const GameSnapshot &snapshot)
    {
        Bytes payload{1, 2};
        appendText(payload, snapshot.id);
        appendText(payload, snapshot.name);
        if (snapshot.connectedParticipants.size() > 1024) throw std::invalid_argument("too many protocol participants");
        appendInteger(payload, static_cast<std::uint32_t>(snapshot.connectedParticipants.size()));
        for (const Participant &participant : snapshot.connectedParticipants)
        {
            appendText(payload, participant.id);
            appendText(payload, participant.name);
        }
        return frame(std::move(payload));
    }

    std::vector<std::uint8_t> encodeRejection(std::string_view reason)
    {
        Bytes payload{1, 3};
        appendText(payload, reason);
        return frame(std::move(payload));
    }

    std::optional<Message> extractMessage(std::vector<std::uint8_t> &buffer)
    {
        if (buffer.size() < 4) return std::nullopt;
        Reader header(buffer);
        const std::uint32_t size = header.integer();
        if (size < 2 || size > MaximumFrameBytes) throw std::invalid_argument("invalid protocol frame size");
        if (buffer.size() - 4 < size) return std::nullopt;
        const Bytes payload(buffer.begin() + 4, buffer.begin() + 4 + size);
        Reader reader(payload);
        if (reader.byte() != 1) throw std::invalid_argument("unsupported protocol version");
        const std::uint8_t type = reader.byte();
        Message message;
        if (type == 1)
        {
            message = JoinRequest{reader.text()};
        }
        else if (type == 2)
        {
            GameSnapshot snapshot{reader.text(), reader.text(), {}};
            const std::uint32_t count = reader.integer();
            if (count > 1024) throw std::invalid_argument("too many protocol participants");
            for (std::uint32_t index = 0; index < count; ++index)
            {
                snapshot.connectedParticipants.push_back(Participant{reader.text(), reader.text()});
            }
            message = std::move(snapshot);
        }
        else if (type == 3)
        {
            message = ConnectionRejected{reader.text()};
        }
        else throw std::invalid_argument("unknown protocol message type");
        reader.requireEnd();
        buffer.erase(buffer.begin(), buffer.begin() + 4 + size);
        return message;
    }
}
