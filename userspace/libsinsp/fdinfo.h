/*
Copyright (C) 2013-2014 Draios inc.

This file is part of sysdig.

sysdig is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

sysdig is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with sysdig.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifdef _WIN32
#define CANCELED_FD_NUMBER INT64_MAX
#else
#define CANCELED_FD_NUMBER std::numeric_limits<int64_t>::max()
#endif

class sinsp_protodecoder;

// fd type characters
#define CHAR_FD_FILE			'f'
#define CHAR_FD_IPV4_SOCK		'4'
#define CHAR_FD_IPV6_SOCK		'6'
#define CHAR_FD_DIRECTORY		'd'
#define CHAR_FD_IPV4_SERVSOCK	'2'
#define CHAR_FD_IPV6_SERVSOCK	'3'
#define CHAR_FD_FIFO			'p'
#define CHAR_FD_UNIX_SOCK		'u'
#define CHAR_FD_EVENT			'e'
#define CHAR_FD_UNKNOWN			'o'
#define CHAR_FD_UNSUPPORTED		'X'
#define CHAR_FD_SIGNAL			's'
#define CHAR_FD_EVENTPOLL		'l'
#define CHAR_FD_INOTIFY			'i'
#define CHAR_FD_TIMERFD			't'

/** @defgroup state State management 
 * A collection of classes to query process and FD state.
 *  @{
 */

typedef union _sinsp_sockinfo
{
	ipv4tuple m_ipv4info; ///< The tuple if this an IPv4 socket.
	ipv6tuple m_ipv6info; ///< The tuple if this an IPv6 socket.
	ipv4serverinfo m_ipv4serverinfo;  ///< Information about an IPv4 server socket.
	ipv6serverinfo m_ipv6serverinfo; ///< Information about an IPv6 server socket.
	unix_tuple m_unixinfo; ///< The tuple if this a unix socket.
}sinsp_sockinfo;

class fd_callbacks_info
{
public:
	vector<sinsp_protodecoder*> m_write_callbacks;
	vector<sinsp_protodecoder*> m_read_callbacks;
};

/*!
  \brief File Descriptor information class.
  This class contains the full state for a FD, and a bunch of functions to
  manipulate FDs and retrieve FD information.

  \note As a library user, you won't need to construct thread objects. Rather,
   you get them by calling \ref sinsp_evt::get_fd_info or 
   \ref sinsp_threadinfo::get_fd.
*/template<class T>
class SINSP_PUBLIC sinsp_fdinfo
{
public:
	sinsp_fdinfo();
	sinsp_fdinfo (const sinsp_fdinfo &other) 
	{
		copy(other, false);
	}

	~sinsp_fdinfo()
	{
		if(m_callbaks != NULL)
		{
			delete m_callbaks;
		}

		if(m_usrstate != NULL)
		{
			delete m_usrstate;
		}
	}

	sinsp_fdinfo& operator=(const sinsp_fdinfo& other)
	{
		copy(other, true);
		return *this;
	}

	void reset();
	string* tostring();

	inline void copy(const sinsp_fdinfo &other, bool free_state)
	{
		m_type = other.m_type;
		m_openflags = other.m_openflags;	
		m_sockinfo = other.m_sockinfo;
		m_name = other.m_name;
		m_flags = other.m_flags;
		m_ino = other.m_ino;
		
		if(free_state)
		{
			if(m_callbaks != NULL)
			{
				delete m_callbaks;
			}

			if(m_usrstate != NULL)
			{
				delete m_usrstate;
			}
		}

		if(other.m_callbaks != NULL)
		{
			m_callbaks = new fd_callbacks_info();
			*m_callbaks = *other.m_callbaks;
		}
		else
		{
			m_callbaks = NULL;
		}

		if(other.m_usrstate != NULL)
		{
			m_usrstate = new T(*other.m_usrstate);
		}
		else
		{
			m_usrstate = NULL;
		}
	}

	/*!
	  \brief Return a single ASCII character that identifies the FD type.

	  Refer to the CHAR_FD_* defines in this fdinfo.h.
	*/
	char get_typechar();

