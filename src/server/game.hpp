/*
   Copyright (C) 2003 - 2015 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include "../network.hpp"
#include "player.hpp"

#include "simple_wml.hpp"

#include <map>
#include <vector>

#include "../mt_rng.hpp"

#include <boost/ptr_container/ptr_vector.hpp>
//class player;

namespace wesnothd {

typedef std::map<network::connection,player> player_map;
typedef std::vector<network::connection> user_vector;
typedef std::vector<network::connection> side_vector;

class game
{
public:
	game(player_map& players, const network::connection host=0,
			const std::string& name="", bool save_replays=false,
			const std::string& replay_save_path="");
	~game();

	int id() const { return id_; }
	const std::string& name() const { return name_; }

	bool is_owner(const network::connection player) const { return (player == owner_); }
	bool is_member(const network::connection player) const
	{ return is_player(player) || is_observer(player); }
	bool allow_observers() const;
	bool is_observer(const network::connection player) const;
	bool is_player(const network::connection player) const;

	/** Checks whether the connection's ip address is banned. */
	bool player_is_banned(const network::connection player) const;
	bool level_init() const { return level_.child("snapshot") || level_.child("scenario"); }
	static simple_wml::node* starting_pos(simple_wml::node& data)
	{
		if(simple_wml::node* scenario = data.child("scenario"))
			return scenario;
		else if(simple_wml::node* snapshot = data.child("snapshot"))
			return snapshot;
		else
			return &data;
	}
	static const simple_wml::node* starting_pos(const simple_wml::node& data)
	{
		if(const simple_wml::node* scenario = data.child("scenario"))
			return scenario;
		else if(const simple_wml::node* snapshot = data.child("snapshot"))
			return snapshot;
		else
			return &data;
	}
	const simple_wml::node::child_list & get_sides_list() const
	{
		return starting_pos( level_.root())->children("side");
	}
	bool started() const { return started_; }

	size_t nplayers() const { return players_.size(); }
	size_t nobservers() const { return observers_.size(); }
	size_t current_turn() const { return (nsides_ ? end_turn_ / nsides_ + 1 : 0); }

	void mute_all_observers();

	/**
	 * Mute an observer or give a message of all currently muted observers if no
	 * name is given.
	 */
	void mute_observer(const simple_wml::node& mute, const player_map::const_iterator muter);

	void unmute_observer(const simple_wml::node& unmute, const player_map::const_iterator unmuter);

	/**
	 * Kick a member by name.
	 *
	 * @return                    The network handle of the removed member if
	 *                            successful, '0' otherwise.
	 */
	network::connection kick_member(const simple_wml::node& kick, const player_map::const_iterator kicker);

	/**
	 * Ban and kick a user by name.
	 *
	 * The user does not need to be in this game but logged in.
	 *
	 * @return                    The network handle of the banned player if he
	 *                            was in this game, '0' otherwise.
	 */
	network::connection ban_user(const simple_wml::node& ban, const player_map::const_iterator banner);

	void unban_user(const simple_wml::node& unban, const player_map::const_iterator unbanner);

	/**
	 * Add a user to the game.
	 *
	 * @return                    True iff the user successfully joined the game.
	 */
	bool add_player(const network::connection player, bool observer = false);

	/**
	 * Removes a user from the game.
	 *
	 * @return                    True iff the game ends. That is, if there are
	 *                            no more players or the host left on a not yet
	 *                            started game.
	 */
	bool remove_player(const network::connection player, const bool disconnect=false, const bool destruct=false);

	/** Adds players and observers into one vector and returns that. */
	const user_vector all_game_users() const;

	void start_game(const player_map::const_iterator starter);
	//this is performed just before starting and before [start_game] signal
	//send scenario_diff's specific to each client so that they locally
	//control their human sides
	void perform_controller_tweaks();

	void update_game();

	/** A user (player only?) asks for the next scenario to advance to. */
	void load_next_scenario(const player_map::const_iterator user); //const
	// iceiceice: I unmarked this const because I want to send and record server messages when I fail to tweak sides properly

	/** Resets the side configuration according to the scenario data. */
	void update_side_data();

	/** Let's a player owning a side give it to another player or observer. */
	void transfer_side_control(const network::connection sock, const simple_wml::node& cfg);

	void process_message(simple_wml::document& data, const player_map::iterator user);

	/**
	 * Handles [end_turn], repackages [commands] with private [speak]s in them
	 * and sends the data.
	 * Also filters commands from all but the current player.
	 * Currently removes all commands but [speak] for observers and all but
	 * [speak], [label] and [rename] for players.
	 *
	 * @returns                   True if the turn ended.
	 */
	bool process_turn(simple_wml::document& data, const player_map::const_iterator user);

	/** Handles incoming [whiteboard] data. */
	void process_whiteboard(simple_wml::document& data, const player_map::const_iterator user);
	/** Handles incoming [change_controller_wml] data. */
	void process_change_controller_wml(simple_wml::document& data, const player_map::const_iterator user);

	/**
	 * Set the description to the number of available slots.
	 *
	 * @returns                   True iff the number of slots has changed.
	 */
	bool describe_slots();

	void send_server_message_to_all(const char* message, network::connection exclude=0) const;
	void send_server_message_to_all(const std::string& message, network::connection exclude=0) const
	{
		send_server_message_to_all(message.c_str(), exclude);
	}

	void send_server_message(const char* message, network::connection sock=0, simple_wml::document* doc=NULL) const;
	void send_server_message(const std::string& message, network::connection sock=0, simple_wml::document* doc=NULL) const
	{
		send_server_message(message.c_str(), sock, doc);
	}

	/** Send data to all players in this game except 'exclude'. */
	void send_and_record_server_message(const char* message, const network::connection exclude=0);
	void send_and_record_server_message(const std::string& message,	const network::connection exclude=0)
	{
		send_and_record_server_message(message.c_str(), exclude);
	}

	void send_data(simple_wml::document& data, const network::connection exclude=0, std::string packet_type = "") const;

	void clear_history();
	void record_data(simple_wml::document* data);
	void save_replay();

	/** The full scenario data. */
	simple_wml::document& level() { return level_; }

	/**
	 * Functions to set/get the address of the game's summary description as
	 * sent to players in the lobby.
	 */
	void set_description(simple_wml::node* desc);
	simple_wml::node* description() const { return description_; }

	void set_password(const std::string& passwd) { password_ = passwd; }
	bool password_matches(const std::string& passwd) const {
		return password_.empty() || passwd == password_;
	}

	const std::string& termination_reason() const {
		static const std::string aborted = "aborted";
		static const std::string not_started = "not started";
		return started_ ? (termination_.empty() ? aborted : termination_) : not_started;
	}

	void set_termination_reason(const std::string& reason);

	void require_random(const simple_wml::document &data, const player_map::iterator user);

	void reset_last_synced_context_id() { last_synced_context_id_ = -1; }
	/**
	 * Function which returns true iff 'player' is on 'team'.
	 * AI controlled sides are not considered on a team.
	 */
	bool is_on_team(const simple_wml::string_span& team, const network::connection player) const;
