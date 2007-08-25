/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#ifndef PARALLACTION_ZONE_H
#define PARALLACTION_ZONE_H

#include "common/list.h"

#include "parallaction/defs.h"

#include "parallaction/graphics.h"


namespace Parallaction {

struct Zone;
struct Animation;
struct Command;
struct Question;
struct Answer;
struct Instruction;
struct Program;

enum ZoneTypes {
	kZoneExamine	   = 1, 				// zone displays comment if activated
	kZoneDoor		   = 2, 				// zone activated on click (after some walk if needed)
	kZoneGet		   = 4, 				// for pickable items
	kZoneMerge		   = 8, 				// tags items which can be merged in inventory
	kZoneTaste		   = 0x10,				// NEVER USED
	kZoneHear		   = 0x20,				// NEVER USED: they ran out of time before integrating sfx
	kZoneFeel		   = 0x40,				// NEVER USED
	kZoneSpeak		   = 0x80,				// tags NPCs the character can talk with
	kZoneNone		   = 0x100, 			// used to prevent parsing on peculiar Animations
	kZoneTrap		   = 0x200, 			// zone activated when character enters
	kZoneYou		   = 0x400, 			// marks the character
	kZoneCommand	   = 0x800
};


enum ZoneFlags {
	kFlagsClosed		= 1,				// Zone: door is closed / switch is off
	kFlagsActive		= 2,				// Zone/Animation: object is visible
	kFlagsRemove		= 4,				// Zone/Animation: object is soon to be removed
	kFlagsActing		= 8,				// Animation: script execution is active
	kFlagsLocked		= 0x10, 			// Zone: door or switch cannot be toggled
	kFlagsFixed 		= 0x20, 			// Zone: Zone item cannot be picked up
	kFlagsNoName		= 0x40, 			// Zone with no name (used to prevent some kEvEnterZone events)
	kFlagsNoMasked		= 0x80, 			// Animation is to be drawn ignoring z buffer
	kFlagsLooping		= 0x100,			// Animation: script is to be executed repeatedly
	kFlagsAdded 		= 0x200,			// NEVER USED in Nippon Safes
	kFlagsCharacter 	= 0x400,			//
	kFlagsNoWalk		= 0x800 			// Zone: character doesn't need to walk towards object to interact
};


enum CommandFlags {
	kFlagsVisited		= 1,
	kFlagsExit			= 0x10000000,
	kFlagsEnter 		= 0x20000000,
	kFlagsGlobal		= 0x40000000,

	// BRA specific
	kFlagsTestTrue		= 2
};

struct CommandData {
	uint32			_flags;
	Animation * 	_animation;
	Zone*			_zone;
	char*			_string;
	uint16			_callable;
	uint16			_object;
	Common::Point	 _move;

	// BRA specific
	Common::Point	_startPos;
	Common::Point	_startPos2;
	uint			_lvalue;
	int				_rvalue;
	int				_zeta0;
	int				_zeta1;
	int				_zeta2;
	int				_characterId;
	char*			_string2;
	int				_musicCommand;
	int				_musicParm;


	CommandData() {
		memset(this, 0, sizeof(CommandData));
	}

	~CommandData() {
		if (_string)
			free(_string);
		if (_string2)
			free(_string2);
	}
};

struct Command {
	uint16			_id;
	CommandData 	u;
	uint32			_flagsOn;
	uint32			_flagsOff;

	Command();
	~Command();
};

typedef ManagedList<Command*> CommandList;


#define NUM_QUESTIONS		20
#define NUM_ANSWERS		 	5

struct Answer {
	char*		_text;
	uint16		_mood;
	union {
		Question*	_question;
		char*		_name;
	} _following;
	CommandList	_commands;
	uint32		_noFlags;
	uint32		_yesFlags;

	Answer();
	~Answer();
};

struct Question {
	char*		_text;
	uint16		_mood;
	Answer*		_answers[NUM_ANSWERS];

	Question();
	~Question();
};

struct Dialogue {
	Question	*_questions[NUM_QUESTIONS];
};

struct GetData {	// size = 24
	uint32			_icon;
	Graphics::Surface		*_cnv;
	byte		   *_backup;
	uint16			field_14;		// unused
	uint16			field_16;		// unused

	GetData() {
		_icon = 0;
		_backup = NULL;
		_cnv = NULL;
	}
};
struct SpeakData {	// size = 36
	char		_name[32];
	Dialogue	*_dialogue;

	SpeakData() {
		_name[0] = '\0';
		_dialogue = NULL;
	}
};
struct ExamineData {	// size = 28
	Graphics::Surface	*_cnv;
	uint16		_opBase;		   // unused
	uint16		field_12;			// unused
	char*		_description;
	char*		_filename;

