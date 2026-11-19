#pragma once

#include "golarion/play/session_code.hpp"
#include "golarion/play/user_token.hpp"

#include <optional>
#include <string>
#include <utility>

namespace golarion::play
{
    struct HostedCampaignData
    {
        std::string id;
        std::string name;
        std::optional<SessionCode> code;
    };

    struct HostedCampaignView
    {
        std::string name;
        std::optional<SessionCode> code;
    };

    class HostedCampaign final
    {
    public:
        explicit HostedCampaign(HostedCampaignData data) : data_(std::move(data)) {}
        HostedCampaignData toData() const { return data_; }
        HostedCampaignView toView() const { return {data_.name, data_.code}; }

    private:
        HostedCampaignData data_;
    };

    struct JoinedCampaignView
    {
        std::string address;
        SessionCode code;
        std::string name;
    };

    struct JoinedCampaignData
    {
        std::string id;
        std::string address;
        SessionCode code;
        std::string name;
        UserToken token;
    };

    class JoinedCampaign final
    {
    public:
        explicit JoinedCampaign(JoinedCampaignData data) : data_(std::move(data)) {}
        JoinedCampaignData toData() const { return data_; }
        JoinedCampaignView toView() const { return {data_.address, data_.code, data_.name}; }

        void updateConnection(std::string address, SessionCode code, std::string name)
        {
            data_.address = std::move(address);
            data_.code = code;
            if (!name.empty()) data_.name = std::move(name);
        }

    private:
        JoinedCampaignData data_;
    };
}
