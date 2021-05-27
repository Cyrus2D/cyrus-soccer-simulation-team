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

#ifndef RCSC_COMMON_SAY_MESSAGE_PARSER_H
#define RCSC_COMMON_SAY_MESSAGE_PARSER_H

#include <rcsc/types.h>

#include <boost/shared_ptr.hpp>

#include <string>

//a
//b GoalieMessageParser
//c
//d DefenseLineMessageParser
//e ThreePlayerMessageParser022
//f
//g BallMessageParser
//h PassRequestMessageParser
//i InterceptMessageParser
//j PassMessageParser
//k ThreePlayerMessageParser002
//l ThreePlayerMessageParser011
//m PreCross
//n GoalieAndPlayerMessageParser
//o OffsideLineMessageParser
//r ThreePlayerMessageParser001
//q PrePassMessageParser
//p RecoveryMessageParser
//s StaminaMessageParser
//t TwoPlayerMessageParser01
//u TwoPlayerMessageParser02
//v ThreePlayerMessageParser012
//x
//w WaitRequestMessageParser
//y
//z OnePlayerMessageParser1
//A
//B
//C BallPlayerMessageParser
//D
//E DribbleMessageParser
//F
//G BallGoalieMessageParser
//H
//I
//J OpponentMessageParser
//K
//L TwoPlayerMessageParser
//M MarkMessageParser
//N TeammateMessageParser
//O
//P OnePlayerMessageParser
//Q
//R ThreePlayerMessageParser
//S
//T
//U
//V
//X
//W SelfMessageParser
//Y
//Z OnePlayerMessageParser2

namespace rcsc {

class AudioMemory;
class GameTime;

/*-------------------------------------------------------------------*/
/*!
  \class SayMessageParser
  \brief abstract player's say message parser
 */
class SayMessageParser {
public:

    //! pointer type alias
    typedef boost::shared_ptr< SayMessageParser > Ptr;

private:

    // not used
    SayMessageParser( const SayMessageParser & );
    SayMessageParser & operator=( const SayMessageParser & );

protected:

    /*!
      \brief protected constructer
     */
    SayMessageParser()
      { }

public:

    /*!
      \brief virtual destruct. do nothing.
     */
    virtual
    ~SayMessageParser()
      { }

    /*!
      \brief pure virtual method that returns header character.
      \return header character.
     */
    virtual
    char header() const = 0;

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
    virtual
    int parse( const int sender,
               const double & dir,
               const char * msg,
               const GameTime & current ) = 0;

};

/*-------------------------------------------------------------------*/
/*!
  \class BallMessageParser
  \brief ball info message parser

  format:
  "m<vector<int> action_number:8>" number player or 0(no action)
  the length of message == 9
 */
class MarkMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    MarkMessageParser( boost::shared_ptr< AudioMemory > memory );

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
    int slength() { return 9; }

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
  \class BallMessageParser
  \brief ball info message parser

  format:
  "b<pos_vel:5>"
  the length of message == 6
 */
class BallMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    BallMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'g'; }

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
    int slength() { return 6; }

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
  \class PassMessageParser
  \brief pass info message parser

  format:
  "p<unum_pos:4><pos_vel:5>"
  the length of message == 10
 */
class PassMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    PassMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'j'; }

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

/*-------------------------------------------------------------------*/
/*!
  \class PassMessageParser
  \brief pass info message parser

  format:
  "p<unum_pos:4>"
  the length of message == 5
 */
class PrePassMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    PrePassMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'q'; }

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

class PreCrossMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    PreCrossMessageParser( boost::shared_ptr< AudioMemory > memory );

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
  \class InterceptMessageParser
  \brief intercept info message parser

  format:
  "i<unum:1><cycle:1>"
  the length of message == 3  format:
 */
class InterceptMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    InterceptMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'i'; }

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
    int slength() { return 3; }

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
  \class GoalieMessageParser
  \brief opponent goalie info message parser

  format:
  "b<pos_body:4>"
  the length of message == 5
 */
class GoalieMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    GoalieMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'b'; }

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
  \class GoalieAndPlayerMessageParser
  \brief opponent goalie & field player message parser

  format:
  "g<pos_body:4,unum_pos:3>"
  the length of message == 8
 */
class GoalieAndPlayerMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    GoalieAndPlayerMessageParser( boost::shared_ptr< AudioMemory > memory );

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
    int slength() { return 8; }

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
  \class OffsideLineMessageParser
  \brief offside line info message parser

  format:
  "o<x_rate:1>"
  the length of message == 2
 */
class OffsideLineMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    OffsideLineMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'o'; }

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
    int slength() { return 2; }

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
  \class DefenseLineMessageParser
  \brief offside line info message parser

  format:
  "d<x_rate:1>"
  the length of message == 2
 */
class DefenseLineMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    DefenseLineMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'd'; }

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
    int slength() { return 2; }

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
  \class WaitRequestMessageParser
  \brief wait request message parser

  format:
  "w"
  the length of message == 1
 */
class WaitRequestMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    WaitRequestMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'w'; }

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

/*-------------------------------------------------------------------*/
/*!
  \class PassRequestMessageParser
  \brief pass request (hey pass) message parser

  format:
  "h<pos:3>"
  the length of message == 4
 */
class PassRequestMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    PassRequestMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'h'; }

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

/*-------------------------------------------------------------------*/
/*!
  \class StaminaMessageParser
  \brief stamina rate value message parser

  format:
  "s<rate:1>"
  the length of message == 2
 */
class StaminaMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    StaminaMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 's'; }

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
    int slength() { return 2; }

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
  \class RecoveryMessageParser
  \brief recovery rate value message parser

  format:
  "r<rate:1>"
  the length of message == 2
 */
class RecoveryMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    RecoveryMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'p'; }

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
    int slength() { return 2; }

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
  \class DribbleMessageParser
  \brief dribble target point message parser

  format:
  "D<count_pos:3>"
  the length of message == 4
 */
class DribbleMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    DribbleMessageParser( boost::shared_ptr< AudioMemory > memory );

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

/*-------------------------------------------------------------------*/
/*!
  \class BallGoalieMessageParser
  \brief ball & goalie info message parser

  format:
  "G<bpos_bvel_gpos_gbody:9>"
  the length of message == 10
 */
class BallGoalieMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    BallGoalieMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'G'; }

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

/*-------------------------------------------------------------------*/
/*!
  \class OnePlayerMessageParser
  \brief player info message parser

  format:
  "P<unum_pos:3>"
  the length of message == 4
 */
class OnePlayerMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    OnePlayerMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'P'; }

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

/*-------------------------------------------------------------------*/
/*!
  \class OnePlayerMessageParser
  \brief player info message parser

  format:
  "P<unum_pos:3>"
  the length of message == 4
 */
class OnePlayerMessageParser1
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    OnePlayerMessageParser1( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'z'; }

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

/*-------------------------------------------------------------------*/
/*!
  \class OnePlayerMessageParser
  \brief player info message parser

  format:
  "P<unum_pos:3>"
  the length of message == 4
 */
class OnePlayerMessageParser2
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    OnePlayerMessageParser2( boost::shared_ptr< AudioMemory > memory );

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
/*-------------------------------------------------------------------*/
/*!
  \class TwoPlayerMessageParser
  \brief 2 players info message parser

  format:
  "Q<unum_pos:3,unum_pos:3>"
  the length of message == 7
 */
class TwoPlayerMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    TwoPlayerMessageParser( boost::shared_ptr< AudioMemory > memory );

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
class TwoPlayerMessageParser01
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    TwoPlayerMessageParser01( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 't'; }

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
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    TwoPlayerMessageParser02( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'u'; }

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

/*-------------------------------------------------------------------*/
/*!
  \class ThreePlayerMessageParser
  \brief 3 players info message parser

  format:
  "R<unum_pos:3,unum_pos:3,unum_pos:3>"
  the length of message == 10
 */
class ThreePlayerMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    ThreePlayerMessageParser( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'R'; }

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

/*-------------------------------------------------------------------*/
/*!
  \class SelfMessageParser
  \brief self info message parser

  format:
  "S<pos_body_stamina>"
  the length of message == 5
 */
class SelfMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    SelfMessageParser( boost::shared_ptr< AudioMemory > memory );

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
  \class TeammateMessageParser
  \brief teammate info message parser

  format:
  "T<unum_pos_body>"
  the length of message == 5
 */
class TeammateMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    TeammateMessageParser( boost::shared_ptr< AudioMemory > memory );

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
  \class OpponentMessageParser
  \brief opponent info message parser

  format:
  "O<unum_pos_body>"
  the length of message == 5
 */
class OpponentMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    OpponentMessageParser( boost::shared_ptr< AudioMemory > memory );

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
  \class BallPlayerMessageParser
  \brief ball & player info message parser

  format:
  "B<bpos_bvel_unum_ppos_pbody:9>"
  the length of message == 10
 */
class BallPlayerMessageParser
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    BallPlayerMessageParser( boost::shared_ptr< AudioMemory > memory );

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


class ThreePlayerMessageParser001
    : public SayMessageParser {
private:

    //! pointer to the audio memory
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    ThreePlayerMessageParser001( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'r'; }

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
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    ThreePlayerMessageParser002( boost::shared_ptr< AudioMemory > memory );

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
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    ThreePlayerMessageParser011( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'l'; }

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
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    ThreePlayerMessageParser012( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'v'; }

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
    boost::shared_ptr< AudioMemory > M_memory;

public:

    /*!
      \brief construct with audio memory
      \param memory pointer to the memory
     */
    explicit
    ThreePlayerMessageParser022( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief get the header character.
      \return header character.
     */
    static
    char sheader() { return 'e'; }

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
}

#endif
