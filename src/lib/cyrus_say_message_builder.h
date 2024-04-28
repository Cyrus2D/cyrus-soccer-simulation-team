// -*-c++-*-

/*!
  \file say_message_builder.h
  \brief player's say message builder Header File
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

#ifndef CYRUS_PLAYER_SAY_MESSAGE_BUILDER_H
#define CYRUS_PLAYER_SAY_MESSAGE_BUILDER_H

#include <rcsc/common/say_message.h>
//#include <rcsc/common/say_message_parser.h>
#include "cyrus_say_message_parser.h"
#include <rcsc/geom/vector_2d.h>

#include <string>
#include <iostream>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!
  \class PrePassMessage
  \brief Prepass info message encoder
  format:
  "p<unum_pos:4>"
  the length of message == 5
*/
    class PrePassMessage
            : public SayMessage {
    private:

        int M_receiver_unum; //!< pass receiver's uniform number
        Vector2D M_receive_point; //!< desired pass receive point

    public:

        /*!
          \brief construct with raw information
          \param receiver_unum pass receiver's uniform number
          \param receive_point desired pass receive point
          \param ball_pos next ball position
          \param ball_vel next ball velocity
        */
        PrePassMessage( const int receiver_unum,
                        const Vector2D & receive_point)
                : M_receiver_unum( receiver_unum )
                , M_receive_point( receive_point )
        { }

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return PrePassMessageParser::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return PrePassMessageParser::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class PreCrossMessage
            : public SayMessage {
    private:

        int M_receiver_unum; //!< pass receiver's uniform number
        Vector2D M_receive_point; //!< desired pass receive point

    public:

        /*!
          \brief construct with raw information
          \param receiver_unum pass receiver's uniform number
          \param receive_point desired pass receive point
          \param ball_pos next ball position
          \param ball_vel next ball velocity
        */
        PreCrossMessage( const int receiver_unum,
                         const Vector2D & receive_point)
                : M_receiver_unum( receiver_unum )
                , M_receive_point( receive_point )
        { }

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return PreCrossMessageParser::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return PreCrossMessageParser::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class OnePlayerMessage1
            : public SayMessage {
    private:

        int M_unum; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param unum player's unum
          \param player_pos player's global position
        */
        OnePlayerMessage1( const int unum,
                           const Vector2D & player_pos )
                : M_unum( unum ),
                  M_player_pos( player_pos )
        { }

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return OnePlayerMessageParser1::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return OnePlayerMessageParser1::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class OnePlayerMessage2
            : public SayMessage {
    private:

        int M_unum; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param unum player's unum
          \param player_pos player's global position
        */
        OnePlayerMessage2( const int unum,
                           const Vector2D & player_pos )
                : M_unum( unum ),
                  M_player_pos( player_pos )
        { }

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return OnePlayerMessageParser2::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return OnePlayerMessageParser2::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class TwoPlayerMessage01
            : public SayMessage {
    private:

        int M_player_unum[2]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[2]; //!< player's position

        // not usd
        TwoPlayerMessage01();
    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
        */
        TwoPlayerMessage01( const int player0_unum,
                            const Vector2D & player0_pos,
                            const int player1_unum,
                            const Vector2D & player1_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return TwoPlayerMessageParser01::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return TwoPlayerMessageParser01::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class TwoPlayerMessage02
            : public SayMessage {
    private:

        int M_player_unum[2]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[2]; //!< player's position

        // not usd
        TwoPlayerMessage02();
    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
        */
        TwoPlayerMessage02( const int player0_unum,
                            const Vector2D & player0_pos,
                            const int player1_unum,
                            const Vector2D & player1_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return TwoPlayerMessageParser02::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return TwoPlayerMessageParser02::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class TwoPlayerMessage11
            : public SayMessage {
    private:

        int M_player_unum[2]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[2]; //!< player's position

        // not usd
        TwoPlayerMessage11();
    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
        */
        TwoPlayerMessage11( const int player0_unum,
                            const Vector2D & player0_pos,
                            const int player1_unum,
                            const Vector2D & player1_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return TwoPlayerMessageParser11::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return TwoPlayerMessageParser11::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class TwoPlayerMessage12
            : public SayMessage {
    private:

        int M_player_unum[2]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[2]; //!< player's position

        // not usd
        TwoPlayerMessage12();
    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
        */
        TwoPlayerMessage12( const int player0_unum,
                            const Vector2D & player0_pos,
                            const int player1_unum,
                            const Vector2D & player1_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return TwoPlayerMessageParser12::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return TwoPlayerMessageParser12::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class TwoPlayerMessage22
            : public SayMessage {
    private:

        int M_player_unum[2]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[2]; //!< player's position

        // not usd
        TwoPlayerMessage22();
    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
        */
        TwoPlayerMessage22( const int player0_unum,
                            const Vector2D & player0_pos,
                            const int player1_unum,
                            const Vector2D & player1_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return TwoPlayerMessageParser22::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return TwoPlayerMessageParser22::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class ThreePlayerMessage001
            : public SayMessage {
    private:

        int M_player_unum[3]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[3]; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
          \param player2_unum player's unum, if opponent, += 11
          \param player2_pos goalie's global position
        */
        ThreePlayerMessage001( const int player0_unum,
                               const Vector2D & player0_pos,
                               const int player1_unum,
                               const Vector2D & player1_pos,
                               const int player2_unum,
                               const Vector2D & player2_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return ThreePlayerMessageParser001::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return ThreePlayerMessageParser001::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class ThreePlayerMessage002
            : public SayMessage {
    private:

        int M_player_unum[3]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[3]; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
          \param player2_unum player's unum, if opponent, += 11
          \param player2_pos goalie's global position
        */
        ThreePlayerMessage002( const int player0_unum,
                               const Vector2D & player0_pos,
                               const int player1_unum,
                               const Vector2D & player1_pos,
                               const int player2_unum,
                               const Vector2D & player2_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return ThreePlayerMessageParser002::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return ThreePlayerMessageParser002::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class ThreePlayerMessage011
            : public SayMessage {
    private:

        int M_player_unum[3]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[3]; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
          \param player2_unum player's unum, if opponent, += 11
          \param player2_pos goalie's global position
        */
        ThreePlayerMessage011( const int player0_unum,
                               const Vector2D & player0_pos,
                               const int player1_unum,
                               const Vector2D & player1_pos,
                               const int player2_unum,
                               const Vector2D & player2_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return ThreePlayerMessageParser011::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return ThreePlayerMessageParser011::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class ThreePlayerMessage012
            : public SayMessage {
    private:

        int M_player_unum[3]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[3]; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
          \param player2_unum player's unum, if opponent, += 11
          \param player2_pos goalie's global position
        */
        ThreePlayerMessage012( const int player0_unum,
                               const Vector2D & player0_pos,
                               const int player1_unum,
                               const Vector2D & player1_pos,
                               const int player2_unum,
                               const Vector2D & player2_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return ThreePlayerMessageParser012::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return ThreePlayerMessageParser012::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class ThreePlayerMessage022
            : public SayMessage {
    private:

        int M_player_unum[3]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[3]; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
          \param player2_unum player's unum, if opponent, += 11
          \param player2_pos goalie's global position
        */
        ThreePlayerMessage022( const int player0_unum,
                               const Vector2D & player0_pos,
                               const int player1_unum,
                               const Vector2D & player1_pos,
                               const int player2_unum,
                               const Vector2D & player2_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return ThreePlayerMessageParser022::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return ThreePlayerMessageParser022::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class ThreePlayerMessage111
            : public SayMessage {
    private:

        int M_player_unum[3]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[3]; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
          \param player2_unum player's unum, if opponent, += 11
          \param player2_pos goalie's global position
        */
        ThreePlayerMessage111( const int player0_unum,
                               const Vector2D & player0_pos,
                               const int player1_unum,
                               const Vector2D & player1_pos,
                               const int player2_unum,
                               const Vector2D & player2_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return ThreePlayerMessageParser111::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return ThreePlayerMessageParser111::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class ThreePlayerMessage112
            : public SayMessage {
    private:

        int M_player_unum[3]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[3]; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
          \param player2_unum player's unum, if opponent, += 11
          \param player2_pos goalie's global position
        */
        ThreePlayerMessage112( const int player0_unum,
                               const Vector2D & player0_pos,
                               const int player1_unum,
                               const Vector2D & player1_pos,
                               const int player2_unum,
                               const Vector2D & player2_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return ThreePlayerMessageParser112::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return ThreePlayerMessageParser112::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class ThreePlayerMessage122
            : public SayMessage {
    private:

        int M_player_unum[3]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[3]; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
          \param player2_unum player's unum, if opponent, += 11
          \param player2_pos goalie's global position
        */
        ThreePlayerMessage122( const int player0_unum,
                               const Vector2D & player0_pos,
                               const int player1_unum,
                               const Vector2D & player1_pos,
                               const int player2_unum,
                               const Vector2D & player2_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return ThreePlayerMessageParser122::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return ThreePlayerMessageParser122::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

    class ThreePlayerMessage222
            : public SayMessage {
    private:

        int M_player_unum[3]; //!< player's unum [1-22]. if opponent, unum > 11
        Vector2D M_player_pos[3]; //!< player's position

    public:

        /*!
          \brief construct with raw information
          \param player0_unum player's unum, if opponent, += 11
          \param player0_pos goalie's global position
          \param player1_unum player's unum, if opponent, += 11
          \param player1_pos goalie's global position
          \param player2_unum player's unum, if opponent, += 11
          \param player2_pos goalie's global position
        */
        ThreePlayerMessage222( const int player0_unum,
                               const Vector2D & player0_pos,
                               const int player1_unum,
                               const Vector2D & player1_pos,
                               const int player2_unum,
                               const Vector2D & player2_pos );

        /*!
          \brief get the header character of this message
          \return header character of this message
         */
        char header() const
        {
            return ThreePlayerMessageParser222::sheader();
        }

        /*!
          \brief get the length of this message.
          \return the length of encoded message
        */
        static
        int slength()
        {
            return ThreePlayerMessageParser222::slength();
        }

        /*!
          \brief get the length of this message
          \return the length of encoded message
        */
        int length() const
        {
            return slength();
        }

        /*!
          \brief append this info to the audio message
          \param to reference to the message string instance
          \return result status of encoding
        */
        bool appendTo( std::string & to ) const;

        /*!
          \brief append the debug message
          \param os reference to the output stream
          \return reference to the output stream
         */
        std::ostream & printDebug( std::ostream & os ) const;

    };

}

#endif
