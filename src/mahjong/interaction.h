#ifndef MJ_INTERACTION_H
#define MJ_INTERACTION_H

#include "mahjong.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Empty the player's hand. Usually done at the begining 
 * of a new round.
 * 
 * @param hand The hand to empty.
 */
void mj_empty_hand(mj_hand *hand);

/**
 * @brief Add a tile to the player's hand.
 * 
 * @param hand The player's hand.
 * @param tile The tile to add.
 */
void mj_add_tile(mj_hand *hand, mj_tile tile);

/**
 * @brief Remove a tile from the player's hand.
 * 
 * @param hand The player's hand.
 * @param tile The tile to remove.
 * 
 * @return true if the tile was removed, and false if the tile was not found.
 */
mj_bool mj_discard_tile(mj_hand *hand, mj_tile tile);

/**
 * @brief Empty the player's melds. Usually done at the begining 
 * of a new round.
 */
void mj_empty_melds(mj_meld *melds);

/**
 * @brief Add a meld to the player's melds.
 */
void mj_add_meld(mj_meld *melds, mj_triple triple);

/**
 * @brief Determine if a PONG call is valid on a specific tile.
 * 
 * @param hand The hand of the player calling PONG.
 * @param tile The tile that is called.
 * @return True if the PONG call is valid, false otherwise.
 */
mj_bool mj_pong_available(mj_hand hand, mj_tile const tile);

/**
 * @brief Determine if a KONG call (from another player) is valid on
 * a specific tile.
 * 
 * @param hand The hand of the player calling KONG.
 * @param tile The tile that is called.
 * @return True if the KONG call is valid, false otherwise.
 */
mj_bool mj_kong_available(mj_hand hand, mj_tile const tile);

/**
 * @brief Determine if a closed KONG call is valid on a specific tile.
 * 
 * @param hand The hand of the player calling KONG.
 * @param tile The tile that is called.
 * @return True if the closed KONG call is valid, false otherwise.
 */
mj_bool mj_closed_kong_available(mj_hand hand, mj_tile const tile);

/**
 * @brief Determine if a open KONG call is valid on a specific tile.
 * 
 * @param hand The hand of the player calling KONG.
 * @param tile The tile that is called.
 * @return The meld the tile should be added to to make the open KONG,
 * and NULL if the open KONG is not valid.
 */
mj_triple *mj_open_kong_available(mj_meld melds, mj_tile const tile);

/**
 * @brief Determine if a CHOW call is valid on a specific tile.
 * 
 * @param hand The hand of the player calling CHOW.
 * @param tile The tile that is called.
 * @param chow_tiles The different sets of 2 tiles that is able to 
 * chow the called tile (since there are more than 1 way to chow).
 * @return The number of different ways to chow the tile. If 0, the
 * call is invalid.
 */
mj_size mj_chow_available(mj_hand hand, mj_tile const tile, mj_pair *chow_tiles);

#ifdef __cplusplus
}
#endif

#endif
