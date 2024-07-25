#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cyrus_say_message_builder.h"

#include <rcsc/common/audio_codec.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/math_util.h>

#include <cstring>

namespace rcsc {


    bool
    PrePassMessage::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "PrePassMessage. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeUnumPosToStr4( M_receiver_unum,
                                                    M_receive_point,
                                                    msg ) )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** PrePassMessage.  receiver"
                      << std::endl;

            dlog.addText( Logger::SENSOR,
                          "PrePassMessage. error! receiver=%d pos=(%.1f %.1f)",
                          M_receiver_unum,
                          M_receive_point.x, M_receive_point.y );
            return false;
        }


        if ( (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** PrePassMessage. length"
                      << std::endl;
            dlog.addText( Logger::SENSOR,
                          "PrePassMessage. error!"
                          " illegal message length = %d [%s] ",
                          msg.length(), msg.c_str() );
            return false;
        }

        dlog.addText( Logger::SENSOR,
                      "PrePassMessage. success!"
                      " receiver=%d recv_pos=(%.1f %.1f)"
                      "  -> [%s]",
                      M_receiver_unum,
                      M_receive_point.x, M_receive_point.y,
                      msg.c_str() );

        to += header();
        to += msg;

        return true;
    }

/*-------------------------------------------------------------------*/
/*!
*/
    std::ostream &
    PrePassMessage::printDebug( std::ostream & os ) const
    {
        //     os << "[Pass "
        //        << M_receiver_unum << " ("
        //        << round( M_receive_point.x, 0.1 ) << ',' << round( M_receive_point.y, 0.1 )
        //        << ")]";
        os << "[PPass:" << M_receiver_unum << ']';
        return os;
    }

    bool
    PreCrossMessage::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "PreCrossMessage. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeUnumPosToStr4( M_receiver_unum,
                                                    M_receive_point,
                                                    msg ) )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** PreCrossMessage.  receiver"
                      << std::endl;

            dlog.addText( Logger::SENSOR,
                          "PreCrossMessage. error! receiver=%d pos=(%.1f %.1f)",
                          M_receiver_unum,
                          M_receive_point.x, M_receive_point.y );
            return false;
        }


        if ( (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** PreCrossMessage. length"
                      << std::endl;
            dlog.addText( Logger::SENSOR,
                          "PreCrossMessage. error!"
                          " illegal message length = %d [%s] ",
                          msg.length(), msg.c_str() );
            return false;
        }

        dlog.addText( Logger::SENSOR,
                      "PreCrossMessage. success!"
                      " receiver=%d recv_pos=(%.1f %.1f)"
                      "  -> [%s]",
                      M_receiver_unum,
                      M_receive_point.x, M_receive_point.y,
                      msg.c_str() );

        to += header();
        to += msg;

        return true;
    }