	/*!
	  \brief Return an ASCII string that identifies the FD type.

	  Can be on of 'file', 'directory', ipv4', 'ipv6', 'unix', 'pipe', 'event', 'signalfd', 'eventpoll', 'inotify', 'signalfd'.
	*/
	char* get_typestring();

	/*!
	  \brief Return the fd name, after removing unprintable or invalid characters from it.
	*/
	string tostring_clean();

	/*!
	  \brief Returns true if this is a unix socket.
	*/
	bool is_unix_socket()
	{
		return m_type == SCAP_FD_UNIX_SOCK;
	}

	/*!
	  \brief Returns true if this is an IPv4 socket.
	*/
	bool is_ipv4_socket()
	{
		return m_type == SCAP_FD_IPV4_SOCK;
	}

	/*!
	  \brief Returns true if this is an IPv4 socket.
	*/
	bool is_ipv6_socket()
	{
		return m_type == SCAP_FD_IPV6_SOCK;
	}

	/*!
	  \brief Returns true if this is a UDP socket.
	*/
	bool is_udp_socket()
	{
		return m_type == SCAP_FD_IPV4_SOCK && m_sockinfo.m_ipv4info.m_fields.m_l4proto == SCAP_L4_UDP;
	}

	/*!
	  \brief Returns true if this is a unix TCP.
	*/
	bool is_tcp_socket()
	{
		return m_type == SCAP_FD_IPV4_SOCK && m_sockinfo.m_ipv4info.m_fields.m_l4proto == SCAP_L4_TCP;
	}

	/*!
	  \brief Returns true if this is a pipe.
	*/
	bool is_pipe()
	{
		return m_type == SCAP_FD_FIFO;
	}

	/*!
	  \brief Returns true if this is a file.
	*/
	bool is_file()
	{
		return m_type == SCAP_FD_FILE;
	}

	/*!
	  \brief Returns true if this is a directory.
	*/
	bool is_directory()
	{
		return m_type == SCAP_FD_DIRECTORY;
	}

	uint16_t get_serverport()
	{
		if(m_type == SCAP_FD_IPV4_SOCK)
		{
			return m_sockinfo.m_ipv4info.m_fields.m_dport;
		}
		else if(m_type == SCAP_FD_IPV4_SOCK)
		{
			return m_sockinfo.m_ipv6info.m_fields.m_dport;
		}
		else
		{
			return 0;
		}
	}

	/*!
	  \brief If this is a socket, returns the IP protocol. Otherwise, return SCAP_FD_UNKNOWN.
	*/
	scap_l4_proto get_l4proto();

	/*!
	  \brief Used by protocol decoders to register callbacks related to this FD.
	*/
	void register_event_callback(sinsp_pd_callback_type etype, sinsp_protodecoder* dec);

	/*!
	  \brief Used by protocol decoders to unregister callbacks related to this FD.
	*/
	void unregister_event_callback(sinsp_pd_callback_type etype, sinsp_protodecoder* dec);

	/*!
	  \brief Return true if this FD is a socket server
	*/
	inline bool is_role_server()
	{
		return (m_flags & FLAGS_ROLE_SERVER) == FLAGS_ROLE_SERVER;
	}

	/*!
	  \brief Return true if this FD is a socket client
	*/
	inline bool is_role_client()
	{
		return (m_flags & FLAGS_ROLE_CLIENT) == FLAGS_ROLE_CLIENT;
	}

	/*!
	  \brief Return true if this FD is neither a client nor a server
	*/
	inline bool is_role_none()
	{
		return (m_flags & (FLAGS_ROLE_CLIENT | FLAGS_ROLE_SERVER)) == 0;
	}

	scap_fd_type m_type; ///< The fd type, e.g. file, directory, IPv4 socket...
	uint32_t m_openflags; ///< If this FD is a file, the flags that were used when opening it. See the PPM_O_* definitions in driver/ppm_events_public.h.
	
	/*!
	  \brief Socket-specific state.
	  This is uninitialized for non-socket FDs.
	*/
	sinsp_sockinfo m_sockinfo;

	string m_name; ///< Human readable rendering of this FD. For files, this is the full file name. For sockets, this is the tuple. And so on.

	inline bool has_decoder_callbacks()
	{
		return (m_callbaks != NULL);
	}

VISIBILITY_PRIVATE

// Doxygen doesn't understand VISIBILITY_PRIVATE
#ifdef _DOXYGEN
private:
#endif

