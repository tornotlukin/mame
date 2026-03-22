// license:BSD-3-Clause
//============================================================
//
//  debugremote_tcp.h - TCP JSON server for remote debugger
//
//  Embeddable TCP server that can be added to any debugger
//  module to enable remote LLM control alongside the native UI.
//
//============================================================

#ifndef MAME_OSD_DEBUGGER_DEBUGREMOTE_TCP_H
#define MAME_OSD_DEBUGGER_DEBUGREMOTE_TCP_H

#pragma once

#include "emu.h"
#include "debug/debugbuf.h"
#include "debug/debugcon.h"
#include "debug/debugcpu.h"
#include "debug/points.h"
#include "debug/textbuf.h"
#include "debugger.h"
#include "distate.h"
#include "fileio.h"

#include <cinttypes>
#include <cstring>
#include <string>
#include <string_view>


//============================================================
//  JSON helpers
//============================================================

static inline std::string json_escape(std::string_view str)
{
	std::string result;
	result.reserve(str.length() + 8);
	for (char ch : str)
	{
		switch (ch)
		{
		case '"':  result += "\\\""; break;
		case '\\': result += "\\\\"; break;
		case '\n': result += "\\n"; break;
		case '\r': result += "\\r"; break;
		case '\t': result += "\\t"; break;
		default:
			if (static_cast<unsigned char>(ch) < 0x20)
			{
				char buf[8];
				snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)ch);
				result += buf;
			}
			else
			{
				result += ch;
			}
			break;
		}
	}
	return result;
}

static inline std::string json_kv(const char *key, std::string_view value)
{
	return string_format("\"%s\":\"%s\"", key, json_escape(value));
}

static inline std::string json_kv_int(const char *key, int64_t value)
{
	return string_format("\"%s\":%" PRId64, key, value);
}

static inline std::string json_kv_hex(const char *key, uint64_t value)
{
	return string_format("\"%s\":\"0x%" PRIx64 "\"", key, value);
}

static inline std::string json_kv_bool(const char *key, bool value)
{
	return string_format("\"%s\":%s", key, value ? "true" : "false");
}

static inline bool json_get_string(const std::string &json, const char *key, std::string &out)
{
	std::string search = string_format("\"%s\"", key);
	size_t pos = json.find(search);
	if (pos == std::string::npos)
		return false;
	pos += search.length();
	while (pos < json.length() && (json[pos] == ':' || json[pos] == ' '))
		pos++;
	if (pos >= json.length() || json[pos] != '"')
		return false;
	pos++;
	out.clear();
	while (pos < json.length() && json[pos] != '"')
	{
		if (json[pos] == '\\' && pos + 1 < json.length())
		{
			pos++;
			switch (json[pos])
			{
			case 'n':  out += '\n'; break;
			case 't':  out += '\t'; break;
			case '"':  out += '"'; break;
			case '\\': out += '\\'; break;
			default:   out += json[pos]; break;
			}
		}
		else
		{
			out += json[pos];
		}
		pos++;
	}
	return true;
}

static inline bool json_get_int(const std::string &json, const char *key, int64_t &out)
{
	std::string search = string_format("\"%s\"", key);
	size_t pos = json.find(search);
	if (pos == std::string::npos)
		return false;
	pos += search.length();
	while (pos < json.length() && (json[pos] == ':' || json[pos] == ' '))
		pos++;
	if (pos >= json.length())
		return false;
	if (json[pos] == '"')
	{
		pos++;
		std::string numstr;
		while (pos < json.length() && json[pos] != '"')
			numstr += json[pos++];
		if (numstr.length() > 2 && numstr[0] == '0' && (numstr[1] == 'x' || numstr[1] == 'X'))
			out = (int64_t)strtoull(numstr.c_str(), nullptr, 16);
		else
			out = (int64_t)strtoll(numstr.c_str(), nullptr, 0);
		return true;
	}
	std::string numstr;
	while (pos < json.length() && (isdigit(json[pos]) || json[pos] == '-' || json[pos] == 'x' || json[pos] == 'X' ||
	       (json[pos] >= 'a' && json[pos] <= 'f') || (json[pos] >= 'A' && json[pos] <= 'F')))
		numstr += json[pos++];
	if (numstr.empty())
		return false;
	if (numstr.length() > 2 && numstr[0] == '0' && (numstr[1] == 'x' || numstr[1] == 'X'))
		out = (int64_t)strtoull(numstr.c_str(), nullptr, 16);
	else
		out = (int64_t)strtoll(numstr.c_str(), nullptr, 0);
	return true;
}


