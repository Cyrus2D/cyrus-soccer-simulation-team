//
// Created by nader on 7/14/24.
//

#ifndef CYRUS_CYRUS_AUDIO_MEMORY_H
#define CYRUS_CYRUS_AUDIO_MEMORY_H

#include <rcsc/common/audio_memory.h>
#include <rcsc/common/logger.h>

using namespace rcsc;
class CyrusAudioMemory : public rcsc::AudioMemory {
public:
    struct StartSetPlayKick {
        int sender_; //!< wait request message sender number;

        /*!
          \brief initialize all member
        */
        StartSetPlayKick( const int sender )
                : sender_( sender )
        { }
    };

    CyrusAudioMemory()
            : rcsc::AudioMemory(),
            M_start_set_play_kick_time( -1, 0 )
    { }
    std::vector< StartSetPlayKick > M_start_set_play_kick; //!< heard info
    GameTime M_start_set_play_kick_time; //!< heard time
    void setStartSetPlayKick( const int sender,
                         const GameTime & current ){
        dlog.addText( Logger::WORLD,
                      __FILE__": set heard start set play kick. sender=%d",
                      sender );
        if ( M_start_set_play_kick_time != current )
        {
            M_start_set_play_kick.clear();
        }

        M_start_set_play_kick.emplace_back( sender );
        M_start_set_play_kick_time = current;

        M_time = current;
    }

    const std::vector< StartSetPlayKick > & startSetPlayKick() const
    {
        return M_start_set_play_kick;
    }

    const GameTime & startSetPlayKickTime() const
    {
        return M_start_set_play_kick_time;
    }
};

#endif //CYRUS_CYRUS_AUDIO_MEMORY_H
