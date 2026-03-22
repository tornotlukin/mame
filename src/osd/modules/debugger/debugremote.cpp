// license:BSD-3-Clause
//============================================================
//
//  debugremote.cpp - Remote JSON debugger for LLM integration
//
//  Provides a TCP server that accepts JSON-line protocol
//  messages for debugger control. Designed to be driven by
//  an external LLM bridge process.
//
//============================================================

#include "emu.h"
#include "debug_module.h"

#include "debug/debugbuf.h"
#include "debug/debugcon.h"
#include "debug/debugcpu.h"
#include "debug/points.h"
#include "debug/textbuf.h"
#include "debugger.h"
#include "distate.h"

#include "modules/lib/osdobj_common.h"
#include "modules/osdmodule.h"

#include "fileio.h"

#include <cinttypes>
#include <cstring>
#include <string>
#include <string_view>


namespace osd {

namespace {

//-------------------------------------------------------------------------
// Simple JSON helpers — minimal, no external dependency
//-------------------------------------------------------------------------

// Escape a string for JSON output
static std::string json_escape(std::string_view str)
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

// Build a JSON string value: "key": "value"
static std::string json_kv(const char *key, std::string_view value)
{
	return string_format("\"%s\":\"%s\"", key, json_escape(value));
}

// Build a JSON integer value: "key": 123
static std::string json_kv_int(const char *key, int64_t value)
{
	return string_format("\"%s\":%" PRId64, key, value);
}

// Build a JSON hex value: "key": "0x1234"
static std::string json_kv_hex(const char *key, uint64_t value)
{
	return string_format("\"%s\":\"0x%" PRIx64 "\"", key, value);
}

// Build a JSON bool value: "key": true
static std::string json_kv_bool(const char *key, bool value)
{
	return string_format("\"%s\":%s", key, value ? "true" : "false");
}

// Extract a string value from a JSON-like line (very simple parser)
// Finds "key":"value" or "key": "value"
static bool json_get_string(const std::string &json, const char *key, std::string &out)
{
	std::string search = string_format("\"%s\"", key);
	size_t pos = json.find(search);
	if (pos == std::string::npos)
		return false;
	pos += search.length();
	// skip colon and whitespace
	while (pos < json.length() && (json[pos] == ':' || json[pos] == ' '))
		pos++;
	if (pos >= json.length() || json[pos] != '"')
		return false;
	pos++; // skip opening quote
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

// Extract an integer value from JSON
static bool json_get_int(const std::string &json, const char *key, int64_t &out)
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
	// handle string-encoded numbers like "0x1234"
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
	// plain number
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


//-------------------------------------------------------------------------
// debug_remote - TCP JSON debugger module
//-------------------------------------------------------------------------

class debug_remote : public osd_module, public debug_module
{
public:
	debug_remote() :
		osd_module(OSD_DEBUG_PROVIDER, "remote"), debug_module(),
		m_machine(nullptr),
		m_maincpu(nullptr),
		m_state(nullptr),
		m_memory(nullptr),
		m_address_space(nullptr),
		m_debugger_cpu(nullptr),
		m_debugger_console(nullptr),
		m_debugger_host("localhost"),
		m_debugger_port(12345),
		m_socket(OPEN_FLAG_WRITE | OPEN_FLAG_CREATE),
		m_initialized(false),
		m_readbuf_len(0),
		m_readbuf_offset(0),
		m_stopped_reason("initial"),
		m_send_stop_event(false)
	{
	}

	virtual ~debug_remote() { }

	virtual int init(osd_interface &osd, const osd_options &options) override;
	virtual void exit() override;

	virtual void init_debugger(running_machine &machine) override;
	virtual void wait_for_debugger(device_t &device, bool firststop) override;
	virtual void debugger_update() override;

private:
	// Socket I/O
	int readchar();
	bool read_line(std::string &line);
	void send_line(const std::string &line);

	// Command handling
	void handle_request(const std::string &line);
	void handle_command(const std::string &json, int64_t id);
	void handle_read_memory(const std::string &json, int64_t id);
	void handle_write_memory(const std::string &json, int64_t id);
	void handle_get_registers(const std::string &json, int64_t id);
	void handle_disassemble(const std::string &json, int64_t id);
	void handle_evaluate(const std::string &json, int64_t id);
	void handle_get_state(const std::string &json, int64_t id);
	void handle_get_breakpoints(const std::string &json, int64_t id);
	void handle_break(const std::string &json, int64_t id);

	// Get the address space for the visible CPU, resolving space name
	address_space &get_space_for_request(const std::string &json);

	// Response helpers
	void send_result(int64_t id, const std::string &output);
	void send_error(int64_t id, const std::string &error);
	void send_event(const char *event_type, const std::string &extra_fields);

	// Console output capture
	std::string capture_console_output();

	// Members
	running_machine *m_machine;
	device_t *m_maincpu;
	device_state_interface *m_state;
	device_memory_interface *m_memory;
	address_space *m_address_space;
	debugger_cpu *m_debugger_cpu;
	debugger_console *m_debugger_console;
	std::string m_debugger_host;
	int m_debugger_port;
	emu_file m_socket;
	bool m_initialized;

	// Read buffer for socket
	uint8_t m_readbuf[4096];
	uint32_t m_readbuf_len;
	uint32_t m_readbuf_offset;

	// Line buffer for accumulating JSON lines
	std::string m_linebuf;

	// Event state
	std::string m_stopped_reason;
	bool m_send_stop_event;
};

//-------------------------------------------------------------------------
int debug_remote::init(osd_interface &osd, const osd_options &options)
{
	m_debugger_host = options.debugger_host();
	if (m_debugger_host.empty())
		m_debugger_host = "localhost";
	m_debugger_port = options.debugger_port();
	if (m_debugger_port == 0)
		m_debugger_port = 12345;
	return 0;
}

//-------------------------------------------------------------------------
void debug_remote::exit()
{
	if (m_socket.is_open())
		m_socket.close();
}

//-------------------------------------------------------------------------
void debug_remote::init_debugger(running_machine &machine)
{
	m_machine = &machine;
}

//-------------------------------------------------------------------------
int debug_remote::readchar()
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

//-------------------------------------------------------------------------
bool debug_remote::read_line(std::string &line)
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

//-------------------------------------------------------------------------
void debug_remote::send_line(const std::string &line)
{
	if (!m_socket.is_open())
		return;
	std::string msg = line + "\n";
	m_socket.write(msg.c_str(), msg.length());
}

//-------------------------------------------------------------------------
void debug_remote::send_result(int64_t id, const std::string &output)
{
	std::string response = string_format("{%s,%s,%s}",
		json_kv_int("id", id),
		json_kv("type", "result"),
		json_kv("output", output));
	send_line(response);
}

//-------------------------------------------------------------------------
void debug_remote::send_error(int64_t id, const std::string &error)
{
	std::string response = string_format("{%s,%s,%s}",
		json_kv_int("id", id),
		json_kv("type", "error"),
		json_kv("error", error));
	send_line(response);
}

//-------------------------------------------------------------------------
void debug_remote::send_event(const char *event_type, const std::string &extra_fields)
{
	std::string response = string_format("{%s,%s",
		json_kv("type", "event"),
		json_kv("event", event_type));
	if (!extra_fields.empty())
		response += "," + extra_fields;
	response += "}";
	send_line(response);
}

//-------------------------------------------------------------------------
std::string debug_remote::capture_console_output()
{
	text_buffer &textbuf = m_debugger_console->get_console_textbuf();

	std::string result;
	auto lines = text_buffer_lines(textbuf);
	for (auto it = lines.begin(); it != lines.end(); ++it)
	{
		if (!result.empty())
			result += "\n";
		result += std::string(*it);
	}

	text_buffer_clear(textbuf);
	return result;
}

//-------------------------------------------------------------------------
void debug_remote::wait_for_debugger(device_t &device, bool firststop)
{
	if (firststop && !m_initialized)
	{
		// Set up CPU access
		m_maincpu = device_interface_enumerator<cpu_device>(m_machine->root_device()).first();
		if (!m_maincpu)
			fatalerror("remote debugger: cannot find any CPUs\n");

		m_maincpu->interface(m_state);
		m_memory = &m_maincpu->memory();
		m_address_space = &m_memory->space(AS_PROGRAM);
		m_debugger_cpu = &m_machine->debugger().cpu();
		m_debugger_console = &m_machine->debugger().console();

		// Open TCP socket
		std::string socket_name = string_format("socket.%s:%d", m_debugger_host, m_debugger_port);
		std::error_condition const filerr = m_socket.open(socket_name);
		if (filerr)
			fatalerror("remote debugger: failed to start listening on %s:%d\n", m_debugger_host, m_debugger_port);
		osd_printf_info("remote debugger: listening on %s:%d\n", m_debugger_host, m_debugger_port);

		// Send welcome event
		send_event("connected", string_format("%s,%s",
			json_kv("cpu", m_maincpu->shortname()),
			json_kv("machine", m_machine->system().name)));

		m_initialized = true;
	}
	else
	{
		// Check if we stopped due to a breakpoint or watchpoint
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

	// Main loop — block while stopped, processing commands from TCP
	while (m_debugger_cpu->is_stopped())
	{
		std::string line;
		if (read_line(line))
		{
			handle_request(line);
		}
		else
		{
			// No data available — sleep briefly to avoid 100% CPU
			osd_sleep(osd_ticks_per_second() / 1000);
		}
	}

	// Execution resumed — we'll want to send a stop event next time we stop
	m_send_stop_event = true;
}

//-------------------------------------------------------------------------
void debug_remote::debugger_update()
{
	// Non-blocking check for incoming commands while emulation runs
	// This allows "break" commands to be received during execution
	std::string line;
	while (read_line(line))
	{
		handle_request(line);
	}
}

//-------------------------------------------------------------------------
void debug_remote::handle_request(const std::string &json)
{
	// Parse request ID
	int64_t id = 0;
	json_get_int(json, "id", id);

	// Parse request type
	std::string type;
	if (!json_get_string(json, "type", type))
	{
		send_error(id, "missing 'type' field");
		return;
	}

	if (type == "command")
		handle_command(json, id);
	else if (type == "read_memory")
		handle_read_memory(json, id);
	else if (type == "write_memory")
		handle_write_memory(json, id);
	else if (type == "get_registers")
		handle_get_registers(json, id);
	else if (type == "disassemble")
		handle_disassemble(json, id);
	else if (type == "evaluate")
		handle_evaluate(json, id);
	else if (type == "get_state")
		handle_get_state(json, id);
	else if (type == "get_breakpoints")
		handle_get_breakpoints(json, id);
	else if (type == "break")
		handle_break(json, id);
	else
		send_error(id, string_format("unknown type: %s", type));
}

//-------------------------------------------------------------------------
// Execute a raw debugger console command
void debug_remote::handle_command(const std::string &json, int64_t id)
{
	std::string cmd;
	if (!json_get_string(json, "cmd", cmd))
	{
		send_error(id, "missing 'cmd' field");
		return;
	}

	// Clear the text buffer before executing so we capture only new output
	text_buffer &textbuf = m_debugger_console->get_console_textbuf();
	text_buffer_clear(textbuf);

	// Execute the command
	CMDERR result = m_debugger_console->execute_command(cmd, false);

	if (result.error_class() != CMDERR::NONE)
	{
		send_error(id, debugger_console::cmderr_to_string(result));
		return;
	}

	// Capture output
	std::string output = capture_console_output();
	send_result(id, output);
}

//-------------------------------------------------------------------------
// Resolve the address space for a request, defaulting to program space of visible CPU
address_space &debug_remote::get_space_for_request(const std::string &json)
{
	// Use visible CPU's memory interface
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

//-------------------------------------------------------------------------
// Read memory from address space (with side effects suppressed)
void debug_remote::handle_read_memory(const std::string &json, int64_t id)
{
	int64_t addr = 0, len = 1;
	if (!json_get_int(json, "addr", addr))
	{
		send_error(id, "missing 'addr' field");
		return;
	}
	json_get_int(json, "len", len);
	if (len < 1) len = 1;
	if (len > 65536) len = 65536;

	address_space &space = get_space_for_request(json);

	// Suppress side effects during debugger memory reads
	auto se = m_machine->disable_side_effects();

	// Read memory bytes
	std::string hexdata;
	hexdata.reserve(len * 2);
	std::string ascii;
	ascii.reserve(len);

	for (int64_t i = 0; i < len; i++)
	{
		uint8_t byte = space.read_byte(addr + i);
		char hex[3];
		snprintf(hex, sizeof(hex), "%02x", byte);
		hexdata += hex;
		ascii += (byte >= 0x20 && byte < 0x7f) ? (char)byte : '.';
	}

	std::string response = string_format("{%s,%s,%s,%s,%s}",
		json_kv_int("id", id),
		json_kv("type", "memory"),
		json_kv_hex("addr", (uint64_t)addr),
		json_kv("data", hexdata),
		json_kv("ascii", ascii));
	send_line(response);
}

//-------------------------------------------------------------------------
// Write memory to address space (side effects enabled — writes should take effect)
void debug_remote::handle_write_memory(const std::string &json, int64_t id)
{
	int64_t addr = 0;
	if (!json_get_int(json, "addr", addr))
	{
		send_error(id, "missing 'addr' field");
		return;
	}

	std::string data;
	if (!json_get_string(json, "data", data))
	{
		send_error(id, "missing 'data' field (hex string)");
		return;
	}

	address_space &space = get_space_for_request(json);

	// Parse hex string and write bytes
	int bytes_written = 0;
	for (size_t i = 0; i + 1 < data.length(); i += 2)
	{
		char hex[3] = { data[i], data[i + 1], 0 };
		uint8_t byte = (uint8_t)strtoul(hex, nullptr, 16);
		space.write_byte(addr + bytes_written, byte);
		bytes_written++;
	}

	send_result(id, string_format("wrote %d bytes at 0x%" PRIx64, bytes_written, (uint64_t)addr));
}

//-------------------------------------------------------------------------
// Get all CPU registers for the visible CPU
void debug_remote::handle_get_registers(const std::string &json, int64_t id)
{
	// Use the visible CPU, not necessarily maincpu
	device_t *visiblecpu = m_debugger_console->get_visible_cpu();
	device_state_interface *state = nullptr;
	if (visiblecpu)
		visiblecpu->interface(state);
	if (!state)
		state = m_state;
	if (!state)
	{
		send_error(id, "no CPU state available");
		return;
	}

	std::string regs;
	for (const auto &entry : state->state_entries())
	{
		// Skip dividers and non-visible entries
		if (entry->divider() || !entry->visible())
			continue;

		if (!regs.empty())
			regs += ",";
		// to_string() returns the formatted current value
		regs += string_format("\"%s\":\"%s\"",
			json_escape(entry->symbol()),
			json_escape(entry->to_string()));
	}

	std::string response = string_format("{%s,%s,%s,\"regs\":{%s}}",
		json_kv_int("id", id),
		json_kv("type", "registers"),
		json_kv("cpu", visiblecpu ? visiblecpu->shortname() : m_maincpu->shortname()),
		regs);
	send_line(response);
}

//-------------------------------------------------------------------------
// Disassemble instructions at address using debug_disasm_buffer
void debug_remote::handle_disassemble(const std::string &json, int64_t id)
{
	int64_t addr = 0, count = 10;
	json_get_int(json, "addr", addr);
	json_get_int(json, "count", count);
	if (count < 1) count = 1;
	if (count > 200) count = 200;

	// Use the visible CPU for disassembly
	device_t *cpu = m_debugger_console->get_visible_cpu();
	if (!cpu)
		cpu = m_maincpu;

	debug_disasm_buffer disasm(*cpu);

	std::string output;
	offs_t pc = (offs_t)addr;
	for (int64_t i = 0; i < count; i++)
	{
		std::string instruction;
		offs_t next_pc;
		offs_t size;
		u32 info;
		disasm.disassemble(pc, instruction, next_pc, size, info);

		std::string pc_str = disasm.pc_to_string(pc);
		std::string data_str = disasm.data_to_string(pc, size, true);

		if (!output.empty())
			output += "\n";
		output += string_format("%s: %-12s %s", pc_str, data_str, instruction);

		pc = next_pc;
	}

	send_result(id, output);
}

//-------------------------------------------------------------------------
// Evaluate a debugger expression
void debug_remote::handle_evaluate(const std::string &json, int64_t id)
{
	std::string expr;
	if (!json_get_string(json, "expr", expr))
	{
		send_error(id, "missing 'expr' field");
		return;
	}

	// Use the print command to evaluate
	text_buffer &textbuf = m_debugger_console->get_console_textbuf();
	text_buffer_clear(textbuf);

	std::string cmd = string_format("print %s", expr);
	CMDERR result = m_debugger_console->execute_command(cmd, false);

	if (result.error_class() != CMDERR::NONE)
	{
		send_error(id, string_format("expression error: %s", expr));
		return;
	}

	std::string output = capture_console_output();

	std::string response = string_format("{%s,%s,%s,%s}",
		json_kv_int("id", id),
		json_kv("type", "eval_result"),
		json_kv("expr", expr),
		json_kv("result", output));
	send_line(response);
}

//-------------------------------------------------------------------------
// Get overall debugger/execution state
void debug_remote::handle_get_state(const std::string &json, int64_t id)
{
	bool stopped = m_debugger_cpu->is_stopped();

	// Use visible CPU for state reporting
	device_t *visiblecpu = m_debugger_console->get_visible_cpu();
	device_state_interface *state = nullptr;
	if (visiblecpu)
		visiblecpu->interface(state);
	if (!state)
		state = m_state;

	uint64_t pc = state ? state->state_int(STATE_GENPC) : 0;
	const char *cpuname = visiblecpu ? visiblecpu->shortname() : m_maincpu->shortname();

	std::string response = string_format("{%s,%s,%s,%s,%s,%s}",
		json_kv_int("id", id),
		json_kv("type", "state"),
		json_kv_bool("stopped", stopped),
		json_kv_hex("pc", pc),
		json_kv("cpu", cpuname),
		json_kv("machine", m_machine->system().name));
	send_line(response);
}

//-------------------------------------------------------------------------
// Get all breakpoints
void debug_remote::handle_get_breakpoints(const std::string &json, int64_t id)
{
	// Use the bplist command and capture output
	text_buffer &textbuf = m_debugger_console->get_console_textbuf();
	text_buffer_clear(textbuf);

	m_debugger_console->execute_command("bplist", false);

	std::string output = capture_console_output();

	send_result(id, output);
}


//-------------------------------------------------------------------------
// Break into debugger (halt execution) — can be sent while running
void debug_remote::handle_break(const std::string &json, int64_t id)
{
	m_machine->debugger().debug_break();
	m_stopped_reason = "break";
	send_result(id, "execution halted");
}


} // anonymous namespace

} // namespace osd

MODULE_DEFINITION(DEBUG_REMOTE, osd::debug_remote)