//============================================================
//  remote_tcp_server — embeddable TCP JSON server
//============================================================

class remote_tcp_server
{
public:
	remote_tcp_server() :
		m_machine(nullptr),
		m_maincpu(nullptr),
		m_state(nullptr),
		m_memory(nullptr),
		m_address_space(nullptr),
		m_debugger_cpu(nullptr),
		m_debugger_console(nullptr),
		m_port(12345),
		m_socket(OPEN_FLAG_WRITE | OPEN_FLAG_CREATE),
		m_initialized(false),
		m_readbuf_len(0),
		m_readbuf_offset(0),
		m_stopped_reason("initial"),
		m_send_stop_event(false),
		m_last_console_lines(0)
	{
	}

	// Initialize with port from options
	void configure(int port)
	{
		if (port > 0)
			m_port = port;
	}

	// Set up the server after machine is ready — call from init_debugger or first wait_for_debugger
	bool start(running_machine &machine)
	{
		if (m_initialized)
			return true;

		m_machine = &machine;
		m_maincpu = device_interface_enumerator<cpu_device>(m_machine->root_device()).first();
		if (!m_maincpu)
			return false;

		m_maincpu->interface(m_state);
		m_memory = &m_maincpu->memory();
		m_address_space = &m_memory->space(AS_PROGRAM);
		m_debugger_cpu = &m_machine->debugger().cpu();
		m_debugger_console = &m_machine->debugger().console();

		osd_printf_info("remote debugger: starting TCP server on port %d...\n", m_port);
		std::string socket_name = string_format("socket.localhost:%d", m_port);
		std::error_condition const filerr = m_socket.open(socket_name);
		if (filerr)
		{
			osd_printf_error("remote debugger: failed to listen on port %d (error: %s)\n", m_port, filerr.message());
			return false;
		}
		osd_printf_info("remote debugger: listening on localhost:%d\n", m_port);

		send_event("connected", string_format("%s,%s",
			json_kv("cpu", m_maincpu->shortname()),
			json_kv("machine", m_machine->system().name)));

		m_initialized = true;
		return true;
	}

	// Shut down
	void stop()
	{
		if (m_socket.is_open())
			m_socket.close();
		m_initialized = false;
	}

	bool is_active() const { return m_initialized; }

	// Process any pending TCP requests (non-blocking) — call from wait_for_debugger and debugger_update
	void poll()
	{
		if (!m_initialized)
			return;
		std::string line;
		while (read_line(line))
			handle_request(line);
	}

	// Send stop notification when execution halts
	void notify_stop(device_t &device)
	{
		if (!m_initialized)
			return;

		device_debug *debug = m_debugger_console->get_visible_cpu()->debug();
		debug_breakpoint *bp = debug->triggered_breakpoint();
		debug_watchpoint *wp = debug->triggered_watchpoint();

		if (bp)
		{
			m_stopped_reason = "breakpoint";
			send_event("breakpoint_hit", string_format("%s,%s",
				json_kv_int("bp_id", bp->index()),
				json_kv_hex("pc", bp->address())));
		}
		else if (wp)
		{
			m_stopped_reason = "watchpoint";
			send_event("watchpoint_hit", string_format("%s,%s",
				json_kv_int("wp_id", wp->index()),
				json_kv_hex("address", wp->address())));
		}
		else if (m_send_stop_event)
		{
			send_event("stopped", string_format("%s,%s",
				json_kv("reason", m_stopped_reason),
				json_kv_hex("pc", m_state->state_int(STATE_GENPC))));
		}
		m_send_stop_event = false;
	}