	/*!
	  \brief FD flags.
	*/
	enum flags
	{
		FLAGS_NONE = 0,
		FLAGS_FROM_PROC = (1 << 0),
		//FLAGS_TRANSACTION = (1 << 1),
		FLAGS_ROLE_CLIENT = (1 << 2),
		FLAGS_ROLE_SERVER = (1 << 3),
		FLAGS_CLOSE_IN_PROGRESS = (1 << 4),
		FLAGS_CLOSE_CANCELED = (1 << 5),
		// Pipe-specific flags
		FLAGS_IS_SOCKET_PIPE = (1 << 6),
		FLAGS_IS_TRACER_FD = (1 << 7),
	};

	void add_filename(const char* fullpath);

	inline bool is_transaction()
	{
		return (m_usrstate != NULL); 
	}

	inline void set_role_server()
	{
		m_flags |= FLAGS_ROLE_SERVER;
	}

	inline void set_role_client()
	{
		m_flags |= FLAGS_ROLE_CLIENT;
	}

	bool set_net_role_by_guessing(sinsp* inspector, 
		sinsp_threadinfo* ptinfo, 
		sinsp_fdinfo_t* pfdinfo,
		bool incoming);

	inline void reset_flags()
	{
		m_flags = FLAGS_NONE;
	}

	inline void set_socketpipe()
	{
		m_flags |= FLAGS_IS_SOCKET_PIPE;
	}

	inline bool is_socketpipe()
	{
		return (m_flags & FLAGS_IS_SOCKET_PIPE) == FLAGS_IS_SOCKET_PIPE; 
	}

	inline bool has_no_role()
	{
		return !is_role_client() && !is_role_server();
	}

	T* m_usrstate;
	uint32_t m_flags;
	uint64_t m_ino;

	fd_callbacks_info* m_callbaks;

	friend class sinsp;
	friend class sinsp_parser;
	friend class sinsp_threadinfo;
	friend class sinsp_analyzer;
	friend class thread_analyzer_info;
	friend class sinsp_analyzer_fd_listener;
	friend class sinsp_fdtable;
	friend class sinsp_filter_check_fd;
	friend class sinsp_filter_check_event;
	friend class lua_cbacks;
	friend class sinsp_proto_detector;
};

/*@}*/

///////////////////////////////////////////////////////////////////////////////
// fd info table
///////////////////////////////////////////////////////////////////////////////
class sinsp_fdtable
{
public:
	sinsp_fdtable(sinsp* inspector);

	inline sinsp_fdinfo_t* find(int64_t fd)
	{
		unordered_map<int64_t, sinsp_fdinfo_t>::iterator fdit;

		//
		// Try looking up in our simple cache
		//
		if(m_last_accessed_fd != -1 && fd == m_last_accessed_fd)
		{
	#ifdef GATHER_INTERNAL_STATS
			m_inspector->m_stats.m_n_cached_fd_lookups++;
	#endif
			return m_last_accessed_fdinfo;
		}

		//
		// Caching failed, do a real lookup
		//
		fdit = m_table.find(fd);

		if(fdit == m_table.end())
		{
	#ifdef GATHER_INTERNAL_STATS
			m_inspector->m_stats.m_n_failed_fd_lookups++;
	#endif
			return NULL;
		}
		else
		{
	#ifdef GATHER_INTERNAL_STATS
			m_inspector->m_stats.m_n_noncached_fd_lookups++;
	#endif
			m_last_accessed_fd = fd;
			m_last_accessed_fdinfo = &(fdit->second);
			return &(fdit->second);
		}
	}
	
	// If the key is already present, overwrite the existing value and return false.
	sinsp_fdinfo_t* add(int64_t fd, sinsp_fdinfo_t* fdinfo);
	// If the key is present, returns true, otherwise returns false.
	void erase(int64_t fd);
	void clear();
	size_t size();
	void reset_cache();

	sinsp* m_inspector;
	unordered_map<int64_t, sinsp_fdinfo_t> m_table;

	//
	// Simple fd cache
	//
	int64_t m_last_accessed_fd;
	sinsp_fdinfo_t *m_last_accessed_fdinfo;
};
