//
// N64.cpp
//
// Author:
//       Christopher "Zoggins" Mallery <zoggins@retro-spy.com>
//
// Copyright (c) 2020 RetroSpy Technologies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "N64.h"

void N64Spy::loop() {
	noInterrupts();
	updateState();
	interrupts();
	if (checkPrefixN64()) {
#if !defined(DEBUG)
		writeSerial();
#else
		debugSerial();
#endif
	}
	else {
		// This makes no sense, but its needed after command 0x0 or else you get garbage on the line
		A_DELAY(2);
	}
	T_DELAY(5);
}

// Verifies that the 9 bits prefixing N64 controller data in 'rawData'
// are actually indicative of a controller state signal.
inline bool N64Spy::checkPrefixN64() {
	return rawData[0] == 0x01;
}

void N64Spy::updateState() {
	unsigned short bits;

	unsigned char *rawDataPtr = &rawData[1];
	byte /*bit7, bit6, bit5, bit4, bit3, */bit2, bit1, bit0;
	WAIT_FALLING_EDGE(N64_PIN);
	asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
	// bit7 = PIND & 0b00000100;
	WAIT_FALLING_EDGE(N64_PIN);
	asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
	// bit6 = PIND & 0b00000100;
	WAIT_FALLING_EDGE(N64_PIN);
	asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
	// bit5 = PIND & 0b00000100;
	WAIT_FALLING_EDGE(N64_PIN);
	asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
	// bit4 = PIND & 0b00000100;
	WAIT_FALLING_EDGE(N64_PIN);
	asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
	// bit3 = PIND & 0b00000100;
	WAIT_FALLING_EDGE(N64_PIN);
	asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
	bit2 = PIND & 0b00000100;
	if (bit2 != 0)  // Controller Reset
	{
		WAIT_FALLING_EDGE(N64_PIN);
		asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
		// bit1 = PIND & 0b00000100;
		WAIT_FALLING_EDGE(N64_PIN);
		asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
		// bit0 = PIND & 0b00000100;
		bits = 25;
		rawData[0] = 0xFF;
		goto read_loop;
	}
	WAIT_FALLING_EDGE(N64_PIN);
	asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
	bit1 = PIND & 0b00000100;
	if (bit1 != 0) // read or write to memory pack (this doesn't work correctly)
	{
		WAIT_FALLING_EDGE(N64_PIN);
		asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
		// bit0 = PIND & 0b00000100;
		bits = 281;
		rawData[0] = 0x02;
		goto read_loop;
	}
	WAIT_FALLING_EDGE(N64_PIN);
	asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);
	bit0 = PIND & 0b00000100;
	if (bit0 != 0) // controller poll
	{
		bits = 33;
		rawData[0] = 0x01;
		goto read_loop;
	}
	bits = 25;   // Get controller info
	rawData[0] = 0x00;

read_loop:

	// Wait for the line to go high then low.
	WAIT_FALLING_EDGE(N64_PIN);

	// Wait ~2us between line reads

	asm volatile(MICROSECOND_NOPS MICROSECOND_NOPS);

	// Read a bit from the line and store as a byte in "rawData"
	*rawDataPtr = PIND & 0b00000100;
	++rawDataPtr;
	if (--bits == 0) return;

	goto read_loop;
}

void N64Spy::writeSerial() {
	const unsigned char first = 2;

	for (unsigned char i = first; i < first + N64_BITCOUNT; i++) {
		Serial.write(rawData[i] ? ONE : ZERO);
	}
	Serial.write(SPLIT);
}

void N64Spy::debugSerial() {
	Serial.print(rawData[0]);
	Serial.print("|");
	int j = 0;
	for (unsigned char i = 0; i < 32; i++) {
		if (j % 8 == 0 && j != 0)
			Serial.print("|");
		Serial.print(rawData[i + 2] ? "1" : "0");
		j++;
	}
	Serial.print("\n");
}
