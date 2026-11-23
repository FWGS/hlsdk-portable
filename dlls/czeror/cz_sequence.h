#pragma once

// Parser for Condition Zero Deleted Scenes ".seq" sequence scripts.
// Pure C++ (no game headers) so it can be unit-tested on the host.

#include <string>
#include <vector>
#include <map>

namespace czseq
{

struct Modifiers
{
	std::string speaker;
	std::string listener;
	float x = -1.0f, y = -1.0f;
	int effect = 0;
	unsigned char c1[4] = { 255, 255, 255, 255 };
	unsigned char c2[4] = { 255, 255, 255, 255 };
	float fadein = 0.0f, fadeout = 0.0f, holdtime = 3.0f, fxtime = 0.0f;
	int channel = 1;
};

enum CommandKind
{
	CMD_PAUSE = 0,
	CMD_FIRE,
	CMD_KILL,
	CMD_TEXT,
	CMD_SOUND,
	CMD_REPEAT,
};

struct Command
{
	CommandKind kind = CMD_PAUSE;
	std::string value;     // targets / text / sound name
	float delay = 0.0f;    // seconds to wait before this command
	int repeat = 0;        // for CMD_REPEAT
	Modifiers mod;         // snapshot of modifiers valid for this command
};

struct Entry
{
	std::string fileName;
	std::string name;
	std::vector<Command> commands;
};

class Library
{
public:
	void Clear();

	// Parse the text of one .seq file. fileName is without extension.
	// Returns number of entries parsed.
	int ParseText(const char *text, size_t length, const std::string &fileName);

	bool IsFileLoaded(const std::string &fileName) const;

	// Find a fully resolved entry. fileName may be empty (search everywhere).
	const Entry *Find(const std::string &fileName, const std::string &entryName);

	// Pick a sentence (path to sound without extension) from a group, nullptr if none.
	const std::string *PickSentence(const std::string &groupName, int *picked = NULL) const;
	const std::vector<std::string> *GetGroup(const std::string &groupName) const;

private:
	struct Pair { std::string key, value; };
	struct Op
	{
		char type;                 // '@' gosub, '$' modifier, '#' command
		std::string key;           // gosub name / modifier name / command name
		std::vector<Pair> args;    // first = main value, rest = inline modifiers
	};
	struct RawEntry
	{
		std::string fileName;
		std::string name;
		std::vector<Op> ops;
	};

	void Emit(const RawEntry &raw, Modifiers &state, std::vector<Command> &out, int depth);
	static void ApplyModifier(Modifiers &m, const std::string &key, const std::string &value);

	std::vector<RawEntry> m_raw;
	std::map<std::string, Entry> m_built;       // key: lower(file)+"|"+lower(name)
	std::map<std::string, std::vector<std::string> > m_groups; // lower(group) -> paths
	std::vector<std::string> m_files;           // lower-case loaded file names
};

std::string ToLower(const std::string &s);

} // namespace czseq