	// Mark that we should send a stop event next time
	void mark_resumed() { m_send_stop_event = true; }

private:
	// Socket I/O
	int readchar()
	{
		if (!m_socket.is_open())
			return -1;
		if (m_readbuf_offset == m_readbuf_len)
		{
			m_readbuf_offset = 0;
			m_readbuf_len = m_socket.read(m_readbuf, sizeof(m_readbuf));
			if (m_readbuf_len == 0)
				return -1;
		}
		return (int)m_readbuf[m_readbuf_offset++];
	}

	bool read_line(std::string &line)
	{
		while (true)
		{
			int ch = readchar();
			if (ch < 0)
				return false;
			if (ch == '\n')
			{
				line = m_linebuf;
				m_linebuf.clear();
				return true;
			}
			if (ch != '\r')
				m_linebuf += (char)ch;
		}
	}

	void send_line(const std::string &line)
	{
		if (!m_socket.is_open())
			return;
		std::string msg = line + "\n";
		m_socket.write(msg.c_str(), msg.length());
	}

	void send_result(int64_t id, const std::string &output)
	{
		send_line(string_format("{%s,%s,%s}",
			json_kv_int("id", id), json_kv("type", "result"), json_kv("output", output)));
	}

	void send_error(int64_t id, const std::string &error)
	{
		send_line(string_format("{%s,%s,%s}",
			json_kv_int("id", id), json_kv("type", "error"), json_kv("error", error)));
	}

	void send_event(const char *event_type, const std::string &extra_fields)
	{
		std::string response = string_format("{%s,%s",
			json_kv("type", "event"), json_kv("event", event_type));
		if (!extra_fields.empty())
			response += "," + extra_fields;
		response += "}";
		send_line(response);
	}

	std::string capture_console_output()
	{
		// Read new lines from the console text buffer since last capture
		// Don't clear the buffer — let the debugger UI keep its history
		text_buffer &textbuf = m_debugger_console->get_console_textbuf();
		u32 num_lines = text_buffer_num_lines(textbuf);

		std::string result;
		if (num_lines > m_last_console_lines)
		{
			auto lines = text_buffer_lines(textbuf);
			u32 skip = m_last_console_lines;
			u32 cur = 0;
			for (auto it = lines.begin(); it != lines.end(); ++it)
			{
				if (cur >= skip)
				{
					if (!result.empty())
						result += "\n";
					result += std::string(*it);
				}
				cur++;
			}
		}
		m_last_console_lines = num_lines;
		return result;
	}

	address_space &get_space_for_request(const std::string &json)
	{
		device_t *visiblecpu = m_debugger_console->get_visible_cpu();
		device_memory_interface *memintf = nullptr;
		if (visiblecpu)
			visiblecpu->interface(memintf);
		if (!memintf)
			memintf = m_memory;

		std::string space_name;
		if (json_get_string(json, "space", space_name))
		{
			if (space_name == "data" && memintf->has_space(AS_DATA))
				return memintf->space(AS_DATA);
			else if (space_name == "io" && memintf->has_space(AS_IO))
				return memintf->space(AS_IO);
			else if (space_name == "opcodes" && memintf->has_space(AS_OPCODES))
				return memintf->space(AS_OPCODES);
		}
		return memintf->space(AS_PROGRAM);
	}

