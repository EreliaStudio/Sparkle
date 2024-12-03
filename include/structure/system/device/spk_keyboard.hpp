#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/system/device/spk_input_state.hpp"

namespace spk
{
	/**
	 * @struct Keyboard
	 * @brief Represents the state of a keyboard, including key states and the current glyph.
	 *
	 * The `Keyboard` struct tracks the state of each key on the keyboard using `InputState` and provides
	 * additional information such as the current glyph being typed.
	 *
	 * ### Example
	 * @code
	 * spk::Keyboard keyboard;
	 * if (keyboard.state[spk::Keyboard::Key::A].isPressed())
	 * {
	 *	 std::wcout << L"Key A is pressed!" << std::endl;
	 * }
	 * keyboard.glyph = L'B';
	 * @endcode
	 */
	struct Keyboard
	{
		/**
		 * @enum Key
		 * @brief Enumerates all possible keys on a keyboard with their respective key codes.
		 */
		enum Key
		{
			Backspace = 8,	  ///< Backspace key.
			Tab = 9,			///< Tab key.
			Clear = 12,		 ///< Clear key.
			Return = 13,		///< Return (Enter) key.
			Shift = 16,		 ///< Shift key.
			Control = 17,	   ///< Control key.
			Alt = 18,		   ///< Alt key.
			Pause = 19,		 ///< Pause key.
			Capslock = 20,	  ///< Caps Lock key.
			Escape = 27,		///< Escape key.
			Convert = 28,	   ///< IME Convert key.
			Non_convert = 29,   ///< IME Non-Convert key.
			Accept = 30,		///< IME Accept key.
			Mode_change = 31,   ///< IME Mode Change key.
			Space = 32,		 ///< Space bar.
			Prior = 33,		 ///< Page Up key.
			Next = 34,		  ///< Page Down key.
			End = 35,		   ///< End key.
			Home = 36,		  ///< Home key.
			LeftArrow = 37,	 ///< Left arrow key.
			UpArrow = 38,	   ///< Up arrow key.
			RightArrow = 39,	///< Right arrow key.
			DownArrow = 40,	 ///< Down arrow key.
			Select = 41,		///< Select key.
			Print = 42,		 ///< Print key.
			Execute = 43,	   ///< Execute key.
			Snapshot = 44,	  ///< Print Screen key.
			Insert = 45,		///< Insert key.
			Delete = 46,		///< Delete key.
			Help = 47,		  ///< Help key.
			Key0 = 48,		  ///< Number 0 key.
			Key1 = 49,		  ///< Number 1 key.
			Key2 = 50,		  ///< Number 2 key.
			Key3 = 51,		  ///< Number 3 key.
			Key4 = 52,		  ///< Number 4 key.
			Key5 = 53,		  ///< Number 5 key.
			Key6 = 54,		  ///< Number 6 key.
			Key7 = 55,		  ///< Number 7 key.
			Key8 = 56,		  ///< Number 8 key.
			Key9 = 57,		  ///< Number 9 key.
			A = 65,			 ///< Letter A key.
			B = 66,			 ///< Letter B key.
			C = 67,			 ///< Letter C key.
			D = 68,			 ///< Letter D key.
			E = 69,			 ///< Letter E key.
			F = 70,			 ///< Letter F key.
			G = 71,			 ///< Letter G key.
			H = 72,			 ///< Letter H key.
			I = 73,			 ///< Letter I key.
			J = 74,			 ///< Letter J key.
			K = 75,			 ///< Letter K key.
			L = 76,			 ///< Letter L key.
			M = 77,			 ///< Letter M key.
			N = 78,			 ///< Letter N key.
			O = 79,			 ///< Letter O key.
			P = 80,			 ///< Letter P key.
			Q = 81,			 ///< Letter Q key.
			R = 82,			 ///< Letter R key.
			S = 83,			 ///< Letter S key.
			T = 84,			 ///< Letter T key.
			U = 85,			 ///< Letter U key.
			V = 86,			 ///< Letter V key.
			W = 87,			 ///< Letter W key.
			X = 88,			 ///< Letter X key.
			Y = 89,			 ///< Letter Y key.
			Z = 90,			 ///< Letter Z key.
			LeftWindows = 91,   ///< Left Windows key.
			RightWindows = 92,  ///< Right Windows key.
			App = 93,		   ///< Application key.
			Sleep = 95,		 ///< Sleep key.
			Numpad0 = 96,	   ///< Numpad 0 key.
			Numpad1 = 97,	   ///< Numpad 1 key.
			Numpad2 = 98,	   ///< Numpad 2 key.
			Numpad3 = 99,	   ///< Numpad 3 key.
			Numpad4 = 100,	  ///< Numpad 4 key.
			Numpad5 = 101,	  ///< Numpad 5 key.
			Numpad6 = 102,	  ///< Numpad 6 key.
			Numpad7 = 103,	  ///< Numpad 7 key.
			Numpad8 = 104,	  ///< Numpad 8 key.
			Numpad9 = 105,	  ///< Numpad 9 key.
			NumpadMultiply = 106, ///< Numpad Multiply key.
			NumpadPlus = 107,   ///< Numpad Plus key.
			NumpadSeparator = 108, ///< Numpad Separator key.
			NumpadMinus = 109,  ///< Numpad Minus key.
			NumpadDecimal = 110, ///< Numpad Decimal key.
			NumpadDivide = 111, ///< Numpad Divide key.
			F1 = 112,		   ///< F1 key.
			F2 = 113,		   ///< F2 key.
			F3 = 114,		   ///< F3 key.
			F4 = 115,		   ///< F4 key.
			F5 = 116,		   ///< F5 key.
			F6 = 117,		   ///< F6 key.
			F7 = 118,		   ///< F7 key.
			F8 = 119,		   ///< F8 key.
			F9 = 120,		   ///< F9 key.
			F10 = 121,		  ///< F10 key.
			F11 = 122,		  ///< F11 key.
			F12 = 123,		  ///< F12 key.
			F13 = 124,		  ///< F13 key.
			F14 = 125,		  ///< F14 key.
			F15 = 126,		  ///< F15 key.
			F16 = 127,		  ///< F16 key.
			F17 = 128,		  ///< F17 key.
			F18 = 129,		  ///< F18 key.
			F19 = 130,		  ///< F19 key.
			F20 = 131,		  ///< F20 key.
			F21 = 132,		  ///< F21 key.
			F22 = 133,		  ///< F22 key.
			F23 = 134,		  ///< F23 key.
			F24 = 135,		  ///< F24 key.
			NumLock = 144,	  ///< Num Lock key.
			Scroll = 145,	   ///< Scroll Lock key.
			LeftShift = 160,	///< Left Shift key.
			RightShift = 161,   ///< Right Shift key.
			LeftControl = 162,  ///< Left Control key.
			RightControl = 163, ///< Right Control key.
			LeftMenu = 164,	 ///< Left Menu key.
			RightMenu = 165,	///< Right Menu key.
			SemiColon = 186,	///< Semicolon key.
			Plus = 187,		 ///< Plus key.
			Comma = 188,		///< Comma key.
			Minus = 189,		///< Minus key.
			Period = 190,	   ///< Period (dot) key.
			QuestionMark = 191, ///< Question mark key.
			Tilde = 192,		///< Tilde key.
			LeftBracket = 219,  ///< Left bracket key.
			VerticalLine = 220, ///< Vertical line (backslash) key.
			RightBracket = 221, ///< Right bracket key.
			Quote = 222,		///< Quote key.
			Unknow = 223,	   ///< Unknown key.
			AngleBracket = 226, ///< Angle bracket key.
			Process = 229,	  ///< Process key.
			MaxNbKey = 255	  ///< Maximum number of keys.
		};

		InputState state[Key::MaxNbKey]; ///< Array representing the state of all keys on the keyboard.
		wchar_t glyph; ///< The current glyph being typed on the keyboard.
	};
}

/**
 * @brief Outputs a string representation of a `Keyboard::Key` to a stream.
 * @param p_os The output stream.
 * @param p_key The key to output.
 * @return Reference to the output stream.
 */
std::ostream& operator << (std::ostream& p_os, const spk::Keyboard::Key& p_key);

/**
 * @brief Outputs a string representation of a `Keyboard::Key` to a wide stream.
 * @param p_os The wide output stream.
 * @param p_key The key to output.
 * @return Reference to the wide output stream.
 */
std::wostream& operator << (std::wostream& p_os, const spk::Keyboard::Key& p_key);