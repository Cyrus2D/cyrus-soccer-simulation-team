// -*-c++-*-

/*!
  \file say_message_parser.h
  \brief player's say message parser Header File
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef CYRUS_COMMON_SAY_MESSAGE_PARSER_H
#define CYRUS_COMMON_SAY_MESSAGE_PARSER_H

#include <rcsc/types.h>
#include <rcsc/common/say_message_parser.h>
#include <memory>
#include <string>

//a
//b BallMessageParser
//c StaminaCapacityMessageParser
//d DefenseLineMessageParser
//e GoalieAndPlayerMessageParser
//f
//g GoalieMessageParser
//h PassRequestMessageParser
//i InterceptMessageParser
//j
//k StartSetPlayKickMessageParser
//l
//m ThreePlayerMessageParser122
//n ThreePlayerMessageParser222
//o OffsideLineMessageParser
//p PassMessageParser
//q
//r RecoveryMessageParser
//s StaminaMessageParser
//t
//u
//v
//w WaitRequestMessageParser
//x
//y
//z
//A ThreePlayerMessageParser022
//B BallPlayerMessageParser
//C ThreePlayerMessageParser111
//D DribbleMessageParser
//E ThreePlayerMessageParser112
//F SetplayMessageParser
//G BallGoalieMessageParser
//H PrePassMessageParser
//I PreCrossMessageParser
//J OnePlayerMessageParser1
//K OnePlayerMessageParser2
//L TwoPlayerMessageParser01
//M TwoPlayerMessageParser02
//N TwoPlayerMessageParser11
//O OpponentMessageParser
//P OnePlayerMessageParser
//Q TwoPlayerMessageParser
//R ThreePlayerMessageParser
//S SelfMessageParser
//T TeammateMessageParser
//U TwoPlayerMessageParser12
//V ThreePlayerMessageParser002
//W ThreePlayerMessageParser011
//X ThreePlayerMessageParser012
//Y TwoPlayerMessageParser22
//Z ThreePlayerMessageParser001

namespace rcsc {

    class AudioMemory;
    class GameTime;

    /*-------------------------------------------------------------------*/
/*!
  \class PrePassMessageParser
  \brief pass info message parser
  format:
  "p<unum_pos:4>"
  the length of message == 5
 */
    class PrePassMessageParser
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        PrePassMessageParser( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'H'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 5; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

/*-------------------------------------------------------------------*/
/*!
  \class PreCrossMessageParser
  \brief pass info message parser
  format:
  "p<unum_pos:4>"
  the length of message == 5
 */
    class PreCrossMessageParser
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        PreCrossMessageParser( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'I'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 5; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class OnePlayerMessageParser1
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        OnePlayerMessageParser1( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'J'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 4; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class OnePlayerMessageParser2
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        OnePlayerMessageParser2( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'K'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 4; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class TwoPlayerMessageParser01
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        TwoPlayerMessageParser01( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'L'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 7; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class TwoPlayerMessageParser02
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        TwoPlayerMessageParser02( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'M'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 7; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class TwoPlayerMessageParser11
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        TwoPlayerMessageParser11( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'N'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 7; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class TwoPlayerMessageParser12
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        TwoPlayerMessageParser12( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'U'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 7; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class TwoPlayerMessageParser22
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        TwoPlayerMessageParser22( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'Y'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 7; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class ThreePlayerMessageParser001
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        ThreePlayerMessageParser001( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'Z'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 10; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class ThreePlayerMessageParser002
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        ThreePlayerMessageParser002( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'V'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 10; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class ThreePlayerMessageParser011
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        ThreePlayerMessageParser011( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'W'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 10; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class ThreePlayerMessageParser012
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        ThreePlayerMessageParser012( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'X'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 10; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class ThreePlayerMessageParser022
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        ThreePlayerMessageParser022( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'A'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 10; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class ThreePlayerMessageParser111
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        ThreePlayerMessageParser111( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'C'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 10; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class ThreePlayerMessageParser112
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        ThreePlayerMessageParser112( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'E'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 10; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class ThreePlayerMessageParser122
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        ThreePlayerMessageParser122( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'm'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 10; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class ThreePlayerMessageParser222
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        ThreePlayerMessageParser222( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'n'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 10; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };

    class StartSetPlayKickMessageParser
            : public SayMessageParser {
    private:

        //! pointer to the audio memory
        std::shared_ptr< AudioMemory > M_memory;

    public:

        /*!
          \brief construct with audio memory
          \param memory pointer to the memory
         */
        explicit
        StartSetPlayKickMessageParser( std::shared_ptr< AudioMemory > memory );

        /*!
          \brief get the header character.
          \return header character.
         */
        static
        char sheader() { return 'k'; }

        /*!
          \brief get the header character.
          \return header character.
         */
        char header() const { return sheader(); }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength() { return 1; }

        /*!
          \brief virtual method which analyzes audio messages.
          \param sender sender's uniform number
          \param dir sender's direction
          \param msg raw audio message
          \param current current game time
          \retval bytes read if success
          \retval 0 message ID is not match. other parser should be tried.
          \retval -1 failed to parse
        */
        int parse( const int sender,
                   const double & dir,
                   const char * msg,
                   const GameTime & current );

    };
}

#endif
