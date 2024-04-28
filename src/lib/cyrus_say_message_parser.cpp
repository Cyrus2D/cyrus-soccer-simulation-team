#include "cyrus_say_message_parser.h"
#include <rcsc/common/audio_codec.h>
#include <rcsc/common/audio_memory.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/game_time.h>

#include <cstring>

namespace rcsc {

    PrePassMessageParser::PrePassMessageParser(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

/*-------------------------------------------------------------------*/
/*!
*/
    int
    PrePassMessageParser::parse(const int sender,
                                const double &,
                                const char *msg,
                                const GameTime &current) {
        // format:
        //    "q<unum_pos:4>"
        // the length of message == 5

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "PrePassMessageParser::parse()"
                      << " Illegal pass pass message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "PrePassMessageParser Failed to decode Pass Info [%s]",
                         msg);
            return -1;
        }

        ++msg;

        int receiver_number = 0;
        Vector2D receive_pos;

        if (!AudioCodec::i().decodeStr4ToUnumPos(std::string(msg, 4),
                                                 &receiver_number,
                                                 &receive_pos)) {
            std::cerr << "PrePassMessageParser::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "PrePassMessageParser: Failed to decode Pass Info [%s]",
                         msg);
            return -1;
        }
        msg += 4;

        dlog.addText(Logger::SENSOR,
                     "PrePassMessageParser::parse() success! receiver %d"
                     " recv_pos(%.1f %.1f)",
                     receiver_number,
                     receive_pos.x, receive_pos.y);

        M_memory->setPass(sender, receiver_number, receive_pos, current, true);

        return slength();
    }

    PreCrossMessageParser::PreCrossMessageParser(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

/*-------------------------------------------------------------------*/
/*!
*/
    int
    PreCrossMessageParser::parse(const int sender,
                                 const double &,
                                 const char *msg,
                                 const GameTime &current) {
        // format:
        //    "q<unum_pos:4>"
        // the length of message == 5

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "PrePassMessageParser::parse()"
                      << " Illegal pass pass message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "PrePassMessageParser Failed to decode Pass Info [%s]",
                         msg);
            return -1;
        }

        ++msg;

        int receiver_number = 0;
        Vector2D receive_pos;

        if (!AudioCodec::i().decodeStr4ToUnumPos(std::string(msg, 4),
                                                 &receiver_number,
                                                 &receive_pos)) {
            std::cerr << "PrePassMessageParser::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "PrePassMessageParser: Failed to decode Pass Info [%s]",
                         msg);
            return -1;
        }
        msg += 4;

        dlog.addText(Logger::SENSOR,
                     "PrePassMessageParser::parse() success! receiver %d"
                     " recv_pos(%.1f %.1f)",
                     receiver_number,
                     receive_pos.x, receive_pos.y);

        M_memory->setPass(sender, receiver_number, receive_pos, current, true, true);

        return slength();
    }


    OnePlayerMessageParser1::OnePlayerMessageParser1(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

/*-------------------------------------------------------------------*/
/*!

*/
    int
    OnePlayerMessageParser1::parse(const int sender,
                                   const double &,
                                   const char *msg,
                                   const GameTime &current) {
        // format:
        //    "P<unum_pos:3>"
        // the length of message == 4

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864

        //               74^3 = 405224

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "OnePlayerMessageParser1::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "OnePlayerMessageParser1::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "OnePlayerMessageParser1: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player_unum = Unum_Unknown;
        Vector2D player_pos;

        // 109 > 68/0.63 + 1
        player_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63 + 1
        player_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "OnePlayerMessageParser1: success! "
                     "unum = %d  pos(%.1f %.1f)",
                     player_unum,
                     player_pos.x, player_pos.y);

        M_memory->setPlayer(sender,
                            player_unum,
                            player_pos,
                            current,
                            1);

        return slength();
    }

    OnePlayerMessageParser2::OnePlayerMessageParser2(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

/*-------------------------------------------------------------------*/
/*!

*/
    int
    OnePlayerMessageParser2::parse(const int sender,
                                   const double &,
                                   const char *msg,
                                   const GameTime &current) {
        // format:
        //    "P<unum_pos:3>"
        // the length of message == 4

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864

        //               74^3 = 405224

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "OnePlayerMessageParser2::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "OnePlayerMessageParser2::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "OnePlayerMessageParser2: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player_unum = Unum_Unknown;
        Vector2D player_pos;

        // 109 > 68/0.63 + 1
        player_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63 + 1
        player_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "OnePlayerMessageParser2: success! "
                     "unum = %d  pos(%.1f %.1f)",
                     player_unum,
                     player_pos.x, player_pos.y);

        M_memory->setPlayer(sender,
                            player_unum,
                            player_pos,
                            current,
                            2);

        return slength();
    }


    TwoPlayerMessageParser01::TwoPlayerMessageParser01(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    TwoPlayerMessageParser01::parse(const int sender,
                                    const double &,
                                    const char *msg,
                                    const GameTime &current) {
        // format:
        //    "Q<unum_pos:3,unum_pos3>"
        // the length of message == 7

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^2 = 162299402496

        //                   74^6 = 164206490176

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "TwoPlayerMessageParser01::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "TwoPlayerMessageParser01::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "TwoPlayerMessageParser01: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "TwoPlayerMessageParser01: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            0);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            1);

        return slength();
    }

    TwoPlayerMessageParser02::TwoPlayerMessageParser02(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    TwoPlayerMessageParser02::parse(const int sender,
                                    const double &,
                                    const char *msg,
                                    const GameTime &current) {
        // format:
        //    "Q<unum_pos:3,unum_pos3>"
        // the length of message == 7

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^2 = 162299402496

        //                   74^6 = 164206490176

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "TwoPlayerMessageParser02::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "TwoPlayerMessageParser02::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "TwoPlayerMessageParser02: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "TwoPlayerMessageParser02: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            0);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            2);

        return slength();
    }

    TwoPlayerMessageParser11::TwoPlayerMessageParser11(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    TwoPlayerMessageParser11::parse(const int sender,
                                    const double &,
                                    const char *msg,
                                    const GameTime &current) {
        // format:
        //    "Q<unum_pos:3,unum_pos3>"
        // the length of message == 7

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^2 = 162299402496

        //                   74^6 = 164206490176

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "TwoPlayerMessageParser11::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "TwoPlayerMessageParser11::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "TwoPlayerMessageParser11: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "TwoPlayerMessageParser11: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            1);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            1);

        return slength();
    }

    TwoPlayerMessageParser12::TwoPlayerMessageParser12(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    TwoPlayerMessageParser12::parse(const int sender,
                                    const double &,
                                    const char *msg,
                                    const GameTime &current) {
        // format:
        //    "Q<unum_pos:3,unum_pos3>"
        // the length of message == 7

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^2 = 162299402496

        //                   74^6 = 164206490176

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "TwoPlayerMessageParser12::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "TwoPlayerMessageParser12::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "TwoPlayerMessageParser12: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "TwoPlayerMessageParser12: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            1);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            2);

        return slength();
    }

    TwoPlayerMessageParser22::TwoPlayerMessageParser22(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    TwoPlayerMessageParser22::parse(const int sender,
                                    const double &,
                                    const char *msg,
                                    const GameTime &current) {
        // format:
        //    "Q<unum_pos:3,unum_pos3>"
        // the length of message == 7

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^2 = 162299402496

        //                   74^6 = 164206490176

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "TwoPlayerMessageParser22::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "TwoPlayerMessageParser22::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "TwoPlayerMessageParser22: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "TwoPlayerMessageParser22: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            2);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            2);

        return slength();
    }


    ThreePlayerMessageParser001::ThreePlayerMessageParser001(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    ThreePlayerMessageParser001::parse(const int sender,
                                       const double &,
                                       const char *msg,
                                       const GameTime &current) {
        // format:
        //    "R<unum_pos:3,unum_pos:3,unm_pos:3>"
        // the length of message == 10

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^3 = 65384586487148544

        //                   74^9 = 66540410775079424

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "ThreePlayerMessageParser001::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "ThreePlayerMessageParser001::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "ThreePlayerMessageParser001: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;
        int player3_unum = Unum_Unknown;
        Vector2D player3_pos;

        // 109 > 68/0.63
        player3_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player3_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player3_unum = (ival % 22) + 1;
        ival /= 22;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "ThreePlayerMessageParser001: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y,
                     player3_unum,
                     player3_pos.x, player3_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            0);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            0);
        M_memory->setPlayer(sender,
                            player3_unum,
                            player3_pos,
                            current,
                            1);

        return slength();
    }

    ThreePlayerMessageParser002::ThreePlayerMessageParser002(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    ThreePlayerMessageParser002::parse(const int sender,
                                       const double &,
                                       const char *msg,
                                       const GameTime &current) {
        // format:
        //    "R<unum_pos:3,unum_pos:3,unm_pos:3>"
        // the length of message == 10

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^3 = 65384586487148544

        //                   74^9 = 66540410775079424

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "ThreePlayerMessageParser002::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "ThreePlayerMessageParser002::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "ThreePlayerMessageParser002: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;
        int player3_unum = Unum_Unknown;
        Vector2D player3_pos;

        // 109 > 68/0.63
        player3_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player3_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player3_unum = (ival % 22) + 1;
        ival /= 22;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "ThreePlayerMessageParser002: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y,
                     player3_unum,
                     player3_pos.x, player3_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            0);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            0);
        M_memory->setPlayer(sender,
                            player3_unum,
                            player3_pos,
                            current,
                            2);

        return slength();
    }

    ThreePlayerMessageParser011::ThreePlayerMessageParser011(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    ThreePlayerMessageParser011::parse(const int sender,
                                       const double &,
                                       const char *msg,
                                       const GameTime &current) {
        // format:
        //    "R<unum_pos:3,unum_pos:3,unm_pos:3>"
        // the length of message == 10

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^3 = 65384586487148544

        //                   74^9 = 66540410775079424

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "ThreePlayerMessageParser011::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "ThreePlayerMessageParser011::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "ThreePlayerMessageParser011: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;
        int player3_unum = Unum_Unknown;
        Vector2D player3_pos;

        // 109 > 68/0.63
        player3_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player3_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player3_unum = (ival % 22) + 1;
        ival /= 22;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "ThreePlayerMessageParser011: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y,
                     player3_unum,
                     player3_pos.x, player3_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            0);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            1);
        M_memory->setPlayer(sender,
                            player3_unum,
                            player3_pos,
                            current,
                            1);

        return slength();
    }

    ThreePlayerMessageParser012::ThreePlayerMessageParser012(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    ThreePlayerMessageParser012::parse(const int sender,
                                       const double &,
                                       const char *msg,
                                       const GameTime &current) {
        // format:
        //    "R<unum_pos:3,unum_pos:3,unm_pos:3>"
        // the length of message == 10

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^3 = 65384586487148544

        //                   74^9 = 66540410775079424

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "ThreePlayerMessageParser012::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "ThreePlayerMessageParser012::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "ThreePlayerMessageParser012: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;
        int player3_unum = Unum_Unknown;
        Vector2D player3_pos;

        // 109 > 68/0.63
        player3_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player3_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player3_unum = (ival % 22) + 1;
        ival /= 22;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "ThreePlayerMessageParser012: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y,
                     player3_unum,
                     player3_pos.x, player3_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            0);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            1);
        M_memory->setPlayer(sender,
                            player3_unum,
                            player3_pos,
                            current,
                            2);

        return slength();
    }

    ThreePlayerMessageParser022::ThreePlayerMessageParser022(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    ThreePlayerMessageParser022::parse(const int sender,
                                       const double &,
                                       const char *msg,
                                       const GameTime &current) {
        // format:
        //    "R<unum_pos:3,unum_pos:3,unm_pos:3>"
        // the length of message == 10

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^3 = 65384586487148544

        //                   74^9 = 66540410775079424

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "ThreePlayerMessageParser022::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "ThreePlayerMessageParser022::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "ThreePlayerMessageParser022: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;
        int player3_unum = Unum_Unknown;
        Vector2D player3_pos;

        // 109 > 68/0.63
        player3_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player3_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player3_unum = (ival % 22) + 1;
        ival /= 22;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "ThreePlayerMessageParser022: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y,
                     player3_unum,
                     player3_pos.x, player3_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            0);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            2);
        M_memory->setPlayer(sender,
                            player3_unum,
                            player3_pos,
                            current,
                            2);

        return slength();
    }

    ThreePlayerMessageParser111::ThreePlayerMessageParser111(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    ThreePlayerMessageParser111::parse(const int sender,
                                       const double &,
                                       const char *msg,
                                       const GameTime &current) {
        // format:
        //    "R<unum_pos:3,unum_pos:3,unm_pos:3>"
        // the length of message == 10

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^3 = 65384586487148544

        //                   74^9 = 66540410775079424

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "ThreePlayerMessageParser111::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "ThreePlayerMessageParser111::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "ThreePlayerMessageParser111: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;
        int player3_unum = Unum_Unknown;
        Vector2D player3_pos;

        // 109 > 68/0.63
        player3_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player3_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player3_unum = (ival % 22) + 1;
        ival /= 22;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "ThreePlayerMessageParser111: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y,
                     player3_unum,
                     player3_pos.x, player3_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            1);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            1);
        M_memory->setPlayer(sender,
                            player3_unum,
                            player3_pos,
                            current,
                            1);

        return slength();
    }

    ThreePlayerMessageParser112::ThreePlayerMessageParser112(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    ThreePlayerMessageParser112::parse(const int sender,
                                       const double &,
                                       const char *msg,
                                       const GameTime &current) {
        // format:
        //    "R<unum_pos:3,unum_pos:3,unm_pos:3>"
        // the length of message == 10

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^3 = 65384586487148544

        //                   74^9 = 66540410775079424

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "ThreePlayerMessageParser112::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "ThreePlayerMessageParser112::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "ThreePlayerMessageParser112: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;
        int player3_unum = Unum_Unknown;
        Vector2D player3_pos;

        // 109 > 68/0.63
        player3_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player3_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player3_unum = (ival % 22) + 1;
        ival /= 22;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "ThreePlayerMessageParser112: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y,
                     player3_unum,
                     player3_pos.x, player3_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            1);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            1);
        M_memory->setPlayer(sender,
                            player3_unum,
                            player3_pos,
                            current,
                            2);

        return slength();
    }

    ThreePlayerMessageParser122::ThreePlayerMessageParser122(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    ThreePlayerMessageParser122::parse(const int sender,
                                       const double &,
                                       const char *msg,
                                       const GameTime &current) {
        // format:
        //    "R<unum_pos:3,unum_pos:3,unm_pos:3>"
        // the length of message == 10

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^3 = 65384586487148544

        //                   74^9 = 66540410775079424

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "ThreePlayerMessageParser122::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "ThreePlayerMessageParser122::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "ThreePlayerMessageParser122: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;
        int player3_unum = Unum_Unknown;
        Vector2D player3_pos;

        // 109 > 68/0.63
        player3_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player3_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player3_unum = (ival % 22) + 1;
        ival /= 22;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "ThreePlayerMessageParser122: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y,
                     player3_unum,
                     player3_pos.x, player3_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            1);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            2);
        M_memory->setPlayer(sender,
                            player3_unum,
                            player3_pos,
                            current,
                            2);

        return slength();
    }

    ThreePlayerMessageParser222::ThreePlayerMessageParser222(std::shared_ptr<AudioMemory> memory)
            : M_memory(memory) {

    }

    int
    ThreePlayerMessageParser222::parse(const int sender,
                                       const double &,
                                       const char *msg,
                                       const GameTime &current) {
        // format:
        //    "R<unum_pos:3,unum_pos:3,unm_pos:3>"
        // the length of message == 10

        // ( 22 * 105/0.63 * 68/0.63 ) = 395767.195767196 < 74^3(=405224)
        //  -> 22 * 168 * 109 = 402864
        //     (22 * 168 * 109)^3 = 65384586487148544

        //                   74^9 = 66540410775079424

        if (*msg != sheader()) {
            return 0;
        }

        if ((int) std::strlen(msg) < slength()) {
            std::cerr << "ThreePlayerMessageParser222::parse()"
                      << " Illegal message ["
                      << msg << "] len = " << std::strlen(msg)
                      << std::endl;
            return -1;
        }
        ++msg;

        std::int64_t ival = 0;
        if (!AudioCodec::i().decodeStrToInt64(std::string(msg, slength() - 1),
                                              &ival)) {
            std::cerr << "ThreePlayerMessageParser222::parse()"
                      << " Failed to parse [" << msg << "]"
                      << std::endl;
            dlog.addText(Logger::SENSOR,
                         "ThreePlayerMessageParser222: Failed to decode Player Info [%s]",
                         msg);
            return -1;
        }

        int player1_unum = Unum_Unknown;
        Vector2D player1_pos;
        int player2_unum = Unum_Unknown;
        Vector2D player2_pos;
        int player3_unum = Unum_Unknown;
        Vector2D player3_pos;

        // 109 > 68/0.63
        player3_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player3_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player3_unum = (ival % 22) + 1;
        ival /= 22;

        // 109 > 68/0.63
        player2_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player2_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player2_unum = (ival % 22) + 1;
        ival /= 22;


        // 109 > 68/0.63
        player1_pos.y = (ival % 109) * 0.63 - 34.0;
        ival /= 109;

        // 168 > 105/0.63
        player1_pos.x = (ival % 168) * 0.63 - 52.5;
        ival /= 168;

        // 22
        player1_unum = (ival % 22) + 1;
        ival /= 22;

        dlog.addText(Logger::SENSOR,
                     "ThreePlayerMessageParser222: success! "
                     "(unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)), (unum=%d (%.2f %.2f)) ",
                     player1_unum,
                     player1_pos.x, player1_pos.y,
                     player2_unum,
                     player2_pos.x, player2_pos.y,
                     player3_unum,
                     player3_pos.x, player3_pos.y);

        M_memory->setPlayer(sender,
                            player1_unum,
                            player1_pos,
                            current,
                            2);
        M_memory->setPlayer(sender,
                            player2_unum,
                            player2_pos,
                            current,
                            2);
        M_memory->setPlayer(sender,
                            player3_unum,
                            player3_pos,
                            current,
                            2);

        return slength();
    }
}