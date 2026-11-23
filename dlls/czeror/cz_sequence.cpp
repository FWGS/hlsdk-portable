#include "cz_sequence.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

namespace czseq
{

std::string ToLower(const std::string &s)
{
	std::string r = s;
	for (size_t i = 0; i < r.size(); i++)
		r[i] = (char)tolower((unsigned char)r[i]);
	return r;
}

static std::string Trim(const std::string &s)
{
	size_t b = 0, e = s.size();
	while (b < e && isspace((unsigned char)s[b])) b++;
	while (e > b && isspace((unsigned char)s[e - 1])) e--;
	return s.substr(b, e - b);
}

static std::string Unquote(const std::string &s)
{
	std::string t = Trim(s);
	if (t.size() >= 2 && t[0] == '"' && t[t.size() - 1] == '"')
		return t.substr(1, t.size() - 2);
	return t;
}

// Remove "//" comments that are outside of quotes.
static std::string StripComment(const std::string &line, bool &inQuote)
{
	std::string out;
	for (size_t i = 0; i < line.size(); i++)
	{
		char c = line[i];
		if (c == '"')
			inQuote = !inQuote;
		if (!inQuote && c == '/' && i + 1 < line.size() && line[i + 1] == '/')
			break;
		out += c;
	}
	return out;
}

static std::string StripTrailingDigits(const std::string &s)
{
	size_t e = s.size();
	while (e > 0 && isdigit((unsigned char)s[e - 1])) e--;
	return s.substr(0, e);
}

// split "k = v, k2 = v2" (commas inside quotes are kept)
static void SplitPairs(const std::string &s, std::vector<std::pair<std::string, std::string> > &out)
{
	std::vector<std::string> parts;
	std::string cur;
	bool q = false;
	for (size_t i = 0; i < s.size(); i++)
	{
		char c = s[i];
		if (c == '"') q = !q;
		if (c == ',' && !q)
		{
			parts.push_back(cur);
			cur.clear();
		}
		else
			cur += c;
	}
	parts.push_back(cur);

	for (size_t i = 0; i < parts.size(); i++)
	{
		std::string p = parts[i];
		size_t eq = p.find('=');
		std::string k, v;
		if (eq == std::string::npos)
		{
			k = Trim(p);
		}
		else
		{
			k = Trim(p.substr(0, eq));
			v = Unquote(p.substr(eq + 1));
		}
		if (!k.empty())
			out.push_back(std::make_pair(k, v));
	}
}

void Library::Clear()
{
	m_raw.clear();
	m_built.clear();
	m_groups.clear();
	m_files.clear();
}

bool Library::IsFileLoaded(const std::string &fileName) const
{
	std::string f = ToLower(fileName);
	for (size_t i = 0; i < m_files.size(); i++)
		if (m_files[i] == f)
			return true;
	return false;
}

int Library::ParseText(const char *text, size_t length, const std::string &fileName)
{
	std::string file = ToLower(fileName);
	if (!IsFileLoaded(file))
		m_files.push_back(file);

	enum { TOP, WANT_BRACE_ENTRY, WANT_BRACE_SENT, IN_ENTRY, IN_SENT } state = TOP;
	int count = 0;

	RawEntry cur;
	bool inQuote = false;
	std::string pending; // line being assembled (for quotes spanning lines)

	size_t pos = 0;
	while (pos <= length)
	{
		size_t eol = pos;
		while (eol < length && text[eol] != '\n') eol++;
		std::string rawLine(text + pos, eol - pos);
		pos = eol + 1;

		if (!rawLine.empty() && rawLine[rawLine.size() - 1] == '\r')
			rawLine.erase(rawLine.size() - 1);

		bool wasInQuote = inQuote;
		std::string line = StripComment(rawLine, inQuote);

		if (wasInQuote || !pending.empty())
		{
			pending += "\n" + line;
			if (inQuote)
				continue; // still inside a multi-line string
			line = pending;
			pending.clear();
		}
		else if (inQuote)
		{
			pending = line;
			continue;
		}

		line = Trim(line);
		if (line.empty())
			continue;

		// Handle "{" or "}" possibly followed by more text.
		if (state == WANT_BRACE_ENTRY || state == WANT_BRACE_SENT)
		{
			if (line[0] == '{')
			{
				state = (state == WANT_BRACE_ENTRY) ? IN_ENTRY : IN_SENT;
				line = Trim(line.substr(1));
				if (line.empty())
					continue;
			}
			else
			{
				// malformed, drop back
				state = TOP;
			}
		}

		if (state == TOP)
		{
			if (line[0] == '%')
			{
				cur = RawEntry();
				cur.fileName = file;
				size_t e = 1;
				while (e < line.size() && !isspace((unsigned char)line[e]) && line[e] != '{') e++;
				cur.name = line.substr(1, e - 1);
				std::string rest = Trim(line.substr(e));
				state = WANT_BRACE_ENTRY;
				if (!rest.empty() && rest[0] == '{')
				{
					state = IN_ENTRY;
				}
			}
			else if (line[0] == '!')
			{
				state = WANT_BRACE_SENT; // block name is ignored, only "!sentences" exists
				if (line.find('{') != std::string::npos)
					state = IN_SENT;
			}
			continue;
		}

		if (line[0] == '}')
		{
			if (state == IN_ENTRY)
			{
				m_raw.push_back(cur);
				count++;
			}
			state = TOP;
			continue;
		}

		if (state == IN_SENT)
		{
			size_t sp = 0;
			while (sp < line.size() && !isspace((unsigned char)line[sp])) sp++;
			std::string name = line.substr(0, sp);
			std::string path = Trim(line.substr(sp));
			if (!name.empty() && !path.empty())
				m_groups[ToLower(StripTrailingDigits(name))].push_back(path);
			continue;
		}

		// IN_ENTRY
		Op op;
		op.type = line[0];
		if (op.type == '@')
		{
			// "@Style  #text = ..." : call and command on the same line
			std::string rest = Trim(line.substr(1));
			size_t sp = 0;
			while (sp < rest.size() && !isspace((unsigned char)rest[sp])) sp++;
			op.key = rest.substr(0, sp);
			cur.ops.push_back(op);
			std::string tail = Trim(rest.substr(sp));
			if (!tail.empty() && (tail[0] == '#' || tail[0] == '$'))
			{
				Op op2;
				op2.type = tail[0];
				std::vector<std::pair<std::string, std::string> > pairs2;
				SplitPairs(tail.substr(1), pairs2);
				if (!pairs2.empty())
				{
					op2.key = ToLower(pairs2[0].first);
					for (size_t i = 0; i < pairs2.size(); i++)
					{
						Pair p;
						p.key = ToLower(pairs2[i].first);
						p.value = pairs2[i].second;
						op2.args.push_back(p);
					}
					cur.ops.push_back(op2);
				}
			}
		}
		else if (op.type != '$' && op.type != '#' && !cur.ops.empty() && (cur.ops.back().type == '#' || cur.ops.back().type == '$'))
		{
			// continuation of the previous command after a trailing comma:
			//	#sound = deltaDialogFirst,
			//		speaker = delta,
			//		pause = 4.0
			std::vector<std::pair<std::string, std::string> > pairs;
			SplitPairs(line, pairs);
			for (size_t i = 0; i < pairs.size(); i++)
			{
				Pair p;
				p.key = ToLower(pairs[i].first);
				p.value = pairs[i].second;
				cur.ops.back().args.push_back(p);
			}
		}
		else if (op.type == '$' || op.type == '#')
		{
			std::string body = line.substr(1);
			std::vector<std::pair<std::string, std::string> > pairs;
			SplitPairs(body, pairs);
			if (pairs.empty())
				continue;
			op.key = ToLower(pairs[0].first);
			for (size_t i = 0; i < pairs.size(); i++)
			{
				Pair p;
				p.key = ToLower(pairs[i].first);
				p.value = pairs[i].second;
				op.args.push_back(p);
			}
			cur.ops.push_back(op);
		}
	}

	m_built.clear();
	return count;
}

static void ParseFloats(const std::string &v, float *out, int n)
{
	const char *p = v.c_str();
	for (int i = 0; i < n; i++)
	{
		char *end;
		double d = strtod(p, &end);
		if (end == p)
			break;
		out[i] = (float)d;
		p = end;
	}
}

static void ParseColor(const std::string &v, unsigned char *out)
{
	float f[4] = { (float)out[0], (float)out[1], (float)out[2], (float)out[3] };
	ParseFloats(v, f, 4);
	for (int i = 0; i < 4; i++)
		out[i] = (unsigned char)(f[i] < 0 ? 0 : (f[i] > 255 ? 255 : f[i]));
}

void Library::ApplyModifier(Modifiers &m, const std::string &key, const std::string &value)
{
	if (key == "speaker") m.speaker = value;
	else if (key == "listener") m.listener = value;
	else if (key == "position")
	{
		float f[2] = { m.x, m.y };
		ParseFloats(value, f, 2);
		m.x = f[0]; m.y = f[1];
	}
	else if (key == "color") ParseColor(value, m.c1);
	else if (key == "color2") ParseColor(value, m.c2);
	else if (key == "effect") m.effect = atoi(value.c_str());
	else if (key == "fadein") m.fadein = (float)atof(value.c_str());
	else if (key == "fadeout") m.fadeout = (float)atof(value.c_str());
	else if (key == "holdtime") m.holdtime = (float)atof(value.c_str());
	else if (key == "fxtime") m.fxtime = (float)atof(value.c_str());
	else if (key == "channel") m.channel = atoi(value.c_str());
}

void Library::Emit(const RawEntry &raw, Modifiers &state, std::vector<Command> &out, int depth)
{
	for (size_t i = 0; i < raw.ops.size(); i++)
	{
		const Op &op = raw.ops[i];

		if (op.type == '@')
		{
			if (depth >= 8)
				continue;
			std::string want = ToLower(op.key);
			for (size_t j = 0; j < m_raw.size(); j++)
			{
				// prefer same file, fall back to any file
				if (ToLower(m_raw[j].name) == want && m_raw[j].fileName == raw.fileName)
				{
					Emit(m_raw[j], state, out, depth + 1);
					want.clear();
					break;
				}
			}
			if (!want.empty())
			{
				for (size_t j = 0; j < m_raw.size(); j++)
				{
					if (ToLower(m_raw[j].name) == want)
					{
						Emit(m_raw[j], state, out, depth + 1);
						break;
					}
				}
			}
			continue;
		}

		if (op.type == '$')
		{
			for (size_t a = 0; a < op.args.size(); a++)
				ApplyModifier(state, op.args[a].key, op.args[a].value);
			continue;
		}

		// '#' command. One command line of the engine holds several fields at once and runs them
		// together (CTriggerSequence::ExecuteSequenceCommand: fire, kill, text, sound), e.g.
		//	#text = "", sound = dialogSworeIHeard, speaker = tStart, pause = 4
		//	@ObjectiveStyle #text = "#OBJ1_LCAUSE_NEW", firetargets = "new_objective01_complete"
		// The other fields of the line were taken for modifiers and lost.
		Modifiers mod = state;
		float inlinePause = 0.0f;
		bool hasPause = false;
		for (size_t a = 0; a < op.args.size(); a++)
		{
			const std::string &k = op.args[a].key;
			if (k == "pause")
			{
				inlinePause = (float)atof(op.args[a].value.c_str());
				hasPause = true;
			}
			else if (a > 0 && k == "holdtime")
				mod.holdtime = (float)atof(op.args[a].value.c_str());
			else if (a > 0)
				ApplyModifier(mod, k, op.args[a].value);
		}

		if (op.key == "holdtime")
		{
			state.holdtime = (float)atof(op.args.empty() ? "0" : op.args[0].value.c_str());
			continue;
		}

		if (op.key == "pause" && op.args.size() <= 1)
		{
			Command c;
			c.mod = mod;
			c.kind = CMD_PAUSE;
			c.delay = inlinePause;
			out.push_back(c);
			continue;
		}

		static const char *order[] = { "firetargets", "killtargets", "text", "sound", "repeat" };
		size_t first = out.size();

		for (size_t o = 0; o < sizeof(order) / sizeof(order[0]); o++)
		{
			for (size_t a = 0; a < op.args.size(); a++)
			{
				if (op.args[a].key != order[o])
					continue;

				const std::string &val = op.args[a].value;
				if (o == 2 && val.empty())
					continue; // #text = "" only carries the line, nothing to show

				Command c;
				c.mod = mod;
				if (o == 0) { c.kind = CMD_FIRE; c.value = val; }
				else if (o == 1) { c.kind = CMD_KILL; c.value = val; }
				else if (o == 2) { c.kind = CMD_TEXT; c.value = val; }
				else if (o == 3) { c.kind = CMD_SOUND; c.value = val; }
				else { c.kind = CMD_REPEAT; c.repeat = atoi(val.c_str()); }
				out.push_back(c);
			}
		}

		// the pause of the line is the wait after all of it
		if (hasPause)
		{
			if (out.size() > first)
				out.back().delay = inlinePause;
			else
			{
				Command c;
				c.mod = mod;
				c.kind = CMD_PAUSE;
				c.delay = inlinePause;
				out.push_back(c);
			}
		}
	}
}

const Entry *Library::Find(const std::string &fileName, const std::string &entryName)
{
	std::string f = ToLower(fileName);
	std::string n = ToLower(entryName);
	std::string key = f + "|" + n;

	std::map<std::string, Entry>::iterator it = m_built.find(key);
	if (it != m_built.end())
		return &it->second;

	const RawEntry *found = NULL;
	for (size_t i = 0; i < m_raw.size(); i++)
	{
		if (ToLower(m_raw[i].name) != n)
			continue;
		if (!f.empty() && m_raw[i].fileName != f)
		{
			if (!found) found = NULL; // keep looking for exact file
			continue;
		}
		found = &m_raw[i];
		break;
	}
	if (!found && !f.empty())
	{
		// fall back to any file
		for (size_t i = 0; i < m_raw.size(); i++)
			if (ToLower(m_raw[i].name) == n) { found = &m_raw[i]; break; }
	}
	if (!found)
		return NULL;

	Entry e;
	e.fileName = found->fileName;
	e.name = found->name;
	Modifiers state;
	Emit(*found, state, e.commands, 0);
	return &(m_built[key] = e);
}

const std::vector<std::string> *Library::GetGroup(const std::string &groupName) const
{
	std::map<std::string, std::vector<std::string> >::const_iterator it = m_groups.find(ToLower(groupName));
	if (it == m_groups.end() || it->second.empty())
		return NULL;
	return &it->second;
}

const std::string *Library::PickSentence(const std::string &groupName, int *picked) const
{
	const std::vector<std::string> *g = GetGroup(groupName);
	if (!g)
		return NULL;
	int idx = (int)(rand() % g->size());
	if (picked)
		*picked = idx;
	return &(*g)[idx];
}

} // namespace czseq
