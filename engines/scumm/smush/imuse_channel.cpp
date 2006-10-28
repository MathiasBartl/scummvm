/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2006 The ScummVM project
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

#include "common/stdafx.h"
#include "common/endian.h"

#include "scumm/scumm.h"	// For DEBUG_SMUSH
#include "scumm/util.h"
#include "scumm/smush/channel.h"
#include "scumm/smush/chunk.h"

namespace Scumm {

ImuseChannel::ImuseChannel(int32 track) : SmushChannel(track) {
}

bool ImuseChannel::isTerminated() const {
	return (_dataSize <= 0 && _sbuffer == 0);
}

bool ImuseChannel::setParameters(int32 nb, int32 size, int32 flags, int32 unk1, int32) {
	if ((flags == 1) || (flags == 2) || (flags == 3)) {
		_volume = 127;
	} else if ((flags >= 100) && (flags <= 163)) {
		_volume = flags * 2 - 200;
	} else if ((flags >= 200) && (flags <= 263)) {
		_volume = flags * 2 - 400;
	} else if ((flags >= 300) && (flags <= 363)) {
		_volume = flags * 2 - 600;
	} else {
		error("ImuseChannel::setParameters(): bad flags: %d", flags);
	}
	_pan = 0;
	return true;
}

bool ImuseChannel::checkParameters(int32 index, int32 nbframes, int32 size, int32 track_flags, int32 unk1) {
	return true;
}

bool ImuseChannel::appendData(Chunk &b, int32 size) {
	if (_dataSize == -1) {
		assert(size > 8);
		Chunk::type imus_type = b.readUint32LE(); imus_type = SWAP_BYTES_32(imus_type);
		uint32 imus_size = b.readUint32LE(); imus_size = SWAP_BYTES_32(imus_size);
		if (imus_type != MKID_BE('iMUS'))
			error("Invalid Chunk for imuse_channel");
		size -= 8;
		_tbufferSize = size;
		assert(_tbufferSize);
		_tbuffer = new byte[_tbufferSize];
		if (!_tbuffer)
			error("imuse_channel failed to allocate memory");
		b.read(_tbuffer, size);
		_dataSize = -2;
	} else {
		if (_tbuffer) {
			byte *old = _tbuffer;
			int32 new_size = size + _tbufferSize;
			_tbuffer = new byte[new_size];
			if (!_tbuffer)
				error("imuse_channel failed to allocate memory");
			memcpy(_tbuffer, old, _tbufferSize);
			delete []old;
			b.read(_tbuffer + _tbufferSize, size);
			_tbufferSize += size;
		} else {
			_tbufferSize = size;
			_tbuffer = new byte[_tbufferSize];
			if (!_tbuffer)
				error("imuse_channel failed to allocate memory");
			b.read(_tbuffer, size);
		}
	}

	processBuffer();

	_srbufferSize = _sbufferSize;
	if (_sbuffer && _bitsize == 12)
		decode();

	return true;
}

bool ImuseChannel::handleFormat(Chunk &src) {
	if (src.size() != 20) error("invalid size for FRMT Chunk");
	uint32 imuse_start = src.readUint32LE();
	imuse_start = SWAP_BYTES_32(imuse_start);
	src.seek(4, SEEK_CUR);
	_bitsize = src.readUint32LE();
	_bitsize = SWAP_BYTES_32(_bitsize);
	_rate = src.readUint32LE();
	_rate = SWAP_BYTES_32(_rate);
	_channels = src.readUint32LE();
	_channels = SWAP_BYTES_32(_channels);
	assert(_channels == 1 || _channels == 2);
	return true;
}

bool ImuseChannel::handleRegion(Chunk &src) {
	if (src.size() != 8)
		error("invalid size for REGN Chunk");
	return true;
}

bool ImuseChannel::handleStop(Chunk &src) {
	if (src.size() != 4)
		error("invalid size for STOP Chunk");
	return true;
}

bool ImuseChannel::handleMap(Chunk &map) {
	while (!map.eos()) {
		Chunk *sub = map.subBlock();
		switch (sub->getType()) {
		case MKID_BE('FRMT'):
			handleFormat(*sub);
			break;
		case MKID_BE('TEXT'):
			break;
		case MKID_BE('REGN'):
			handleRegion(*sub);
			break;
		case MKID_BE('STOP'):
			handleStop(*sub);
			break;
		default:
			error("Unknown iMUS subChunk found : %s, %d", tag2str(sub->getType()), sub->size());
		}
		delete sub;
	}
	return true;
}

void ImuseChannel::decode() {
	int remaining_size = _sbufferSize % 3;
	if (remaining_size) {
		_srbufferSize -= remaining_size;
		assert(_inData);
		if (_tbuffer == 0) {
			_tbuffer = new byte[remaining_size];
			memcpy(_tbuffer, _sbuffer + _sbufferSize - remaining_size, remaining_size);
			_tbufferSize = remaining_size;
			_sbufferSize -= remaining_size;
		} else {
			debugC(DEBUG_SMUSH, "impossible ! : %p, %d, %d, %p(%d), %p(%d, %d)",
				this, _dataSize, _inData, _tbuffer, _tbufferSize, _sbuffer, _sbufferSize, _srbufferSize);
			byte *old = _tbuffer;
			int new_size = remaining_size + _tbufferSize;
			_tbuffer = new byte[new_size];
			if (!_tbuffer)  error("imuse_channel failed to allocate memory");
			memcpy(_tbuffer, old, _tbufferSize);
			delete []old;
			memcpy(_tbuffer + _tbufferSize, _sbuffer + _sbufferSize - remaining_size, remaining_size);
			_tbufferSize += remaining_size;
		}
	}

	// FIXME: Code duplication! See decode12BitsSample() in imuse_digi/dimuse_codecs.cpp

	int loop_size = _sbufferSize / 3;
	int new_size = loop_size * 4;
	byte *keep, *decoded;
	uint32 value;
	keep = decoded = new byte[new_size];
	assert(keep);
	unsigned char * source = _sbuffer;

	while (loop_size--) {
		byte v1 =  *source++;
		byte v2 =  *source++;
		byte v3 =  *source++;
		value = ((((v2 & 0x0f) << 8) | v1) << 4) - 0x8000;
		WRITE_BE_UINT16(decoded, value); decoded += 2;
		value = ((((v2 & 0xf0) << 4) | v3) << 4) - 0x8000;
		WRITE_BE_UINT16(decoded, value); decoded += 2;
	}
	delete []_sbuffer;
	_sbuffer = (byte *)keep;
	_sbufferSize = new_size;
}

bool ImuseChannel::handleSubTags(int32 &offset) {
	if (_tbufferSize - offset >= 8) {
		Chunk::type type = READ_BE_UINT32(_tbuffer + offset);
		uint32 size = READ_BE_UINT32(_tbuffer + offset + 4);
		uint32 available_size = _tbufferSize - offset;
		switch (type) {
		case MKID_BE('MAP '):
			_inData = false;
			if (available_size >= (size + 8)) {
				MemoryChunk c((byte *)_tbuffer + offset);
				handleMap(c);
			}
			break;
		case MKID_BE('DATA'):
			_inData = true;
			_dataSize = size;
			offset += 8;
			{
				int reqsize = 1;
				if (_channels == 2)
					reqsize *= 2;
				if (_bitsize == 16)
					reqsize *= 2;
				else if (_bitsize == 12) {
					if (reqsize > 1)
						reqsize = reqsize * 3 / 2;
					else reqsize = 3;
				}
				if ((size % reqsize) != 0) {
					debugC(DEBUG_SMUSH, "Invalid iMUS sound data size : (%d %% %d) != 0, correcting...", size, reqsize);
					size += 3 - (size % reqsize);
				}
			}
			return false;
		default:
			error("unknown Chunk in iMUS track : %s ", tag2str(type));
		}
		offset += size + 8;
		return true;
	}
	return false;
}

byte *ImuseChannel::getSoundData() {
	byte *tmp = _sbuffer;

	assert(_dataSize > 0);
	_dataSize -= _srbufferSize;

	_sbuffer = 0;
	_sbufferSize = 0;
	
	return tmp;
}

} // End of namespace Scumm