	ExamineData() {
		_opBase = 0;
		_description = NULL;
		_filename = NULL;
		_cnv = NULL;
	}
};
struct DoorData {	// size = 28
	char*	_location;
	Cnv 	*_cnv;
	byte*	_background;
	Common::Point	_startPos;
	uint16	_startFrame;

	DoorData() {
		_location = NULL;
		_background = NULL;
		_startFrame = 0;
		_cnv = NULL;
	}
};
struct HearData {	// size = 20
	char		_name[20];
	int			_channel;
	int			_freq;

	HearData() {
		_channel = -1;
		_freq = -1;
		_name[0] = '\0';
	}
};
struct MergeData {	// size = 12
	uint32	_obj1;
	uint32	_obj2;
	uint32	_obj3;

	MergeData() {
		_obj1 = _obj2 = _obj3 = 0;
	}
};

struct TypeData {
	GetData 	*get;
	SpeakData	*speak;
	ExamineData *examine;
	DoorData	*door;
	HearData	*hear;
	MergeData	*merge;

	TypeData() {
		get = NULL;
		speak = NULL;
		examine = NULL;
		door = NULL;
		hear = NULL;
		merge = NULL;
	}
};

struct Label {
	char*				_text;
	Graphics::Surface	_cnv;

	Label();
	~Label();
};

struct Zone {
	int16 			_left;
	int16			_top;
	int16			_right;
	int16			_bottom;
	uint32			_type;
	uint32			_flags;
	Label       	_label;
	uint16			field_2C;		// unused
	uint16			field_2E;		// unused
	TypeData		u;
	CommandList 	_commands;
	Common::Point	_moveTo;

	Zone();
	virtual ~Zone();

	void getRect(Common::Rect& r) const;
	void translate(int16 x, int16 y);
	virtual uint16 width() const;
	virtual uint16 height() const;
};

typedef Zone* ZonePointer;
typedef ManagedList<ZonePointer> ZoneList;

struct LocalVariable {
	int16		_value;
	int16		_min;
	int16		_max;

	LocalVariable() {
		_value = 0;
		_min = -10000;
		_max = 10000;
	}
};

enum ParaFlags {
	kParaImmediate	= 1,				// instruction is using an immediate parameter
	kParaLocal		= 2,				// instruction is using a local variable
	kParaField		= 0x10,				// instruction is using an animation's field
	kParaRandom		= 0x100
};


struct ScriptVar {
	uint32 			_flags;

	int16			_value;
	int16*			_pvalue;
	LocalVariable*	_local;

	ScriptVar();

	int16	getRValue();
	int16*	getLValue();

	void	setLocal(LocalVariable *local);
	void	setField(int16 *field);
	void 	setImmediate(int16 value);
	void 	setRandom(int16 seed);
};

enum InstructionFlags {
	kInstMod			= 4,
	kInstMaskedPut		= 8,
	kInstUnk20			= 0x20
};

typedef ManagedList<Instruction*> InstructionList;

struct Instruction {
	uint32	_index;
	uint32	_flags;

	// common
	Animation	*_a;
	Zone		*_z;
	int16		_immediate;
	ScriptVar	_opA;
	ScriptVar	_opB;

	// BRA specific
	byte		_colors[3];
	ScriptVar	_opC;
	char		*_text;
	char		*_text2;
	int			_y;
	InstructionList::iterator	_endif;

	Instruction();
	~Instruction();

};


struct Program {
	LocalVariable	*_locals;

	uint16			_loopCounter;

	uint16	_numLocals;

	InstructionList::iterator	_ip;
	InstructionList::iterator	_loopStart;
	InstructionList				_instructions;

	Program();
	~Program();

	int16 		findLocal(const char* name);
	int16 		addLocal(const char *name, int16 value = 0, int16 min = -10000, int16 max = 10000);
};



struct Animation : public Zone  {

	Common::Point	_oldPos;
	Program 	*_program;
	Cnv 		*_cnv;
	char		*_scriptName;
	int16		_frame;
	uint16		field_50;		// unused
	int16		_z;
	uint16		field_54;		// unused
	uint16		field_56;		// unused
	uint16		field_58;		// unused
	uint16		field_5A;		// unused
	uint16		field_5C;		// unused
	uint16		field_5E;		// unused

	Animation();
	virtual ~Animation();
	virtual uint16 width() const;
	virtual uint16 height() const;
	uint16 getFrameNum() const;
	byte* getFrameData(uint32 index) const;
};

typedef Animation* AnimationPointer;
typedef ManagedList<AnimationPointer> AnimationList;


} // namespace Parallaction

#endif