	// Request dispatcher
	void handle_request(const std::string &json)
	{
		int64_t id = 0;
		json_get_int(json, "id", id);

		std::string type;
		if (!json_get_string(json, "type", type))
		{
			send_error(id, "missing 'type' field");
			return;
		}

		if (type == "command")           handle_command(json, id);
		else if (type == "read_memory")  handle_read_memory(json, id);
		else if (type == "write_memory") handle_write_memory(json, id);
		else if (type == "get_registers") handle_get_registers(json, id);
		else if (type == "disassemble")  handle_disassemble(json, id);
		else if (type == "evaluate")     handle_evaluate(json, id);
		else if (type == "get_state")    handle_get_state(json, id);
		else if (type == "get_breakpoints") handle_get_breakpoints(json, id);
		else if (type == "break")        handle_break(json, id);
		else send_error(id, string_format("unknown type: %s", type));
	}

	void handle_command(const std::string &json, int64_t id)
	{
		std::string cmd;
		if (!json_get_string(json, "cmd", cmd)) { send_error(id, "missing 'cmd' field"); return; }

		// Sync our line counter before executing
		text_buffer &textbuf = m_debugger_console->get_console_textbuf();
		m_last_console_lines = text_buffer_num_lines(textbuf);

		// Execute with echo=true so the command appears in the debugger console
		CMDERR result = m_debugger_console->execute_command(cmd, true);
		if (result.error_class() != CMDERR::NONE) { send_error(id, debugger_console::cmderr_to_string(result)); return; }
		send_result(id, capture_console_output());
	}

	void handle_read_memory(const std::string &json, int64_t id)
	{
		int64_t addr = 0, len = 1;
		if (!json_get_int(json, "addr", addr)) { send_error(id, "missing 'addr' field"); return; }
		json_get_int(json, "len", len);
		if (len < 1) len = 1;
		if (len > 65536) len = 65536;
		address_space &space = get_space_for_request(json);
		auto se = m_machine->disable_side_effects();
		std::string hexdata, ascii;
		hexdata.reserve(len * 2);
		ascii.reserve(len);
		for (int64_t i = 0; i < len; i++)
		{
			uint8_t byte = space.read_byte(addr + i);
			char hex[3];
			snprintf(hex, sizeof(hex), "%02x", byte);
			hexdata += hex;
			ascii += (byte >= 0x20 && byte < 0x7f) ? (char)byte : '.';
		}
		send_line(string_format("{%s,%s,%s,%s,%s}",
			json_kv_int("id", id), json_kv("type", "memory"),
			json_kv_hex("addr", (uint64_t)addr), json_kv("data", hexdata), json_kv("ascii", ascii)));
	}

	void handle_write_memory(const std::string &json, int64_t id)
	{
		int64_t addr = 0;
		if (!json_get_int(json, "addr", addr)) { send_error(id, "missing 'addr' field"); return; }
		std::string data;
		if (!json_get_string(json, "data", data)) { send_error(id, "missing 'data' field"); return; }
		address_space &space = get_space_for_request(json);
		int bytes_written = 0;
		for (size_t i = 0; i + 1 < data.length(); i += 2)
		{
			char hex[3] = { data[i], data[i + 1], 0 };
			space.write_byte(addr + bytes_written, (uint8_t)strtoul(hex, nullptr, 16));
			bytes_written++;
		}
		send_result(id, string_format("wrote %d bytes at 0x%" PRIx64, bytes_written, (uint64_t)addr));
	}

	void handle_get_registers(const std::string &json, int64_t id)
	{
		device_t *visiblecpu = m_debugger_console->get_visible_cpu();
		device_state_interface *state = nullptr;
		if (visiblecpu) visiblecpu->interface(state);
		if (!state) state = m_state;
		if (!state) { send_error(id, "no CPU state available"); return; }
		std::string regs;
		for (const auto &entry : state->state_entries())
		{
			if (entry->divider() || !entry->visible()) continue;
			if (!regs.empty()) regs += ",";
			regs += string_format("\"%s\":\"%s\"", json_escape(entry->symbol()), json_escape(entry->to_string()));
		}
		send_line(string_format("{%s,%s,%s,\"regs\":{%s}}",
			json_kv_int("id", id), json_kv("type", "registers"),
			json_kv("cpu", visiblecpu ? visiblecpu->shortname() : m_maincpu->shortname()), regs));
	}