/*-------------------------------------------------------------------*/
/*!
*/
    std::ostream &
    PreCrossMessage::printDebug( std::ostream & os ) const
    {
        //     os << "[Pass "
        //        << M_receiver_unum << " ("
        //        << round( M_receive_point.x, 0.1 ) << ',' << round( M_receive_point.y, 0.1 )
        //        << ")]";
        os << "[PCPass:" << M_receiver_unum << ']';
        return os;
    }


    bool
    OnePlayerMessage1::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "OnePlayerMessage1. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        if ( M_unum < 1 || 22 < M_unum )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** OnePlayerMessage1. illegal unum = "
                      << M_unum
                      << std::endl;
            dlog.addText( Logger::SENSOR,
                          "OnePlayerMessage1. illegal unum = %d",
                          M_unum );
            return false;
        }

        std::int64_t ival = 0;
        double player_x = min_max( -52.49, M_player_pos.x, 52.49 ) + 52.5;
        double player_y = min_max( -33.99, M_player_pos.y, 33.99 ) + 34.0;

        // ival *= 22;
        ival += M_unum - 1;

        ival *= 168;
        ival += static_cast< std::int64_t >( bound( 0.0, rint( player_x / 0.63 ), 167.0 ) );

        ival *= 109;
        ival += static_cast< std::int64_t >( bound( 0.0, rint( player_y / 0.63 ), 108.0 ) );

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** OnePlayerMessage1. "
                      << std::endl;

            dlog.addText( Logger::SENSOR,
                          "OnePlayerMessage1. error!"
                          " unum=%d pos=(%f %f)",
                          M_unum,
                          M_player_pos.x, M_player_pos.y );
            return false;
        }

        dlog.addText( Logger::SENSOR,
                      "OnePlayerMessage1. success!. unum = %d pos=(%f %f) -> [%s]",
                      M_unum,
                      M_player_pos.x, M_player_pos.y,
                      msg.c_str() );

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    OnePlayerMessage1::printDebug( std::ostream & os ) const
    {
        //     os << "[1Player "
        //        << ( M_unum > 11 ? 11 - M_unum : M_unum ) << ' '
        //        << '(' << round( M_player_pos.x, 0.1 ) << ',' << round( M_player_pos.y, 0.1 ) << ')'
        //        << ']';
        os << "[1Player1:"
           << ( M_unum > 11 ? "O_" : "T_" )  << ( M_unum > 11 ? M_unum - 11 : M_unum )
           << ']';
        return os;
    }

    bool
    OnePlayerMessage2::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "OnePlayerMessage2. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        if ( M_unum < 1 || 22 < M_unum )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** OnePlayerMessage2. illegal unum = "
                      << M_unum
                      << std::endl;
            dlog.addText( Logger::SENSOR,
                          "OnePlayerMessage2. illegal unum = %d",
                          M_unum );
            return false;
        }

        std::int64_t ival = 0;
        double player_x = min_max( -52.49, M_player_pos.x, 52.49 ) + 52.5;
        double player_y = min_max( -33.99, M_player_pos.y, 33.99 ) + 34.0;

        // ival *= 22;
        ival += M_unum - 1;

        ival *= 168;
        ival += static_cast< std::int64_t >( bound( 0.0, rint( player_x / 0.63 ), 167.0 ) );

        ival *= 109;
        ival += static_cast< std::int64_t >( bound( 0.0, rint( player_y / 0.63 ), 108.0 ) );

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** OnePlayerMessage2. "
                      << std::endl;

            dlog.addText( Logger::SENSOR,
                          "OnePlayerMessage2. error!"
                          " unum=%d pos=(%f %f)",
                          M_unum,
                          M_player_pos.x, M_player_pos.y );
            return false;
        }

        dlog.addText( Logger::SENSOR,
                      "OnePlayerMessage2. success!. unum = %d pos=(%f %f) -> [%s]",
                      M_unum,
                      M_player_pos.x, M_player_pos.y,
                      msg.c_str() );

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    OnePlayerMessage2::printDebug( std::ostream & os ) const
    {
        //     os << "[1Player "
        //        << ( M_unum > 11 ? 11 - M_unum : M_unum ) << ' '
        //        << '(' << round( M_player_pos.x, 0.1 ) << ',' << round( M_player_pos.y, 0.1 ) << ')'
        //        << ']';
        os << "[1Player2:"
           << ( M_unum > 11 ? "O_" : "T_" )  << ( M_unum > 11 ? M_unum - 11 : M_unum )
           << ']';
        return os;
    }


    TwoPlayerMessage01::TwoPlayerMessage01( const int player0_unum,
                                            const Vector2D & player0_pos,
                                            const int player1_unum,
                                            const Vector2D & player1_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;
    }

    bool
    TwoPlayerMessage01::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "TwoPlayerMessage01. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 2; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** TwoPlayerMessage01. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage01. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** TwoPlayerMessage01. "
                      << std::endl;

            for ( int i = 0; i < 2; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage01. error! unum=%d pos=(%f %f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 2; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage01. success!. unum=%d pos=(%f %f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    TwoPlayerMessage01::printDebug( std::ostream & os ) const
    {
        //     os << "[2Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << ']';
        os << "[2Player01:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << ']';
        return os;
    }

    TwoPlayerMessage02::TwoPlayerMessage02( const int player0_unum,
                                            const Vector2D & player0_pos,
                                            const int player1_unum,
                                            const Vector2D & player1_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;
    }

    bool
    TwoPlayerMessage02::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "TwoPlayerMessage02. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 2; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** TwoPlayerMessage02. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage02. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** TwoPlayerMessage02. "
                      << std::endl;

            for ( int i = 0; i < 2; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage02. error! unum=%d pos=(%f %f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 2; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage02. success!. unum=%d pos=(%f %f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    TwoPlayerMessage02::printDebug( std::ostream & os ) const
    {
        //     os << "[2Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << ']';
        os << "[2Player02:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << ']';
        return os;
    }

    TwoPlayerMessage11::TwoPlayerMessage11( const int player0_unum,
                                            const Vector2D & player0_pos,
                                            const int player1_unum,
                                            const Vector2D & player1_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;
    }

    bool
    TwoPlayerMessage11::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "TwoPlayerMessage11. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 2; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** TwoPlayerMessage11. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage11. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** TwoPlayerMessage11. "
                      << std::endl;

            for ( int i = 0; i < 2; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage11. error! unum=%d pos=(%f %f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 2; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage11. success!. unum=%d pos=(%f %f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    TwoPlayerMessage11::printDebug( std::ostream & os ) const
    {
        //     os << "[2Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << ']';
        os << "[2Player11:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << ']';
        return os;
    }

    TwoPlayerMessage12::TwoPlayerMessage12( const int player0_unum,
                                            const Vector2D & player0_pos,
                                            const int player1_unum,
                                            const Vector2D & player1_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;
    }

    bool
    TwoPlayerMessage12::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "TwoPlayerMessage12. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 2; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** TwoPlayerMessage12. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage12. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** TwoPlayerMessage12. "
                      << std::endl;

            for ( int i = 0; i < 2; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage12. error! unum=%d pos=(%f %f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 2; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage12. success!. unum=%d pos=(%f %f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    TwoPlayerMessage12::printDebug( std::ostream & os ) const
    {
        //     os << "[2Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << ']';
        os << "[2Player12:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << ']';
        return os;
    }

    TwoPlayerMessage22::TwoPlayerMessage22( const int player0_unum,
                                            const Vector2D & player0_pos,
                                            const int player1_unum,
                                            const Vector2D & player1_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;
    }

    bool
    TwoPlayerMessage22::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "TwoPlayerMessage22. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 2; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** TwoPlayerMessage22. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage22. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** TwoPlayerMessage22. "
                      << std::endl;

            for ( int i = 0; i < 2; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage22. error! unum=%d pos=(%f %f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 2; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "TwoPlayerMessage22. success!. unum=%d pos=(%f %f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    TwoPlayerMessage22::printDebug( std::ostream & os ) const
    {
        //     os << "[2Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << ']';
        os << "[2Player22:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << ']';
        return os;
    }


    ThreePlayerMessage001::ThreePlayerMessage001( const int player0_unum,
                                                  const Vector2D & player0_pos,
                                                  const int player1_unum,
                                                  const Vector2D & player1_pos,
                                                  const int player2_unum,
                                                  const Vector2D & player2_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;

        M_player_unum[2] = player2_unum;
        M_player_pos[2] = player2_pos;
    }

    bool
    ThreePlayerMessage001::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "ThreePlayerMessage001. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 3; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** ThreePlayerMessage001. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage001. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** ThreePlayerMessage001. "
                      << std::endl;

            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage001. error! unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage001. success!. unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    ThreePlayerMessage001::printDebug( std::ostream & os ) const
    {
        //     os << "[3Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[2] > 11 ? 11 - M_player_unum[2] : M_player_unum[2] ) << ' '
        //        << '(' << round( M_player_pos[2].x, 0.1 ) << ',' << round( M_player_pos[2].y, 0.1 ) << "))"
        //        << ']';
        os << "[3Player001:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << '|'
           << ( M_player_unum[2] > 11 ? "O_" : "T_" )
           << ( M_player_unum[2] > 11 ? M_player_unum[2] - 11 : M_player_unum[2] ) << ']';

        return os;
    }

    ThreePlayerMessage002::ThreePlayerMessage002( const int player0_unum,
                                                  const Vector2D & player0_pos,
                                                  const int player1_unum,
                                                  const Vector2D & player1_pos,
                                                  const int player2_unum,
                                                  const Vector2D & player2_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;

        M_player_unum[2] = player2_unum;
        M_player_pos[2] = player2_pos;
    }

    bool
    ThreePlayerMessage002::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "ThreePlayerMessage0022. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 3; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** ThreePlayerMessage002. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage002. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** ThreePlayerMessage002. "
                      << std::endl;

            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage002. error! unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage002. success!. unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    ThreePlayerMessage002::printDebug( std::ostream & os ) const
    {
        //     os << "[3Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[2] > 11 ? 11 - M_player_unum[2] : M_player_unum[2] ) << ' '
        //        << '(' << round( M_player_pos[2].x, 0.1 ) << ',' << round( M_player_pos[2].y, 0.1 ) << "))"
        //        << ']';
        os << "[3Player002:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << '|'
           << ( M_player_unum[2] > 11 ? "O_" : "T_" )
           << ( M_player_unum[2] > 11 ? M_player_unum[2] - 11 : M_player_unum[2] ) << ']';

        return os;
    }

    ThreePlayerMessage011::ThreePlayerMessage011( const int player0_unum,
                                                  const Vector2D & player0_pos,
                                                  const int player1_unum,
                                                  const Vector2D & player1_pos,
                                                  const int player2_unum,
                                                  const Vector2D & player2_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;

        M_player_unum[2] = player2_unum;
        M_player_pos[2] = player2_pos;
    }

    bool
    ThreePlayerMessage011::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "ThreePlayerMessage011. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 3; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** ThreePlayerMessage011. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage011. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** ThreePlayerMessage011. "
                      << std::endl;

            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage011. error! unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage011. success!. unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    ThreePlayerMessage011::printDebug( std::ostream & os ) const
    {
        //     os << "[3Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[2] > 11 ? 11 - M_player_unum[2] : M_player_unum[2] ) << ' '
        //        << '(' << round( M_player_pos[2].x, 0.1 ) << ',' << round( M_player_pos[2].y, 0.1 ) << "))"
        //        << ']';
        os << "[3Player011:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << '|'
           << ( M_player_unum[2] > 11 ? "O_" : "T_" )
           << ( M_player_unum[2] > 11 ? M_player_unum[2] - 11 : M_player_unum[2] ) << ']';

        return os;
    }

    ThreePlayerMessage012::ThreePlayerMessage012( const int player0_unum,
                                                  const Vector2D & player0_pos,
                                                  const int player1_unum,
                                                  const Vector2D & player1_pos,
                                                  const int player2_unum,
                                                  const Vector2D & player2_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;

        M_player_unum[2] = player2_unum;
        M_player_pos[2] = player2_pos;
    }

    bool
    ThreePlayerMessage012::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "ThreePlayerMessage012. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 3; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** ThreePlayerMessage012. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage012. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** ThreePlayerMessage012. "
                      << std::endl;

            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage012. error! unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage012. success!. unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    ThreePlayerMessage012::printDebug( std::ostream & os ) const
    {
        //     os << "[3Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[2] > 11 ? 11 - M_player_unum[2] : M_player_unum[2] ) << ' '
        //        << '(' << round( M_player_pos[2].x, 0.1 ) << ',' << round( M_player_pos[2].y, 0.1 ) << "))"
        //        << ']';
        os << "[3Player012:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << '|'
           << ( M_player_unum[2] > 11 ? "O_" : "T_" )
           << ( M_player_unum[2] > 11 ? M_player_unum[2] - 11 : M_player_unum[2] ) << ']';

        return os;
    }

    ThreePlayerMessage022::ThreePlayerMessage022( const int player0_unum,
                                                  const Vector2D & player0_pos,
                                                  const int player1_unum,
                                                  const Vector2D & player1_pos,
                                                  const int player2_unum,
                                                  const Vector2D & player2_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;

        M_player_unum[2] = player2_unum;
        M_player_pos[2] = player2_pos;
    }

    bool
    ThreePlayerMessage022::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "ThreePlayerMessage022. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 3; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** ThreePlayerMessage022. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage022. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** ThreePlayerMessage022. "
                      << std::endl;

            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage022. error! unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage022. success!. unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    ThreePlayerMessage022::printDebug( std::ostream & os ) const
    {
        //     os << "[3Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[2] > 11 ? 11 - M_player_unum[2] : M_player_unum[2] ) << ' '
        //        << '(' << round( M_player_pos[2].x, 0.1 ) << ',' << round( M_player_pos[2].y, 0.1 ) << "))"
        //        << ']';
        os << "[3Player022:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << '|'
           << ( M_player_unum[2] > 11 ? "O_" : "T_" )
           << ( M_player_unum[2] > 11 ? M_player_unum[2] - 11 : M_player_unum[2] ) << ']';

        return os;
    }

    ThreePlayerMessage111::ThreePlayerMessage111( const int player0_unum,
                                                  const Vector2D & player0_pos,
                                                  const int player1_unum,
                                                  const Vector2D & player1_pos,
                                                  const int player2_unum,
                                                  const Vector2D & player2_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;

        M_player_unum[2] = player2_unum;
        M_player_pos[2] = player2_pos;
    }

    bool
    ThreePlayerMessage111::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "ThreePlayerMessage111. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 3; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** ThreePlayerMessage111. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage111. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** ThreePlayerMessage111. "
                      << std::endl;

            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage111. error! unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage111. success!. unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    ThreePlayerMessage111::printDebug( std::ostream & os ) const
    {
        //     os << "[3Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[2] > 11 ? 11 - M_player_unum[2] : M_player_unum[2] ) << ' '
        //        << '(' << round( M_player_pos[2].x, 0.1 ) << ',' << round( M_player_pos[2].y, 0.1 ) << "))"
        //        << ']';
        os << "[3Player111:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << '|'
           << ( M_player_unum[2] > 11 ? "O_" : "T_" )
           << ( M_player_unum[2] > 11 ? M_player_unum[2] - 11 : M_player_unum[2] ) << ']';

        return os;
    }

    ThreePlayerMessage112::ThreePlayerMessage112( const int player0_unum,
                                                  const Vector2D & player0_pos,
                                                  const int player1_unum,
                                                  const Vector2D & player1_pos,
                                                  const int player2_unum,
                                                  const Vector2D & player2_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;

        M_player_unum[2] = player2_unum;
        M_player_pos[2] = player2_pos;
    }

    bool
    ThreePlayerMessage112::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "ThreePlayerMessage112. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 3; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** ThreePlayerMessage112. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage112. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** ThreePlayerMessage112. "
                      << std::endl;

            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage112. error! unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage112. success!. unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    ThreePlayerMessage112::printDebug( std::ostream & os ) const
    {
        //     os << "[3Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[2] > 11 ? 11 - M_player_unum[2] : M_player_unum[2] ) << ' '
        //        << '(' << round( M_player_pos[2].x, 0.1 ) << ',' << round( M_player_pos[2].y, 0.1 ) << "))"
        //        << ']';
        os << "[3Player112:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << '|'
           << ( M_player_unum[2] > 11 ? "O_" : "T_" )
           << ( M_player_unum[2] > 11 ? M_player_unum[2] - 11 : M_player_unum[2] ) << ']';

        return os;
    }

    ThreePlayerMessage122::ThreePlayerMessage122( const int player0_unum,
                                                  const Vector2D & player0_pos,
                                                  const int player1_unum,
                                                  const Vector2D & player1_pos,
                                                  const int player2_unum,
                                                  const Vector2D & player2_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;

        M_player_unum[2] = player2_unum;
        M_player_pos[2] = player2_pos;
    }

    bool
    ThreePlayerMessage122::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "ThreePlayerMessage122. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 3; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** ThreePlayerMessage122. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage122. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** ThreePlayerMessage122. "
                      << std::endl;

            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage122. error! unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage122. success!. unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    ThreePlayerMessage122::printDebug( std::ostream & os ) const
    {
        //     os << "[3Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[2] > 11 ? 11 - M_player_unum[2] : M_player_unum[2] ) << ' '
        //        << '(' << round( M_player_pos[2].x, 0.1 ) << ',' << round( M_player_pos[2].y, 0.1 ) << "))"
        //        << ']';
        os << "[3Player122:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << '|'
           << ( M_player_unum[2] > 11 ? "O_" : "T_" )
           << ( M_player_unum[2] > 11 ? M_player_unum[2] - 11 : M_player_unum[2] ) << ']';

        return os;
    }

    ThreePlayerMessage222::ThreePlayerMessage222( const int player0_unum,
                                                  const Vector2D & player0_pos,
                                                  const int player1_unum,
                                                  const Vector2D & player1_pos,
                                                  const int player2_unum,
                                                  const Vector2D & player2_pos )
    {
        M_player_unum[0] = player0_unum;
        M_player_pos[0] = player0_pos;

        M_player_unum[1] = player1_unum;
        M_player_pos[1] = player1_pos;

        M_player_unum[2] = player2_unum;
        M_player_pos[2] = player2_pos;
    }

    bool
    ThreePlayerMessage222::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "ThreePlayerMessage222. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        std::int64_t ival = 0;
        double dval = 0.0;

        for ( int i = 0; i < 3; ++i )
        {
            if ( M_player_unum[i] < 1 || 22 < M_player_unum[i] )
            {
                std::cerr << __FILE__ << ":" << __LINE__
                          << " ***ERROR*** ThreePlayerMessage222. illegal unum = "
                          << M_player_unum[i]
                          << std::endl;
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage222. illegal unum = %d",
                              M_player_unum[i] );
                return false;
            }

            ival *= 22;
            ival += M_player_unum[i] - 1;

            dval = min_max( -52.49, M_player_pos[i].x, 52.49 ) + 52.5;
            ival *= 168;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 167.0 ) );

            dval = min_max( -33.99, M_player_pos[i].y, 33.99 ) + 34.0;
            ival *= 109;
            ival += static_cast< std::int64_t >( bound( 0.0, rint( dval / 0.63 ), 108.0 ) );
        }

        std::string msg;
        msg.reserve( slength() - 1 );

        if ( ! AudioCodec::i().encodeInt64ToStr( ival, slength() - 1, msg )
             || (int)msg.length() != slength() - 1 )
        {
            std::cerr << __FILE__ << ":" << __LINE__
                      << " ***ERROR*** ThreePlayerMessage222. "
                      << std::endl;

            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage222. error! unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }
            return false;
        }

        if ( dlog.isEnabled( Logger::SENSOR ) )
        {
            for ( int i = 0; i < 3; ++i )
            {
                dlog.addText( Logger::SENSOR,
                              "ThreePlayerMessage222. success!. unum=%d pos=(%.2f %.2f)",
                              M_player_unum[i],
                              M_player_pos[i].x, M_player_pos[i].y );
            }

            dlog.addText( Logger::SENSOR,
                          "--> [%s]", msg.c_str() );
        }

        to += header();
        to += msg;

        return true;
    }

    std::ostream &
    ThreePlayerMessage222::printDebug( std::ostream & os ) const
    {
        //     os << "[3Player "
        //        << '(' << ( M_player_unum[0] > 11 ? 11 - M_player_unum[0] : M_player_unum[0] ) << ' '
        //        << '(' << round( M_player_pos[0].x, 0.1 ) << ',' << round( M_player_pos[0].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[1] > 11 ? 11 - M_player_unum[1] : M_player_unum[1] ) << ' '
        //        << '(' << round( M_player_pos[1].x, 0.1 ) << ',' << round( M_player_pos[1].y, 0.1 ) << "))"
        //        << '(' << ( M_player_unum[2] > 11 ? 11 - M_player_unum[2] : M_player_unum[2] ) << ' '
        //        << '(' << round( M_player_pos[2].x, 0.1 ) << ',' << round( M_player_pos[2].y, 0.1 ) << "))"
        //        << ']';
        os << "[3Player222:"
           << ( M_player_unum[0] > 11 ? "O_" : "T_" )
           << ( M_player_unum[0] > 11 ? M_player_unum[0] - 11 : M_player_unum[0] ) << '|'
           << ( M_player_unum[1] > 11 ? "O_" : "T_" )
           << ( M_player_unum[1] > 11 ? M_player_unum[1] - 11 : M_player_unum[1] ) << '|'
           << ( M_player_unum[2] > 11 ? "O_" : "T_" )
           << ( M_player_unum[2] > 11 ? M_player_unum[2] - 11 : M_player_unum[2] ) << ']';

        return os;
    }

    bool
    StartSetPlayKickMessage::appendTo( std::string & to ) const
    {
        if ( (int)to.length() + slength() > ServerParam::i().playerSayMsgSize() )
        {
            dlog.addText( Logger::SENSOR,
                          "StartSetPlayKickMessage. over the message size : buf = %d, this = %d",
                          to.length(), slength() );
            return false;
        }

        dlog.addText( Logger::SENSOR,
                      "StartSetPlayKickMessage. success! [w]" );

        to += header();

        return true;
    }

/*-------------------------------------------------------------------*/
/*!

*/
    std::ostream &
    StartSetPlayKickMessage::printDebug( std::ostream & os ) const
    {
        os << "[SSPK]";
        return os;
    }
}