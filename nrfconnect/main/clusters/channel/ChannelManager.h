/**
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <app/clusters/channel-server/channel-server.h>
#include <string>
#include <vector>

using chip::CharSpan;
using chip::app::AttributeValueEncoder;
using chip::app::CommandResponseHelper;
using ChannelDelegate           = chip::app::Clusters::Channel::Delegate;
using ChangeChannelResponseType = chip::app::Clusters::Channel::Commands::ChangeChannelResponse::Type;
using ChannelInfoType           = chip::app::Clusters::Channel::Structs::ChannelInfoStruct::Type;
using LineupInfoType            = chip::app::Clusters::Channel::Structs::LineupInfoStruct::Type;

class ChannelManager : public ChannelDelegate
{
public:
    ChannelManager();

    CHIP_ERROR HandleGetChannelList(AttributeValueEncoder & aEncoder) override;
    CHIP_ERROR HandleGetLineup(AttributeValueEncoder & aEncoder) override;
    CHIP_ERROR HandleGetCurrentChannel(AttributeValueEncoder & aEncoder) override;

    // only needed function
    void HandleChangeChannel(CommandResponseHelper<ChangeChannelResponseType> & helper, const CharSpan & match) override;

    bool HandleChangeChannelByNumber(const uint16_t & majorNumber, const uint16_t & minorNumber) override;
    bool HandleSkipChannel(const int16_t & count) override;

    uint32_t GetFeatureMap(chip::EndpointId endpoint) override;

protected:
    // ChannelInfoType defined by  Matter specification -> not used but needed to conform!
    ChannelInfoType mCurrentChannel;
    // current message as string saved
    std::string currentChannel;

private:
};