	void handle_disassemble(const std::string &json, int64_t id)
	{
		int64_t addr = 0, count = 10;
		json_get_int(json, "addr", addr);
		json_get_int(json, "count", count);
		if (count < 1) count = 1;
		if (count > 200) count = 200;
		device_t *cpu = m_debugger_console->get_visible_cpu();
		if (!cpu) cpu = m_maincpu;
		debug_disasm_buffer disasm(*cpu);
		std::string output;
		offs_t pc = (offs_t)addr;
		for (int64_t i = 0; i < count; i++)
		{
			std::string instruction;
			offs_t next_pc, size;
			u32 info;
			disasm.disassemble(pc, instruction, next_pc, size, info);
			if (!output.empty()) output += "\n";
			output += string_format("%s: %-12s %s", disasm.pc_to_string(pc), disasm.data_to_string(pc, size, true), instruction);
			pc = next_pc;
		}
		send_result(id, output);
	}

	void handle_evaluate(const std::string &json, int64_t id)
	{
		std::string expr;
		if (!json_get_string(json, "expr", expr)) { send_error(id, "missing 'expr' field"); return; }
		text_buffer &textbuf = m_debugger_console->get_console_textbuf();
		m_last_console_lines = text_buffer_num_lines(textbuf);
		CMDERR result = m_debugger_console->execute_command(string_format("print %s", expr), true);
		if (result.error_class() != CMDERR::NONE) { send_error(id, string_format("expression error: %s", expr)); return; }
		send_line(string_format("{%s,%s,%s,%s}",
			json_kv_int("id", id), json_kv("type", "eval_result"),
			json_kv("expr", expr), json_kv("result", capture_console_output())));
	}

	void handle_get_state(const std::string &json, int64_t id)
	{
		bool stopped = m_debugger_cpu->is_stopped();
		device_t *visiblecpu = m_debugger_console->get_visible_cpu();
		device_state_interface *state = nullptr;
		if (visiblecpu) visiblecpu->interface(state);
		if (!state) state = m_state;
		uint64_t pc = state ? state->state_int(STATE_GENPC) : 0;
		send_line(string_format("{%s,%s,%s,%s,%s,%s}",
			json_kv_int("id", id), json_kv("type", "state"), json_kv_bool("stopped", stopped),
			json_kv_hex("pc", pc), json_kv("cpu", visiblecpu ? visiblecpu->shortname() : m_maincpu->shortname()),
			json_kv("machine", m_machine->system().name)));
	}

	void handle_get_breakpoints(const std::string &json, int64_t id)
	{
		text_buffer &textbuf = m_debugger_console->get_console_textbuf();
		m_last_console_lines = text_buffer_num_lines(textbuf);
		m_debugger_console->execute_command("bplist", true);
		send_result(id, capture_console_output());
	}

	void handle_break(const std::string &json, int64_t id)
	{
		m_machine->debugger().debug_break();
		m_stopped_reason = "break";
		send_result(id, "execution halted");
	}

	// Members
	running_machine *m_machine;
	device_t *m_maincpu;
	device_state_interface *m_state;
	device_memory_interface *m_memory;
	address_space *m_address_space;
	debugger_cpu *m_debugger_cpu;
	debugger_console *m_debugger_console;
	int m_port;
	emu_file m_socket;
	bool m_initialized;
	uint8_t m_readbuf[4096];
	uint32_t m_readbuf_len;
	uint32_t m_readbuf_offset;
	std::string m_linebuf;
	std::string m_stopped_reason;
	bool m_send_stop_event;
	u32 m_last_console_lines;
};

#endif // MAME_OSD_DEBUGGER_DEBUGREMOTE_TCP_H
