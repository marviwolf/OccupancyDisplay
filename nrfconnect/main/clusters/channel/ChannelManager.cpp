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

#include "ChannelManager.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/util/config.h>
#include <lib/support/logging/CHIPLogging.h>

#include <string.h>
#include <vector>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters::Channel;
using namespace chip::Uint8;

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

// create and initialize semaphore
K_SEM_DEFINE(update_signal, 0, 1);

// include the function from C code "display.c"
extern "C" void display_update(void * string, void * signal);

// maximum length of one recieved message (CHIP tool limitation)
#define MAX_MATTER_MESSAGE_LENGTH 951

// char array for passing the message to "display_update"
static char currentC[MAX_MATTER_MESSAGE_LENGTH];

// create and initialize / start the thread
K_THREAD_DEFINE(display_manager, 8192, display_update, currentC, &update_signal, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

ChannelManager::ChannelManager()
{
    ChannelInfoType none;
    // current message
    std::string currentChannel;
}

CHIP_ERROR ChannelManager::HandleGetChannelList(AttributeValueEncoder & aEncoder)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR ChannelManager::HandleGetLineup(AttributeValueEncoder & aEncoder)
{
    LineupInfoType lineup;
    return aEncoder.Encode(lineup);
}

CHIP_ERROR ChannelManager::HandleGetCurrentChannel(AttributeValueEncoder & aEncoder)
{
    return aEncoder.Encode(mCurrentChannel);
}

/** - function is called when "change-channel" command reaches matter framework
 *  - message, that contains display information is given as Charspan
 */
void ChannelManager::HandleChangeChannel(CommandResponseHelper<ChangeChannelResponseType> & helper, const CharSpan & match)
{
    // extract new message
    std::string word = std::string(match.begin(), match.end());
    LOG_INF("ChannelManager recieved data: %s with size %i", word.c_str(), match.size());

    LOG_INF("Current Data: %s with size %i", currentChannel.c_str(), currentChannel.size());

    // assure, that something has changed in the message
    bool updated = !(word == currentChannel);

    // create response that gets transmitted back to the Matter controller
    ChangeChannelResponseType response;

    if (updated) // if new information was recieved
    {
        LOG_INF("New information recieved - display gets updated");
        currentChannel  = std::string(match.begin(), match.end());                   // save new information as current
        response.status = chip::app::Clusters::Channel::ChannelStatusEnum::kSuccess; // answer status set
        response.data   = chip::MakeOptional(CharSpan::fromCharString("Successful update of screen")); // answer message set
        memset(currentC, 0, MAX_MATTER_MESSAGE_LENGTH);                                                // set char array to zero
        memcpy(currentC, word.c_str(), word.size()); // insert new data in char array
        k_sem_give(&update_signal);                  // semaphore is given, so screen will be updated with the new information
    }
    else // if information isnt new
    {
        LOG_INF("Information recieved is not new - no display update");
        response.status = chip::app::Clusters::Channel::ChannelStatusEnum::kNoMatches;           // answer status set
        response.data   = chip::MakeOptional(CharSpan::fromCharString("Information wasnt new")); // answer message set
    }

    helper.Success(response); // send response to matter controller
}

bool ChannelManager::HandleChangeChannelByNumber(const uint16_t & majorNumber, const uint16_t & minorNumber)
{
    return false;
}

bool ChannelManager::HandleSkipChannel(const int16_t & count)
{
    return true;
}

uint32_t ChannelManager::GetFeatureMap(chip::EndpointId endpoint)
{
    return 0;
}