private:
	//forbidden operations
	game(const game&);
	void operator=(const game&);

	size_t current_side() const { return (nsides_ ? end_turn_ % nsides_ : 0); }
	network::connection current_player() const
	{ return (nsides_ ? sides_[current_side()] : 0); }
	bool is_current_player(const network::connection player) const
	{ return (current_player() == player); }
	bool is_muted_observer(const network::connection player) const;
	bool all_observers_muted() const { return all_observers_muted_; }
	void send_muted_observers(const player_map::const_iterator user) const;

	bool send_taken_side(simple_wml::document& cfg, const simple_wml::node::child_list::const_iterator side) const;
	/**
	 * Figures out which side to take and tells that side to the game owner.
	 *
	 * The owner then should send a [scenario_diff] that implements the side
	 * change and a subsequent update_side_data() call makes it actually
	 * happen.
	 * First we look for a side where save_id= or current_player= matches the
	 * new user's name then we search for the first controller="network" side.
	 */
	bool take_side(const player_map::const_iterator user);

	/**
	 * Send [change_controller] message to tell all clients the new controller's name
	 * or controller type (human or ai).
	 */
	void change_controller(const size_t side_num,
			const network::connection sock,
			const std::string& player_name,
			const bool player_left = true,
			const std::string& controller = "");
	void transfer_ai_sides(const network::connection player);
	void send_leave_game(network::connection user) const;
	void send_data_team(simple_wml::document& data, const simple_wml::string_span& team,
			const network::connection exclude=0, std::string packet_type = "") const;
	void send_data_observers(simple_wml::document& data, const network::connection exclude=0, std::string packet_type = "") const;

	/**
	 * Send [observer] tags of all the observers in the game to the user or
	 * everyone if none given.
	 */
	void send_observerjoins(const network::connection sock=0) const;
	void send_observerquit(const player_map::const_iterator observer) const;
	void send_history(const network::connection sock) const;

	/** In case of a host transfer, notify the new host about its status. */
	void notify_new_host();

	/** Shortcut to a convenience function for finding a user by name. */
	player_map::iterator find_user(const simple_wml::string_span& name);

	bool observers_can_label() const { return false; }
	bool observers_can_chat() const { return true; }
	bool is_legal_command(const simple_wml::node& command, const player_map::const_iterator user);

	/**
	 * Checks whether a user has the same IP as members of this game.
	 * If observer is true it only checks against players.
	 * @return  A comma separated string of members with matching IPs.
	 */
	std::string has_same_ip(const network::connection& user, bool observer) const;

	/**
	 * Function which should be called every time a player ends their turn
	 * (i.e. [end_turn] received). This will update the 'turn' attribute for
	 * the game's description when appropriate. Will return true iff there has
	 * been a change.
	 */
	bool end_turn();

	/**
	 * Function to send a list of users to all clients.
	 *
	 * Only sends data if the game is initialized but not yet started.
	 */
	void send_user_list(const network::connection exclude=0) const;

	/** Returns the name of the user or "(unfound)". */
	std::string username(const player_map::const_iterator pl) const;

	/** Returns a comma separated list of user names. */
	std::string list_users(user_vector users, const std::string& func) const;

	/** Function to log when we don't find a connection in player_info_. */
	void missing_user(network::connection socket, const std::string& func) const;

	/** calculates the initial value for sides_, side_controllerds_, nsides_*/
	void reset_sides();
	/** Helps debugging player and observer lists. */
	std::string debug_player_info() const;
	/** Helps debugging controller tweaks. */
	std::string debug_sides_info() const;

	player_map* player_info_;

	static int id_num;
	int id_;

	/** The name of the game. */
	std::string name_;
	std::string password_;

	/** The game host or later owner (if the host left). */
	network::connection owner_;

	/** A vector of players (members owning a side). */
	user_vector players_;

	/** A vector of observers (members not owning a side). */
	user_vector observers_;
	user_vector muted_observers_;

	/** A vector of side owners. */
	side_vector sides_;

	/**
	 * A vector of controller strings indicating the type.
	 * "human"   - a side controlled by a human
	 * "ai"      - a side controlled by an AI
	 * "null"    - an empty side
	 */
	std::vector<std::string> side_controllers_;

	/** Number of sides in the current scenario. */
	int nsides_;
	bool started_;

	/** 
		The current scenario data.´ 
		WRONG! This contains the initial state or the state from which 
		the game was loaded from.
		Using this to make assumptions about the current gamestate is 
		extremely dangerous and should especially not be done for anything 
		that can be nodified by wml (especially by [modify_side]),
		like team_name, controller ... in [side].
		FIXME: move every code here that uses this object to query those
		information to the clients. But note that there are some checks 
		(like controller == null) that are definitely needed by the server and
		in this case we should try to modify the client to inform the server if 
		a change of those properties occur. Ofc we shouldn't update level_ 
		then, but rather store that information in a seperate object 
		(like in side_controllers_).
	*/
	simple_wml::document level_;

	/** Replay data. */
	typedef boost::ptr_vector<simple_wml::document> t_history;
	mutable t_history history_;

	/** Pointer to the game's description in the games_and_users_list_. */
	simple_wml::node* description_;

	int end_turn_;

	bool all_observers_muted_;

	std::vector<std::string> bans_;

	std::string termination_;

	bool save_replays_;
	std::string replay_save_path_;

	/** A wrapper for mersenne twister rng which generates randomness for this game */
	rand_rng::mt_rng rng_;
	int last_synced_context_id_;
};

struct game_is_member {
	game_is_member(network::connection sock) : sock_(sock) {}
	bool operator()(const game& g) const { return g.is_owner(sock_) || g.is_member(sock_); }

private:
	network::connection sock_;
};

struct game_id_matches {
	game_id_matches(int id) : id_(id) {}
	bool operator()(const game& g) const { return g.id() == id_; }

private:
	int id_;
};
}
#endif

