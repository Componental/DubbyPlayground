// ChannelMappingMenu.h

#pragma once

#include <iostream>

enum ChannelMappings
{
    NONE,
    PASS,
    EFCT,
    SNTH,
    CHANNELMAPPINGS_LAST
};

enum InOutChannels
{
    ONE,
    TWO,
    THREE,
    FOUR,
    INOUTCHANNELS_LAST
};


namespace daisy
{
    class ChannelMappingMenu
    {
    public:
        int currentChannelMappingOption = 0;
        int currentInOutChannelOption = 0;

        ChannelMappings channelMappings;
        const char *ChannelMappingsStrings[CHANNELMAPPINGS_LAST] =
            {
                "----",
                "PASS",
                "EFCT",
                "SYNT",
            };

        bool hasNone = true;
        bool hasPass = true;
        bool hasEfct = true;
        bool hasSynth = false;

        const char *InOutChannelsStrings[INOUTCHANNELS_LAST] =
            {
                "1",
                "2", 
                "3",
                "4",
            };

        void Init(ChannelMappings c)
        {
            setting = c;
        }

    private:
        ChannelMappings setting;
    };
}