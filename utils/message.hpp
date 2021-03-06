/**
 * This file describes the message structure of all communications between
 * the server and the client within the game.
 *
 * This file is the only file in the server folder that the client should use,
 * because it defines how the server communicates with the client.
 */

#ifndef MJ_UTILS_MESSAGE_HPP
#define MJ_UTILS_MESSAGE_HPP

#include <array>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <iostream>

constexpr unsigned short MJ_SERVER_DEFAULT_PORT = 10000;

namespace msg
{
using id_type = unsigned short;

/**
 * Constants for the different messages and excuses.
 */
static constexpr id_type
    NEW_PLAYER      = 0x3f3f,
    REJECT          = 0x8088,
    START_STREAM    = 0xa000,
    END_STREAM      = 0xa001,
    PING            = 0xefe0,
    TENPAI          = 0x1009,
    NO_TEN          = 0x100a,
    NO_INFO         = 0x6083,
    EXHAUSTIVE_DRAW = 0x100b,
    FOUR_KONGS      = 0x100c,
    NINE_TERMINALS  = 0x100d,
    FOUR_WINDS      = 0x100e,
    TIMEOUT         = 0x0000;

/**
 * The different types of messages specified by the header.
 */
enum class header : char
{
    my_id                   = 'e', /* uid original */
    join_as_player          = 'p', /* magic number */
    join_as_spectator       = 's', /* game id */
//    draw_tile               = 'd', /* uid */
    discard_tile            = 't', /* 9-bit tile unique ID */
    call_with_tile          = 'w', /* 9-bit tile unique ID */
    call_pong               = '3', /* uid */
    call_chow               = 'c', /* uid */
    call_kong               = '4', /* uid */
    call_riichi             = 'r', /* uid */
    call_ron                = '*', /* uid */
    call_tsumo              = '+', /* uid */
//    ask_for_draw            = 'd', /* uid */
    pass_calls              = 'n', /* uid */
    call_tenpai             = 'i',

    ping                    = ';', /* random number (16 bit) */

    reject                  = 'X', /* magic number */
    queue_size              = 'Q', /* number (1,2,3,4) */
    your_id                 = 'I', /* number 16-bit */
    your_position           = 'P', /* number (0,1,2,3) */
    this_player_drew        = 'D', /* player */
    tile                    = 'T', /* 9 bit tile unique ID */
    tsumogiri_tile          = 'G', /* 9 bit tile unique ID */
    this_player_pong        = '#', /* player */
    this_player_chow        = 'C', /* player */
    this_player_kong        = '$', /* player */
    this_player_riichi      = 'R', /* player */
    this_player_ron         = '/', /* player */
    this_player_tsumo       = '-', /* player */
    this_player_won         = 'W', /* num points */
    this_many_points        = 'Z', /* num points */
    dora_indicator          = 'B', /* 9 bit tile unique ID */
    error                   = '!', /* player that caused it */
    this_player_hand        = 'H', /* player */
    closed_hand             = 'K', /* stream */
    yaku_list               = 'M', /* stream */
    winning_yaku            = 'Y', /* yaku_type */
    yaku_fan_count          = 'F', /* int */
    fu_count                = 'U', /* int */
    game_draw               = 'E', /* No Info */
    new_round               = 'N', /* direction + 4*wind */

    timeout                 = '\0'
};

static constexpr std::size_t BUFFER_SIZE = 3;

using buffer = std::array<char,BUFFER_SIZE>;

/**
 * Buffe the message with a header and some data into a buffer.
 *
 * @tparam ObjType The type of the data to be buffered.
 * @param header The header of the message.
 * @param data The data to be buffered.
 *
 * @return A buffer with the header and data.
 */
template<typename ObjType>
constexpr buffer buffer_data(header header, ObjType obj)
{
    return buffer({
        static_cast<char>(header),
        static_cast<char>(obj&0xff),
        static_cast<char>((obj>>8)&0xff)
    });
}

/**
 * @param buf The buffer to be parsed.
 *
 * @return The type of the message stored in the buffer.
 */
constexpr header type(buffer const &buf)
{
    return static_cast<header>(buf[0]);
}

/**
 * @tparam ObjType The type of the data stored in the buffer.
 * @param buf The buffer to be parsed.
 *
 * @return The data stored in the buffer.
 */
template<typename ObjType>
constexpr ObjType data(buffer const &buf)
{
    return static_cast<ObjType>((buf[1]&0xff) | ((buf[2]&0xff)<<8));
}

/**
 * @brief The thread safe queue for receiving and sending messages.
 *
 * @details The class is implemented using a deque and a mutex.
 *
 * @tparam MsgType The type of messages that would be received.
 */
template<typename MsgType>
class queue
{
public:
    using container_type = std::deque<MsgType>;

public:
    /**
     * @brief Construct a new queue object.
     * @param notification Reference to a condition variable to notify when a
     * message is received.
     */
    explicit queue(std::condition_variable &notification) noexcept
        : notification(notification) {}
    ~queue() = default;

    queue(queue &&other) noexcept : container(std::move(other.container)) {}

    void push_front(MsgType &&msg)
    {
        std::scoped_lock lock(mutex);
        container.push_front(std::move(msg));
        notification.notify_one();
    }

    /**
     * @brief Push a message to the queue.
     * @param message The message to be pushed.
     */
    void push_back(MsgType &&message)
    {
        std::scoped_lock lock(mutex);
        container.push_back(std::move(message));
        notification.notify_one();
    }

    /**
     * @brief Flush all messages in the queue.
     */
    void flush()
    {
        std::scoped_lock lock(mutex);
        container.clear();
    }

    /**
     * @brief Pop a message from the front of the queue.
     * @return The message popped from the front of the queue.
     */
    MsgType pop_front()
    {
        std::scoped_lock lock(mutex);
        MsgType elem = container.front();
        container.pop_front();
        return elem;
    }

    /**
     * @brief Check if the queue is empty.
     * @return True if the queue is empty, false otherwise.
     */
    bool inline empty() const noexcept { return container.empty(); }

private:
    container_type container {};
    std::mutex mutex;
    std::condition_variable &notification;
};

}

#endif
